/*
    Copyright Verbux Soluciones Informáticas Enero 2019
*/
/**
 * @file
 * @author Ignacio Maldonado
 * @brief
 *
 */

#ifndef __CAN_COLLECTOR_OTA_UTILS_H__
#define __CAN_COLLECTOR_OTA_UTILS_H__

#include "esp_ota_ops.h"

#define VERSION_LENGTH 6

//#include "freertos/FreeRTOS.h"


typedef enum {
    ESP_OTA_IMG_VALID           = 0xAA,
    ESP_OTA_IMG_UNDEFINED       = 0xBB,
    ESP_OTA_IMG_INVALID         = 0xCC,
    ESP_OTA_IMG_ABORTED         = 0xDD,
    ESP_OTA_IMG_NEW             = 0xEE,
    ESP_OTA_IMG_PENDING_VERIFY  = 0xFF,
} OTA_state_t;

OTA_state_t OTA_state;

void OTA_set_state(OTA_state_t state);

OTA_state_t OTA_get_state();

void ota_example_task();

void OTA_init();

void OTA_download_latest_version();

#endif //CAN_COLLECTOR_OTA_UTILS_H
