/*
    Copyright Verbux Soluciones Informáticas Junio 2018
*/
/**
 * @file
 * @author Ignacio Maldonado Aylwin
 *
 */
#include "include/can_collector_utils.h"
#include "../build/include/sdkconfig.h"


#define MESSAGE_QUEUE_LENGTH 5

static const int RX_BUF_SIZE = 128;
volatile uint32_t ulIdleCycleCount = 0UL;
volatile bool packetREADY;

void vApplicationIdleHook( void ) {
    /* This hook function does nothing but increment a counter. */
    ulIdleCycleCount++;
}

void collector_query_task(void *queueStruct){
    is_vin = false;
    ESP_LOGI("COLLECTOR_INIT", "Query Task creation successful");
    vTaskDelay(3000/portTICK_PERIOD_MS);

    while(!is_vin) {
        elm327_query_VIN();
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }

    for(;;) {
        elm327_query_fueltank();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        elm327_query_oiltemp();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        elm327_query_speed();
        vTaskDelay(700 / portTICK_PERIOD_MS);
        packetREADY = true;

        vTaskDelay(4300 / portTICK_PERIOD_MS);
        ESP_LOGI("HOUSEKEEPING", "Idle task count: %i",ulIdleCycleCount);
        ESP_LOGI("HOUSEKEEPING", "Available Heap Size: %i bytes",esp_get_free_heap_size());
        ulIdleCycleCount = 0UL;
    }
    vTaskDelete(NULL);
}

//Proceso de monitoreo de interfase UART
void collector_elm_rx_task(void *queueStruct) {
    ESP_LOGI("COLLECTOR_INIT", "ELM RX Task creation successful");

    uint8_t* data;
    for(;;) {

        data = (uint8_t*) pvPortMalloc(ELM_RX_BUF_SIZE+1);
        while(data == NULL){
            ESP_LOGI("ELM_RX_TASK","Waiting for available heap space...");
            vTaskDelay(200/portTICK_PERIOD_MS);
            data = (uint8_t*) pvPortMalloc(ELM_RX_BUF_SIZE+1);
        }

        int rxBytes = uart_read_bytes(ELM_UART_NUM, data, ELM_RX_BUF_SIZE, 100 / portTICK_RATE_MS);
        if (rxBytes > 0) {

            data[rxBytes] = 0;
            //ESP_LOGI("ELM_RX_TASK", "Read %d bytes: '%s'", rxBytes, data);
            //ESP_LOGI("ELM_RX_TASK", "%s", data);
            ESP_LOG_BUFFER_HEXDUMP("ELM_RX_TASK_HEXDUMP", data, rxBytes, ESP_LOG_INFO);

            //Send through queue to data processing Task
            xQueueSend(((struct param *)queueStruct)->rxQueue,(void *)(&data),0);
            // data will be vPortFreed by receiving function
        } else {
                vPortFree(data);
            }
        //vPortFree(data);
        //ESP_LOGI("HOUSEKEEPING", "Available Heap Size: %i bytes",esp_get_free_heap_size());
    }
    //vPortFree(data);
    vTaskDelete(NULL);
}

//Proceso de monitoreo de interfase UART
void collector_gps_rx_task(void *queueStruct) {
    ESP_LOGI("COLLECTOR_INIT", "GPS RX Task creation successful");

    uint8_t* data;
    for(;;) {

        data = (uint8_t*) pvPortMalloc(GPS_RX_BUF_SIZE+1);
        while(data == NULL){
            ESP_LOGI("GPS_RX_TASK","Waiting for available heap space...");
            vTaskDelay(500/portTICK_PERIOD_MS);
            data = (uint8_t*) pvPortMalloc(GPS_RX_BUF_SIZE+1);
        }

        int rxBytes = uart_read_bytes(GPS_UART_NUM, data, GPS_RX_BUF_SIZE, 100 / portTICK_RATE_MS);
        if (rxBytes > 0) {

            data[rxBytes] = 0;
            //ESP_LOGI("ELM_RX_TASK", "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOGI("GPS_RX_TASK", "%s", data);
            //ESP_LOG_BUFFER_HEXDUMP("ELM_RX_TASK_HEXDUMP", data, rxBytes, ESP_LOG_INFO);

            //Send through queue to data processing Task
            xQueueSend(((struct param *)queueStruct)->rxQueue,(void *)(&data),0);
            // data will be vPortFreed by receiving function
        } else {
                vPortFree(data);
            }
        //vPortFree(data);
        //ESP_LOGI("HOUSEKEEPING", "Available Heap Size: %i bytes",esp_get_free_heap_size());
    }
    //vPortFree(data);
    vTaskDelete(NULL);
}

void collector_parse_task(void *queueStruct){
    ESP_LOGI("COLLECTOR_INIT", "Parse Task creation successful");

    void **buff = pvPortMalloc(sizeof(void *));
    BaseType_t xStatus;
    can_msg_t msg_type;

    elm327_data_t packet;
    elm327_new_data(&packet);
    packet.fields = MISC_FIELD;

    for(;;){
        xStatus = xQueueReceive(((struct param *)queueStruct)->rxQueue, buff, 200 / portTICK_PERIOD_MS);
        if(xStatus == pdPASS) {
            if(parse_is_GPS((uint8_t *)(*buff))){
                ESP_LOGI("PARSE_TASK", "Message Type Received: GPS");
                parse_GPS((uint8_t *)(*buff),&packet);
            }
            else if(parse_is_data((uint8_t *)(*buff))){
                msg_type = parse_check_msg_type((uint8_t *)*buff,6);
                ESP_LOGI("PARSE_TASK", "Message Type Received: %04x", msg_type);
                uint8_t var = parse_msg(*buff);
                switch(msg_type){
                    case FUELTANK_MSG:
                        packet.fuel = var;
                        packet.fields = packet.fields | FUEL_FIELD;
                        ESP_LOGI("FUELTANK_MSG","Fuel Level = %02x",var);
                        break;
                    case OILTEMP_MSG:
                        packet.temp = var;
                        packet.fields = packet.fields | TEMP_FIELD;
                        ESP_LOGI("OILTEMP_MSG","Oil Temp = %02x",var);
                        break;
                    case SPEED_MSG:
                        packet.speed = var;
                        packet.fields = packet.fields | SPEED_FIELD;
                        ESP_LOGI("SPEED_MSG","Speed = %02x",var);
                        break;
                    case VIN_MSG:
                        parse_vin(VIN,*buff);
                        memcpy(packet.VIN,VIN,17);
                        packet.fields = packet.fields | VIN_FIELD;
                        is_vin = true;
                        ESP_LOGI("VIN_MSG","VIN = %s",VIN);
                        break;
                    case UNKNOWN_MSG:
                        break;
                }
            }
            else{
                ESP_LOGI("PARSE_TASK", "No Data!");
            }
            vPortFree(*buff);

            if(packetREADY){
                ESP_LOGI("PARSE_TASK","Packet Ready for Sending");

                if(xQueueSend(((struct param *)queueStruct)->OutQueue,&packet,0) == pdPASS){
                    ESP_LOGI("PARSE_TASK","Packet sent to Outgoing Queue");
                }
                else{
                    if(xQueueSend(((struct param *)queueStruct)->storeQueue,&packet,0) == pdPASS){
                        ESP_LOGI("PARSE_TASK","Packet sent to Storage Queue");
                    }
                    else{
                        ESP_LOGI("PARSE_TASK","Storage Queue Full!");
                    }
                }
                //elm327_print(packet);
                packet.fields = MISC_FIELD;// | VIN_FIELD;
                packetREADY = false;
            }
        }
    }
    vTaskDelete(NULL);
}

void collector_card_task(void *queueStruct){
    ESP_LOGI("COLLECTOR_INIT", "Card Task creation successful");

    elm327_data_t message;

    int stackdepth = fStack_depth;

    for(;;) {
        if (uxQueueMessagesWaiting(((struct param *) queueStruct)->OutQueue) < MESSAGE_QUEUE_LENGTH && stackdepth > 0) {

            fStack_pop(&message);
            stackdepth--;
            ESP_LOGI("SD_TASK", "Message popped from stack. Stack Depth: %i",stackdepth);
            xQueueSend(((struct param *) queueStruct)->OutQueue, &message, 100/portTICK_PERIOD_MS);

        }

        if (xQueueReceive(((struct param *) queueStruct)->storeQueue, &message, 100/portTICK_PERIOD_MS) == pdPASS) {

            fStack_push(&message);
            stackdepth++;
            ESP_LOGI("SD_TASK", "Message pushed to stack. Stack Depth: %i",stackdepth);
        }
    }
    vTaskDelete(NULL);
}

void collector_SIM_task(void *queueStruct){

    ESP_LOGI("COLLECTOR_INIT", "SIM Task creation successful");


    //ESP_LOGI("COLLECTOR_INIT", "ppposInit() exitoso");

    // ==== Get time from NTP server =====

    time_t now = 0;
    struct tm timeinfo = { 0 };
    initialize_sntp();
    time(&now);
    localtime_r(&now, &timeinfo);

     // ==== Create PPPoS tasks ====
    mqtt_app_start(msgQueues.OutQueue);
    while(1)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
   vTaskDelete(NULL);
}

// Inicializa el módulo UART #0 que está conectalo a la interfase USB-UART
void collector_init(void) {

    //elm327_init();
    GPS_init();
    //stack_init();

    msgQueues.rxQueue = xQueueCreate(MESSAGE_QUEUE_LENGTH, sizeof(void *));
    if(msgQueues.rxQueue != (NULL)){
        ESP_LOGI("RX_QUEUE", "rxQueue creation successful");
    }

    msgQueues.OutQueue = xQueueCreate(MESSAGE_QUEUE_LENGTH, sizeof(elm327_data_t));
    if(msgQueues.OutQueue != (NULL)){
        ESP_LOGI("OUT_QUEUE", "OutQueue creation successful");
    }

    msgQueues.storeQueue = xQueueCreate(MESSAGE_QUEUE_LENGTH, sizeof(elm327_data_t));
    if(msgQueues.storeQueue != (NULL)){
        ESP_LOGI("STORE_QUEUE", "storeQueue creation successful");
    }

    xTaskCreate(collector_elm_rx_task, "collector_rx_task", 1024 * 2, (void *)&msgQueues, configMAX_PRIORITIES -1, NULL);
    xTaskCreate(collector_gps_rx_task, "collector_rx_task", 1024 * 2, (void *)&msgQueues, configMAX_PRIORITIES -1, NULL);
    xTaskCreate(collector_parse_task, "collector_parse_task", 1024 * 2, (void *)&msgQueues, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(collector_card_task, "collector_card_task", 1024 * 2, (void *)&msgQueues, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(collector_SIM_task, "collector_SIM_task", 1024 * 2, (void *)&msgQueues, configMAX_PRIORITIES, NULL);
    xTaskCreate(collector_query_task, "collector_query_task", 2 * 1024, NULL, configMAX_PRIORITIES - 3, NULL);
}