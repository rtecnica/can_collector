/**
 * @file
 * @author Ignacio Maldonado Aylwin
 *
 */
#include "SIM_utils.h"
#include "esp_log.h"
#include "apps/sntp/sntp.h"

QueueHandle_t http_mutex;

static const char *TIME_TAG = "[SNTP]";
static const char *HTTP_TAG = "[HTTP]";
static const char *HTTPS_TAG = "[HTTPS]";
static const char *TAG = "SIM_utils";

static int level = 0;

esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event){
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, MQTT_TOPIC, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}

void mqtt_app_start(QueueHandle_t queue){
    char *uri = (char *)pvPortMalloc(20);
    char *tmp = (char *)pvPortMalloc(5);
    strcpy(uri, "mqtt://");
    strcat(uri, MQTT_SERVER_URI);
    strcat(uri, ":");
    sprintf(tmp, "%d", MQTT_TCP_DEFAULT_PORT);
    strcat(uri, tmp);
    const esp_mqtt_client_config_t mqtt_cfg = {
            .uri = uri,
            .event_handle = mqtt_event_handler,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg, queue);
    esp_mqtt_client_start(client);
}

void initialize_sntp(void){
    ESP_LOGI(TIME_TAG,"OBTAINING TIME");
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    //sntp_setservername(0, "pool.ntp.org");
    sntp_setservername(0, "ntp.shoa.cl");
    sntp_init();
    ESP_LOGI(TIME_TAG,"SNTP INITIALIZED");
}

void SIM_init(void){
    while (1) {
        if (ppposInit() == 0) {
            ESP_LOGE("PPPoS", "ERROR: GSM not initialized, HALTED");
            vTaskDelay(1000 / portTICK_RATE_MS);
        } else {
            break;
        }
    }
}