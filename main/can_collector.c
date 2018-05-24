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

#include "driver/gpio.h"

#include "time.h"
#include "sys/time.h"

#include "driver/uart.h"
#include "soc/uart_struct.h"

#define APP_TAG "CAN_DATA_COLLECTOR"
#define DEVICE_NAME "CAN_DATA_COLLECTOR"

#define BLINK_GPIO 12

#define TXD_PIN (GPIO_NUM_1)
#define RXD_PIN (GPIO_NUM_3)

#define SPP_DATA_LEN ESP_SPP_MAX_MTU


static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;

static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_NONE;
static const esp_spp_role_t role_master = ESP_SPP_ROLE_MASTER;

static esp_bd_addr_t peer_bd_addr;
static uint8_t peer_bdname_len;
static char peer_bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
static const char remote_device_name[] = "ESP_SPP_ACCEPTOR";
static const esp_bt_inq_mode_t inq_mode = ESP_BT_INQ_MODE_GENERAL_INQUIRY;
static const uint8_t inq_len = 30;
static const uint8_t inq_num_rsps = 0;

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
    esp_log_level_set("RX_TASK", ESP_LOG_INFO);
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

    esp_log_buffer_hex("",param->data_ind.data,param->data_ind.len);
    blink_twice();
}

//Funciones de inicialización y manejo de eventos BT
static bool get_name_from_eir(uint8_t *eir, char *bdname, uint8_t *bdname_len) {
    uint8_t *rmt_bdname = NULL;
    uint8_t rmt_bdname_len = 0;

    if (!eir) {
        return false;
    }

    rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rmt_bdname_len);
    if (!rmt_bdname) {
        rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rmt_bdname_len);
    }

    if (rmt_bdname) {
        if (rmt_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN) {
            rmt_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        }

        if (bdname) {
            memcpy(bdname, rmt_bdname, rmt_bdname_len);
            bdname[rmt_bdname_len] = '\0';
        }
        if (bdname_len) {
            *bdname_len = rmt_bdname_len;
        }
        return true;
    }

    return false;
}

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
    case ESP_SPP_INIT_EVT:
        ESP_LOGI(APP_TAG, "ESP_SPP_INIT_EVT");
        esp_bt_dev_set_device_name(DEVICE_NAME);
        esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        esp_bt_gap_start_discovery(inq_mode, inq_len, inq_num_rsps);
        break;

    case ESP_SPP_DISCOVERY_COMP_EVT:
        ESP_LOGI(APP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT status=%d scn_num=%d",param->disc_comp.status, param->disc_comp.scn_num);
        if (param->disc_comp.status == ESP_SPP_SUCCESS) {
            esp_spp_connect(sec_mask, role_master, param->disc_comp.scn[0], peer_bd_addr);
            ESP_LOGI(APP_TAG,"CONNECTING....");
        }
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
        ESP_LOGI(APP_TAG, "ESP_SPP_DATA_IND_EVT");
        bt_data_rcv_handler(param);
        break;

    case ESP_SPP_CONG_EVT:
        ESP_LOGI(APP_TAG, "ESP_SPP_CONG_EVT cong=%d", param->cong.cong);
        if (param->cong.cong == 0) {}
        break;

    case ESP_SPP_WRITE_EVT:
        ESP_LOGI(APP_TAG, "ESP_SPP_WRITE_EVT len=%d cong=%d", param->write.len , param->write.cong);
        if (param->write.cong == 0) {}
        break;

    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(APP_TAG, "ESP_SPP_SRV_OPEN_EVT");
        break;

    default:
        break;
    }
}

static void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch(event){
        case ESP_BT_GAP_DISC_RES_EVT:
            ESP_LOGI(APP_TAG, "ESP_BT_GAP_DISC_RES_EVT");
            esp_log_buffer_hex(APP_TAG, param->disc_res.bda, ESP_BD_ADDR_LEN);
            for (int i = 0; i < param->disc_res.num_prop; i++){
                if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR
                    && get_name_from_eir(param->disc_res.prop[i].val, peer_bdname, &peer_bdname_len)){
                    esp_log_buffer_char(APP_TAG, peer_bdname, peer_bdname_len);
                    if (strlen(remote_device_name) == peer_bdname_len
                        && strncmp(peer_bdname, remote_device_name, peer_bdname_len) == 0) {
                        memcpy(peer_bd_addr, param->disc_res.bda, ESP_BD_ADDR_LEN);
                        esp_spp_start_discovery(peer_bd_addr);
                        esp_bt_gap_cancel_discovery();
                    }
                }
            }
            break;
        case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
            ESP_LOGI(APP_TAG, "ESP_BT_GAP_DISC_STATE_CHANGED_EVT");
            break;
        case ESP_BT_GAP_RMT_SRVCS_EVT:
            ESP_LOGI(APP_TAG, "ESP_BT_GAP_RMT_SRVCS_EVT");
            break;
        case ESP_BT_GAP_RMT_SRVC_REC_EVT:
            ESP_LOGI(APP_TAG, "ESP_BT_GAP_RMT_SRVC_REC_EVT");
            break;
        default:
            break;
    }
}

void app_main() {

    blink_init();

    uart_init();

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

    if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK) {
        ESP_LOGE(APP_TAG, "%s gap register failed: %s\n", __func__, esp_err_to_name(ret));
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

    xTaskCreate(uart_rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
}