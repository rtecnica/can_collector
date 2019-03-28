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

/**
* @brief Enumeration defining OTA image validity
*   ESP_OTA_IMG_VALID: Firmware verified and stable, no rollback necesarry
*   ESP_OTA_IMG_UNDEFINED
*   ESP_OTA_IMG_INVALID
*   ESP_OTA_IMG_ABORTED
*   ESP_OTA_IMG_NEW: New firmware downloaded, pending reset and verification
*   ESP_OTA_IMG_PENDING_VERIFY: Firmware loaded, pending verification. If rebooted in this state, rollback is initiated
*/
typedef enum {
    ESP_OTA_IMG_VALID           = 0xAA,
    ESP_OTA_IMG_UNDEFINED       = 0xBB,
    ESP_OTA_IMG_INVALID         = 0xCC,
    ESP_OTA_IMG_ABORTED         = 0xDD,
    ESP_OTA_IMG_NEW             = 0xEE,
    ESP_OTA_IMG_PENDING_VERIFY  = 0xFF,
} OTA_state_t;

/**
* @brief Global variable for tracking OTA state
*/
int16_t OTA_state;


/**
 * @brief Function for setting OTA state in NVS
 *
 * @param state : OTA state to be set
 *
 */
void OTA_set_state(OTA_state_t state);

/**
 * @brief Function for retrieving OTA state from NVS
 *
 *
 * @returns Current OTA state written in NVS
 */
OTA_state_t OTA_get_state();

/**
 * @brief Initializer function for loading NVS handles into memory and initializing OTA variables
 */
void OTA_init();

/**
 * @brief Checks if latest assigned version is downloaded, and initiates OTA sequence
 */
void OTA_download_latest_version();

#endif //CAN_COLLECTOR_OTA_UTILS_H
