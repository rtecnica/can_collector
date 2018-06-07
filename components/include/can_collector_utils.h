//
// Created by Ignacio Maldonado Aylwin on 6/5/18.
//

/**
 *  A test class. A more elaborate class description.
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
 *  A test class. A more elaborate class description.
 */
uint32_t bt_handle;

static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_NONE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 *
 */
void bt_data_rcv_handler(esp_spp_cb_param_t *param);

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 *
 */
void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 *
 */
void bt_init(void);