//
// Created by Ignacio Maldonado Aylwin on 5/29/18.
//
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "driver/uart.h"
#include "soc/uart_struct.h"

#include "esp_spp_api.h"

#include "elm327.h"


static const int RX_BUF_SIZE = 1024;

//Proceso de monitoreo de interfase UART
void elm327_rx_task(void *pvParameters) {
    esp_log_level_set("RX_TASK", ESP_LOG_INFO);

    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);

    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0) {

            data[rxBytes] = 0;
            ESP_LOGI("RX_TASK", "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP("RX_TASK", data, rxBytes, ESP_LOG_INFO);
            uint32_t *tmp = (uint32_t *)pvParameters;
            ESP_LOGI("RX_TASK", "Sending to handle %i", *tmp);

            //TODO Find a better fucking solution for this
            esp_spp_write(*tmp,rxBytes,data);


        }
    }
    free(data);
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

    xTaskCreate(elm327_rx_task, "elm327_rx_task", 1024 * 2, (void *)bt_handle, configMAX_PRIORITIES, NULL);
}

//Función de utilidad para enviar bytestream a través de UART
int elm327_sendData(const char* logName, unsigned char* data, const int len) {
    union Data{
        const char *string;
        unsigned char *raw;
    }uni;

    uni.raw = data;

    const int txBytes = uart_write_bytes(UART_NUM_1, uni.string, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    ESP_LOG_BUFFER_HEXDUMP("TX_TASK", data, txBytes, ESP_LOG_INFO);
    return txBytes;
}