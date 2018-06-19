//
// Created by Ignacio Maldonado Aylwin on 5/29/18.
//
/**
 * @file
 *
 */
#include "include/elm327.h"
#include "include/parse_utils.h"
#include "include/card_utils.h"

#define MESSAGE_QUEUE_LENGTH 5
static const int RX_BUF_SIZE = 128;

char VIN[17];

//Proceso de monitoreo de interfase UART
void elm327_rx_task(void *pvParameters) {
    for(;;) {
        uint8_t* data = (uint8_t*) pvPortMalloc(RX_BUF_SIZE+1);
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
            esp_spp_write(*((( struct param *)pvParameters)->out_bt_handle),rxBytes,data);

            xQueueSend((( struct param *)pvParameters)->rxQueue,(void *)(&data),0);
            //vPortFree(data); // data will be vPortFreed by recieving function
        }
        else{
            vPortFree(data);
        }
    }
    //vPortFree(data);
    vTaskDelete(NULL);
}

void elm327_parse_task(void *pvParameters){

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

// Inicializa el módulo UART #0 que está conectalo a la interfase USB-UART
void elm327_init(uint32_t *bt_handle) {
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

    vParams.out_bt_handle = bt_handle;

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

    xTaskCreate(elm327_rx_task, "elm327_rx_task", 1024 * 2, (void *)&vParams, configMAX_PRIORITIES, NULL);
    xTaskCreate(elm327_parse_task, "elm327_parse_task", 1024 * 2, (void *)&vParams, configMAX_PRIORITIES - 1, NULL);
}

//Función de utilidad para enviar bytestream a través de UART al ELM327
bool elm327_sendData(const char* logName, unsigned char* data, const int len) {
    union Data{
        const char *string;
        unsigned char *raw;
    }uni;

    uni.raw = data;
    
    const int txBytes = uart_write_bytes(UART_NUM_1, uni.string, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    ESP_LOG_BUFFER_HEXDUMP(logName, data, txBytes, ESP_LOG_INFO);

    return txBytes == len;
}

bool elm327_reset(void){
    unsigned char msg[5] = {0x41, 0x54, 0x20, 0x5a, 0x0d};
    return elm327_sendData("Reset", msg, 5);
}

bool elm327_setCAN(void){
    unsigned char msg2[9] = {0x41, 0x54, 0x20, 0x54, 0x50, 0x20, 0x36, 0x0d};
    return elm327_sendData("Set Protocol", msg2, 8);
};

bool elm327_query_oiltemp(void){

    unsigned char msg[5] = {0x30, 0x31, 0x35, 0x43, 0x0d};

    return elm327_sendData("OilTemp Query", msg, 5);
}

bool elm327_query_fueltank(void){

    unsigned char msg[5] = {0x30, 0x31, 0x32, 0x46, 0x0d};

    return elm327_sendData("FuelTank Query", msg, 5);
}

bool elm327_query_speed(void){

    unsigned char msg[5] = {0x30, 0x31, 0x30, 0x44, 0x0d};

    return elm327_sendData("Speed Query", msg, 5);
}

bool elm327_query_VIN(void){
    unsigned char msg[5] = {0x30, 0x39, 0x30, 0x32, 0x0d};

    return elm327_sendData("VIN Query", msg, 5);
}

bool elm327_query_GPS(void){
    //TODO Write this function
    return 1;
}

void elm327_new_data(elm327_data_t *data){

    strcpy(data->VIN, "AAAAAAAAAAAAAAAAA");
    strcpy(data->LAT, "00000000");
    strcpy(data->LONG, "00000000");
    strcpy(data->TIME, "0000");
    data->temp = 0x46;
    data->fuel = 0x46;
    data->speed = 0x46;
    data->fields = ALL_FIELDS;
}