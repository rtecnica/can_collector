/*
    Copyright Verbux Soluciones Informáticas Julio 2018
*/
/**
 * @file
 * @author Ignacio Maldonado Aylwin
 *
 */

#include "L80GPS.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "string.h"

bool GPS_sendData(const char* logName, unsigned char* data, const int len) {
    union Data{
        const char *string;
        unsigned char *raw;
    }uni;

    uni.raw = data;

    const int txBytes = uart_write_bytes(GPS_UART_NUM, uni.string, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    ESP_LOG_BUFFER_HEXDUMP(logName, data, txBytes, ESP_LOG_INFO);

    return txBytes == len;
}


void GPS_init(void){
    
    const uart_config_t uart_config = {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(GPS_UART_NUM, &uart_config);
    uart_set_pin(GPS_UART_NUM, GPS_TXD_PIN, GPS_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(GPS_UART_NUM, GPS_RX_BUF_SIZE * 2, 0, 0, NULL, 0);

    char *CMD =  "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n";
    uint8_t *CMD_data = malloc(strlen(CMD));
    memccpy(CMD_data,CMD,1,strlen(CMD));
    vTaskDelay(1000/portTICK_PERIOD_MS);
    GPS_sendData("GPS_init", CMD_data,strlen(CMD));
    free(CMD_data);
/*
    char *CMD2 =  "$PQTXT,W,0,1*23\r\n";
    uint8_t *CMD2_data = malloc(strlen(CMD2));
    memccpy(CMD2_data,CMD2,1,strlen(CMD2));
    vTaskDelay(3000/portTICK_PERIOD_MS);
    GPS_sendData("GPS_noTXT", CMD2_data,strlen(CMD2));
    vTaskDelay(3000/portTICK_PERIOD_MS);
    GPS_sendData("GPS_noTXT", CMD2_data,strlen(CMD2));
    vTaskDelay(3000/portTICK_PERIOD_MS);
    GPS_sendData("GPS_noTXT", CMD2_data,strlen(CMD2));
    free(CMD2_data);
    */
}