//
// Created by Ignacio Maldonado Aylwin on 5/29/18.
//
/**
 * @file
 *
 */
#include "include/elm327.h"

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

    memcpy(data->VIN, "AAAAAAAAAAAAAAAAA",17);
    memcpy(data->LAT, "00000000", 8);
    memcpy(data->LONG, "00000000", 8);
    memcpy(data->TIME, "0000", 8);
    data->temp = 0x46;
    data->fuel = 0x46;
    data->speed = 0x46;
    data->fields = ALL_FIELDS;
}
