/*
    Copyright Verbux Soluciones Informáticas Mayo 2018
*/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "nvs.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#include "../components/elm327.h"

#include "driver/gpio.h"

#include "time.h"
#include "sys/time.h"

#define APP_TAG "CAN_DATA_COLLECTOR"
#define SPP_SERVER_NAME "CAN_DATA_COLLECTOR"
#define EXAMPLE_DEVICE_NAME "CAN_DATA_COLLECTOR"

#define BLINK_GPIO GPIO_NUM_12


static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_NONE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

uint32_t bt_handle;

//Inicialización de pines GPIO para la señalización con LED
void blink_init() {
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

//Función de parpadeo de LEDS
void blink_twice(){

    gpio_set_level(BLINK_GPIO, 1);
    vTaskDelay(60 / portTICK_PERIOD_MS);

    gpio_set_level(BLINK_GPIO, 0);
    vTaskDelay(40 / portTICK_PERIOD_MS);

    gpio_set_level(BLINK_GPIO, 1);
    vTaskDelay(60 / portTICK_PERIOD_MS);

    gpio_set_level(BLINK_GPIO, 0);
}

//Handler de mensaje entrante por BT
void bt_data_rcv_handler(esp_spp_cb_param_t *param) {
    elm327_sendData("TX_TASK", param->data_ind.data, param->data_ind.len);
    //esp_log_buffer_hex("",param->data_ind.data,param->data_ind.len);
    blink_twice();
}

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event) {
        case ESP_SPP_INIT_EVT:
            ESP_LOGI(APP_TAG, "ESP_SPP_INIT_EVT");
            esp_bt_dev_set_device_name(EXAMPLE_DEVICE_NAME);
            esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
            esp_spp_start_srv(sec_mask,role_slave, 0, SPP_SERVER_NAME);
            break;
        case ESP_SPP_DISCOVERY_COMP_EVT:
            ESP_LOGI(APP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
            break;
        case ESP_SPP_OPEN_EVT:
            ESP_LOGI(APP_TAG, "ESP_SPP_OPEN_EVT");
            bt_handle = param->open.handle;
            break;
        case ESP_SPP_CLOSE_EVT:
            ESP_LOGI(APP_TAG, "ESP_SPP_CLOSE_EVT");
            break;
        case ESP_SPP_START_EVT:
            ESP_LOGI(APP_TAG, "ESP_SPP_START_EVT");
            break;
        case ESP_SPP_CL_INIT_EVT:
            ESP_LOGI(APP_TAG, "ESP_SPP_CL_INIT_EVT");
            break;
        case ESP_SPP_DATA_IND_EVT:
            ESP_LOGI(APP_TAG, "ESP_SPP_DATA_IND_EVT len=%d handle=%d", param->data_ind.len, param->data_ind.handle);
            esp_log_buffer_hex("",param->data_ind.data,param->data_ind.len);
            bt_data_rcv_handler(param);
            blink_twice();

            break;
        case ESP_SPP_CONG_EVT:
            ESP_LOGI(APP_TAG, "ESP_SPP_CONG_EVT");
            break;
        case ESP_SPP_WRITE_EVT:
            ESP_LOGI(APP_TAG, "ESP_SPP_WRITE_EVT");
            break;
        case ESP_SPP_SRV_OPEN_EVT:
            ESP_LOGI(APP_TAG, "ESP_SPP_SRV_OPEN_EVT");
            bt_handle = param->srv_open.handle;
            break;
        default:
            break;
    }
}

void app_main() {

    blink_init();

    esp_log_level_set("*",ESP_LOG_INFO);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );


    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(APP_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(APP_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(APP_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(APP_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
        ESP_LOGE(APP_TAG, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK) {
        ESP_LOGE(APP_TAG, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    elm327_init(&bt_handle);

}