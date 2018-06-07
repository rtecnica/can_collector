//
// Created by Ignacio Maldonado Aylwin on 5/29/18.
//

#include "include/elm327.h"
#include "include/parse_utils.h"

static const int RX_BUF_SIZE = 1024;

struct param {
    uint32_t *out_bt_handle;
    QueueHandle_t rxQueue;
} vParams;

//Proceso de monitoreo de interfase UART
void elm327_rx_task(void *pvParameters) {
    esp_log_level_set("RX_TASK", ESP_LOG_INFO);

    uint8_t* data;

    while (1) {
        data = (uint8_t*) malloc(RX_BUF_SIZE+1);
        while(data == NULL){
            vTaskDelay(100/portTICK_PERIOD_MS);
            data = (uint8_t*) malloc(RX_BUF_SIZE+1);
        }
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0) {

            data[rxBytes] = 0;
            ESP_LOGI("RX_TASK", "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP("RX_TASK_HEXDUMP", data, rxBytes, ESP_LOG_INFO);

            //Send through queue to data processing Task
            struct param *tmp = malloc(sizeof(*tmp));

            tmp = ( struct param *)pvParameters;

            esp_spp_write(*(tmp->out_bt_handle),rxBytes,data);

            xQueueSend(tmp->rxQueue,(void *)(&data),10);
            free(data); // data will be freed by recieving function
        }
    }
    free(data);
}

void elm327_parse_task(void *pvParameters){

    void **buff = malloc(sizeof(void **));
    struct param *tmp = malloc(sizeof(*tmp));
    BaseType_t xStatus;
            tmp = ( struct param *)pvParameters;

    for(;;){
        xStatus = xQueueReceive(tmp->rxQueue, buff, 10);
        uint8_t *msg = *buff;
        if(xStatus == pdPASS)
            ESP_LOGI("PARSE_TASK", "Message Type Recieved: %x", parse_check_msg_type(msg, 6));
        //TODO parse vars
        free((*buff));
    }
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
    vParams.rxQueue = xQueueCreate(5, sizeof(void *));
    if(vParams.rxQueue != (NULL)){
        ESP_LOGI("RX_QUEUE", "rxQueue creation successful.");
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

