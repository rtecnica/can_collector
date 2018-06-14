//
// Created by Ignacio Maldonado Aylwin on 6/5/18.
//
/**
 * @file
 *
 * @brief Utility functions for testing setup, mainly initializing and handling bluetooth connectivity for debugging.
 */

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "esp_log.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "elm327.h"

#define APP_TAG "CAN_DATA_COLLECTOR"
#define SPP_SERVER_NAME "CAN_DATA_COLLECTOR"
#define EXAMPLE_DEVICE_NAME "CAN_DATA_COLLECTOR"

/**
 *  @var bt_handle: Bluetooth connection handle for global use. For other parameters
 *  @see ESP_SPP_API_H
 */
uint32_t bt_handle;

static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_NONE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

/**
 * @brief Handler function for incoming bluetooth data
 *
 * @param[in] param : ESP_SPP_DATA_IND_EVENT asociated struct
 */
void bt_data_rcv_handler(esp_spp_cb_param_t *param);

/**
 * @brief Callback Function for handling ESP_SPP_*_EVENT
 *
 * @param[in] event : The event currently handled
 * @param[in] param : pointer to event asociated struct
 */
void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

/**
 * @brief Function for initialization of bluetooth stack
 */
void bt_init(void);