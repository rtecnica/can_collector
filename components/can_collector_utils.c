//
// Created by Ignacio Maldonado Aylwin on 6/5/18.
//
/**
 * @file
 */
#include "include/can_collector_utils.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/semphr.h"

#include "driver/uart.h"

#include "netif/ppp/pppos.h"
#include "netif/ppp/ppp.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/pppapi.h"

#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#include <esp_event.h>
#include <esp_wifi.h>

#include "apps/sntp/sntp.h"
#include "cJSON.h"

#include "../build/include/sdkconfig.h"

#define MESSAGE_QUEUE_LENGTH 5

#define TXD_PIN GPIO_NUM_32
#define RXD_PIN GPIO_NUM_36

static const char *TIME_TAG = "[SNTP]";

static const int RX_BUF_SIZE = 128;
volatile uint32_t ulIdleCycleCount = 0UL;

void vApplicationIdleHook( void ) {
    /* This hook function does nothing but increment a counter. */
    ulIdleCycleCount++;
}

void collector_query_task(void *pvParameters){

    elm327_reset();
    vTaskDelay(5000/portTICK_PERIOD_MS);
    elm327_setCAN();
    vTaskDelay(3000/portTICK_PERIOD_MS);
    elm327_query_VIN();
    vTaskDelay(3000/portTICK_PERIOD_MS);

    for(;;) {
        elm327_query_fueltank();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        elm327_query_oiltemp();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        elm327_query_speed();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        //elm327_query_GPS();
        //vTaskDelay(2000 / portTICK_PERIOD_MS);
        ESP_LOGI("##############", "Idle task count: %i",ulIdleCycleCount);
        ESP_LOGI("##############", "Available Heap Size: %i bytes",esp_get_free_heap_size());
        ulIdleCycleCount = 0UL;
    }
    vTaskDelete(NULL);
}

//Proceso de monitoreo de interfase UART
void collector_rx_task(void *pvParameters) {
    uint8_t* data;
    for(;;) {
        data = (uint8_t*) pvPortMalloc(RX_BUF_SIZE+1);
        while(data == NULL){
            ESP_LOGI("RX_TASK","Waiting for available heap space...");
            vTaskDelay(100/portTICK_PERIOD_MS);
            data = (uint8_t*) pvPortMalloc(RX_BUF_SIZE+1);
        }
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 500 / portTICK_RATE_MS);
        if (rxBytes > 0) {

            data[rxBytes] = 0;
            ESP_LOGI("RX_TASK", "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP("RX_TASK_HEXDUMP", data, rxBytes, ESP_LOG_INFO);

            //Send through queue to data processing Task
            xQueueSend(((struct param *)pvParameters)->rxQueue,(void *)(&data),0);
            //vPortFree(data); // data will be vPortFreed by recieving function
        }
        else{
            vPortFree(data);
        }
    }
    vPortFree(data);
    vTaskDelete(NULL);
}

void collector_parse_task(void *pvParameters){

    void **buff = pvPortMalloc(sizeof(void *));
    BaseType_t xStatus;
    can_msg_t msg_type;

    elm327_data_t packet;

    for(;;){
        xStatus = xQueueReceive(((struct param *)pvParameters)->rxQueue, buff, 100/portTICK_PERIOD_MS);
        if(xStatus == pdPASS) {
            msg_type = parse_check_msg_type((uint8_t *)*buff,6);
            ESP_LOGI("PARSE_TASK", "Message Type Recieved: %04x", msg_type);
            if(parse_is_data((uint8_t *)(*buff))){
                uint8_t var = (((uint8_t)parse_char_to_hex(((uint8_t *)(*buff))[11]))<<4) + ((uint8_t)parse_char_to_hex(((uint8_t *)(*buff))[12]));
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
                        vin_parse(VIN,*buff);
                        packet.fields = packet.fields | VIN_FIELD;
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

            if((packet.fields & (VIN_FIELD | SPEED_FIELD | FUEL_FIELD | TEMP_FIELD | MISC_FIELD)) == (VIN_FIELD | SPEED_FIELD | FUEL_FIELD | TEMP_FIELD | MISC_FIELD) ){
                ESP_LOGI("PARSE_TASK","Packet Ready for Sending");

                if(xQueueSend(((struct param *)pvParameters)->OutQueue,&packet,0) == pdPASS){
                    ESP_LOGI("PARSE_TASK","Packet sent to Outgoing Queue");
                }
                else{
                    if(xQueueSend(((struct param *)pvParameters)->storeQueue,&packet,0) == pdPASS){
                        ESP_LOGI("PARSE_TASK","Packet sent to Storage Queue");
                    }
                    else{
                        ESP_LOGI("PARSE_TASK","Storage Queue Full!");
                    }
                }
                packet.fields = VIN_FIELD | MISC_FIELD;
            }
        }
    }
    vTaskDelete(NULL);
}

void collector_card_task(void *pvParameters){

    stack_init();

    elm327_data_t message;

    int stackdepth = 0;

    for(;;) {
        if (uxQueueMessagesWaiting(((struct param *) pvParameters)->OutQueue) < MESSAGE_QUEUE_LENGTH && stackdepth > 0) {

            fStack_pop(&message);
            stackdepth--;
            ESP_LOGI("SD_TASK", "Message popped from stack");
            xQueueSend(((struct param *) pvParameters)->OutQueue, &message, 100/portTICK_PERIOD_MS);

        }

        if (xQueueReceive(((struct param *) pvParameters)->storeQueue, &message, 100/portTICK_PERIOD_MS) == pdPASS) {

            fStack_push(&message);
            stackdepth++;
            ESP_LOGI("SD_TASK", "Message pushed to stack. Stack Depth: %i",stackdepth);
        }
    }
    vTaskDelete(NULL);
}

void collector_SIM_task(void *pvParameters){

    if (ppposInit() == 0) {
        ESP_LOGE("PPPoS EXAMPLE", "ERROR: GSM not initialized, HALTED");
        while (1) {
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
    }

    // ==== Get time from NTP server =====
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;

    time(&now);
    localtime_r(&now, &timeinfo);

    while (1) {
        printf("\r\n");
        ESP_LOGI(TIME_TAG,"OBTAINING TIME");
        ESP_LOGI(TIME_TAG, "Initializing SNTP");
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "south-america.pool.ntp.org");
        sntp_init();
        ESP_LOGI(TIME_TAG,"SNTP INITIALIZED");

        // wait for time to be set
        now = 0;
        while ((timeinfo.tm_year < (2016 - 1900)) && (++retry < retry_count)) {
            ESP_LOGI(TIME_TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            time(&now);
            localtime_r(&now, &timeinfo);
            if (ppposStatus() != GSM_STATE_CONNECTED) break;
        }
        if (ppposStatus() != GSM_STATE_CONNECTED) {
            sntp_stop();
            ESP_LOGE(TIME_TAG, "Disconnected, waiting for reconnect");
            retry = 0;
            while (ppposStatus() != GSM_STATE_CONNECTED) {
                vTaskDelay(100 / portTICK_RATE_MS);
            }
            continue;
        }

        if (retry < retry_count) {
            ESP_LOGI(TIME_TAG, "TIME SET TO %s", asctime(&timeinfo));
            break;
        }
        else {
            ESP_LOGI(TIME_TAG, "ERROR OBTAINING TIME\n");
        }
        sntp_stop();
        break;
    }
    vTaskDelete(NULL);
}

// Inicializa el módulo UART #0 que está conectalo a la interfase USB-UART
void collector_init(void) {
    const uart_config_t uart_config = {
            .baud_rate = 38400,//115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);

    vParams.rxQueue = xQueueCreate(MESSAGE_QUEUE_LENGTH, sizeof(void *));
    if(vParams.rxQueue != (NULL)){
        ESP_LOGI("RX_QUEUE", "rxQueue creation successful");
    }

    vParams.OutQueue = xQueueCreate(MESSAGE_QUEUE_LENGTH, sizeof(elm327_data_t));
    if(vParams.OutQueue != (NULL)){
        ESP_LOGI("OUT_QUEUE", "OutQueue creation successful");
    }

    vParams.storeQueue = xQueueCreate(MESSAGE_QUEUE_LENGTH, sizeof(elm327_data_t));
    if(vParams.storeQueue != (NULL)){
        ESP_LOGI("STORE_QUEUE", "storeQueue creation successful");
    }

    xTaskCreate(collector_rx_task, "collector_rx_task", 1024 * 2, (void *)&vParams, configMAX_PRIORITIES -1, NULL);
    ESP_LOGI("COLLECTOR_INIT", "RX Task creation successful");
    xTaskCreate(collector_parse_task, "collector_parse_task", 1024 * 2, (void *)&vParams, configMAX_PRIORITIES - 2, NULL);
    ESP_LOGI("COLLECTOR_INIT", "Parse Task creation successful");
    xTaskCreate(collector_card_task, "collector_card_task", 1024 * 2, (void *)&vParams, configMAX_PRIORITIES - 2, NULL);
    ESP_LOGI("COLLECTOR_INIT", "Card Task creation successful");
    xTaskCreate(collector_SIM_task, "collector_SIM_task", 1024 * 2, (void *)&vParams, configMAX_PRIORITIES, NULL);
    ESP_LOGI("COLLECTOR_INIT", "SIM Task creation successful");
    xTaskCreate(collector_query_task, "collector_query_task", 2 * 1024, NULL, configMAX_PRIORITIES - 3, NULL);
    ESP_LOGI("COLLECTOR_INIT", "Query Task creation successful");
}