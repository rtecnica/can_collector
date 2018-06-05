//
// Created by Ignacio Maldonado Aylwin on 6/5/18.
//

#include "include/can_collector_utils.h"

void bt_data_rcv_handler(esp_spp_cb_param_t *param) {
    elm327_sendData("TX_TASK", param->data_ind.data, param->data_ind.len);
}

void queryTask(void *pvParameters){
    unsigned char msg[5] = {0x41, 0x54, 0x20, 0x5a, 0x0d};
    elm327_sendData("Reset", msg, 5);
    vTaskDelay(5000/portTICK_PERIOD_MS);
    unsigned char msg2[9] = {0x41, 0x54, 0x20, 0x54, 0x50, 0x20, 0x41, 0x36, 0x0d};
    elm327_sendData("Set Protocol", msg2, 9);
    vTaskDelay(3000/portTICK_PERIOD_MS);
    elm327_query_fueltank();
    vTaskDelay(3000/portTICK_PERIOD_MS);
    elm327_query_oiltemp();
    vTaskDelay(3000/portTICK_PERIOD_MS);
    elm327_query_speed();
    vTaskDelete(NULL);
}

void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
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

            xTaskCreate(queryTask, "queryTask", 2 * 1024, NULL, configMAX_PRIORITIES, NULL);


            break;
        default:
            break;
    }
}

void bt_init(void){

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
}