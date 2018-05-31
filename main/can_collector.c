/*
    Copyright Verbux Soluciones Informáticas Mayo 2018
*/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <esp_spp_api.h>

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

#include "../include/elm327.h"

#include "driver/gpio.h"

#include "time.h"
#include "sys/time.h"

#include "driver/uart.h"
#include "soc/uart_struct.h"

#define APP_TAG "CAN_DATA_COLLECTOR"
#define SPP_SERVER_NAME "CAN_DATA_COLLECTOR"
#define EXAMPLE_DEVICE_NAME "CAN_DATA_COLLECTOR"

#define BLINK_GPIO GPIO_NUM_12

#define TXD_PIN GPIO_NUM_1
#define RXD_PIN GPIO_NUM_3


static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_NONE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;


static const int RX_BUF_SIZE = 1024;

uint32_t bt_handle;

//Función de utilidad para enviar bytestream a través de UART
int sendData(const char* logName, unsigned char* data, const int len) {
    union Data{
        const char *string;
        unsigned char *raw;
    }uni;

    uni.raw = data;

    const int txBytes = uart_write_bytes(UART_NUM_0, uni.string, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    ESP_LOG_BUFFER_HEXDUMP("TX_TASK", data, txBytes, ESP_LOG_INFO);
    return txBytes;
}

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

// Inicializa el módulo UART #0 que está conectalo a la interfase USB-UART
void uart_init() {
    const uart_config_t uart_config = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_0, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
}

//Proceso de monitoreo de interfase UART
static void uart_rx_task() {
    esp_log_level_set("RX_TASK", ESP_LOG_NONE);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);

    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_0, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0) {

            data[rxBytes] = 0;
            ESP_LOGI("RX_TASK", "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP("RX_TASK", data, rxBytes, ESP_LOG_INFO);

            esp_spp_write(bt_handle,rxBytes,data);
            blink_twice();
        }
    }
    free(data);
}

//Handler de mensaje entrante por BT
void bt_data_rcv_handler(esp_spp_cb_param_t *param) {
    sendData("TX_TASK", param->data_ind.data, param->data_ind.len);
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

    uart_init();

    esp_log_level_set("*",ESP_LOG_NONE);

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

    xTaskCreate(uart_rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);
}