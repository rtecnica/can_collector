/**
 * @file
 * @author Ignacio Maldonado Aylwin
 *
 */
#include "include/OTA_utils.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_http_client.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "string.h"

#define EXAMPLE_SERVER_URL CONFIG_FIRMWARE_UPG_URL
#define BUFFSIZE 1024

#define VERSION_LENGTH 10

char* running_version = "0.0.0";

static const char *TAG = "OTA";
/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE + 1] = { 0 };
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

static const char *OTA_STATE_NVS_KEY = "OTA_app_state";

nvs_handle app_version_nvs_handle;
nvs_handle OTA_app_state_nvs_handle;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

esp_partition_t *configured;
esp_partition_t *running;
esp_partition_t *update_partition = NULL;

static esp_err_t event_handler(void *ctx, system_event_t *event){
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            /* This is a workaround as ESP32 WiFi libs don't currently
               auto-reassociate. */
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void http_cleanup(esp_http_client_handle_t client){
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void __attribute__((noreturn)) task_fatal_error(){
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    (void)vTaskDelete(NULL);

    while (1) {
        ;
    }
}

esp_err_t err;

//------------------------------------------------//

void OTA_set_state(OTA_state_t state){
    /*
    if(state == ESP_OTA_IMG_INVALID)
        nvs_set_i16(OTA_app_state_nvs_handle, "OTA_app_state", ESP_OTA_IMG_VALID);
    else
        nvs_set_i16(OTA_app_state_nvs_handle, "OTA_app_state", ESP_OTA_IMG_INVALID);
    */
    nvs_set_i16(OTA_app_state_nvs_handle, "OTA_app_state", state);
     };

OTA_state_t OTA_get_state(){

    err = nvs_get_i16(OTA_app_state_nvs_handle, OTA_STATE_NVS_KEY, &OTA_state);
    switch (err) {
        case ESP_OK:
            ESP_LOGI("OTA_NVS","App state recovered");
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI("OTA_NVS","The value is not initialized yet!\n");
            nvs_set_i16(OTA_app_state_nvs_handle, "OTA_app_state", 0);
            break;
        default :
            printf("Error (%s) reading!\n", esp_err_to_name(err));
    }
    return OTA_state;
};

void OTA_init(){
    // Initialize NVS.
    err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    err = nvs_open(OTA_STATE_NVS_KEY, NVS_READWRITE, &OTA_app_state_nvs_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle %s!\n", esp_err_to_name(err), OTA_STATE_NVS_KEY);
    }

    ESP_LOGI("NVS","OTA_app_state = %i", OTA_state);

    //OTA_set_state(ESP_OTA_IMG_NEW);

    if(OTA_get_state() == ESP_OTA_IMG_PENDING_VERIFY)
    {
        ESP_LOGE("OTA", "Unexpected reset! Rolling back update...");

        update_partition = esp_ota_get_next_update_partition(NULL);

        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
            task_fatal_error();
        }

        OTA_set_state(ESP_OTA_IMG_VALID);

        esp_restart();
    }
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */

    ESP_LOGI(TAG, "Starting OTA example...");

    configured = esp_ota_get_boot_partition();
    running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    if(OTA_get_state() == ESP_OTA_IMG_NEW)
    {
        ESP_LOGI("OTA","New partition loaded, pending verify.");
        OTA_set_state(ESP_OTA_IMG_PENDING_VERIFY);
    }

};

bool OTA_check_latest_version(){
    char *current_version = malloc(VERSION_LENGTH);

    // Connect
    // Check Version

    if(strcmp(current_version, running_version) == 0){
        return true;
    } else
        return false;
};

void OTA_download_latest_version(){

    esp_ota_handle_t update_handle = 0 ;
    esp_http_client_config_t config = {
            .url = EXAMPLE_SERVER_URL,
            //.cert_pem = (char *)server_cert_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection");
        task_fatal_error();
    }
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        task_fatal_error();
    }
    esp_http_client_fetch_headers(client);

    update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }
    ESP_LOGI(TAG, "esp_ota_begin succeeded");

    int binary_file_length = 0;
    /*deal with all receive packet*/
    printf("Downloading Image...");
    while (1) {
        printf(".");
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG, "Error: SSL data read error");
            http_cleanup(client);
            task_fatal_error();
        } else if (data_read > 0) {
            err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
                http_cleanup(client);
                task_fatal_error();
            }
            binary_file_length += data_read;
            ESP_LOGD(TAG, "Written image length %d", binary_file_length);
        } else if (data_read == 0) {
            ESP_LOGI(TAG, "Connection closed, all data received");
            break;
        }
    }
    ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);

    if (esp_ota_end(update_handle) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed!");
        http_cleanup(client);
        task_fatal_error();
    }
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }
    OTA_set_state(ESP_OTA_IMG_NEW);
    ESP_LOGI(TAG, "Setting OTA state to ESP_OTA_IMG_NEW");

};

//------------------------------------------------/*/

void ota_example_task(){
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;

    ESP_LOGI(TAG, "Starting OTA example...");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    esp_http_client_config_t config = {
            .url = EXAMPLE_SERVER_URL,
            //.cert_pem = (char *)server_cert_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection");
        task_fatal_error();
    }
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        task_fatal_error();
    }
    esp_http_client_fetch_headers(client);

    update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }
    ESP_LOGI(TAG, "esp_ota_begin succeeded");

    int binary_file_length = 0;
    /*deal with all receive packet*/
    printf("Downloading Image...");
    while (1) {
        printf(".");
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG, "Error: SSL data read error");
            http_cleanup(client);
            task_fatal_error();
        } else if (data_read > 0) {
            err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
                http_cleanup(client);
                task_fatal_error();
            }
            binary_file_length += data_read;
            ESP_LOGD(TAG, "Written image length %d", binary_file_length);
        } else if (data_read == 0) {
            ESP_LOGI(TAG, "Connection closed, all data received");
            break;
        }
    }
    ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);

    if (esp_ota_end(update_handle) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed!");
        http_cleanup(client);
        task_fatal_error();
    }
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
}
