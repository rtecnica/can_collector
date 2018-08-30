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

// Constants that aren't configurable in menuconfig
#define WEB_SERVER "loboris.eu"
#define WEB_URL "http://loboris.eu/ESP32/info.txt"
#define WEB_URL1 "http://loboris.eu"

#define SSL_WEB_SERVER "www.howsmyssl.com"
#define SSL_WEB_URL "https://www.howsmyssl.com/a/check"

static const char *REQUEST = "GET " WEB_URL " HTTP/1.1\r\n"
                             "Host: "WEB_SERVER"\r\n"
                             "User-Agent: esp-idf/1.0 esp32\r\n"
                             "\r\n";

static const char *REQUEST1 = "GET " WEB_URL1 " HTTP/1.1\r\n"
                              "Host: "WEB_SERVER"\r\n"
                              "User-Agent: esp-idf/1.0 esp32\r\n"
                              "\r\n";

static const char *SSL_REQUEST = "GET " SSL_WEB_URL " HTTP/1.1\r\n"
                                 "Host: "SSL_WEB_SERVER"\r\n"
                                 "User-Agent: esp-idf/1.0 esp32\r\n"
                                 "\r\n";

/* Root cert for howsmyssl.com, taken from server_root_cert.pem

   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null

   The CA root cert is the last cert given in the chain of certs.

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

static int level = 0;

esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event){
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            esp_mqtt_client_subscribe(client, MQTT_TOPIC, 0);

//            msg_id = esp_mqtt_client_subscribe(client, "esp32_cnavarro_1997_1482", 0);

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, "data", 0, 0, 0);
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
            //.uri = "mqtt://172.16.127.182:1883",
            .uri = uri,
            .event_handle = mqtt_event_handler,
            // .user_context = (void *)your_context
    };

/*    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://iot.eclipse.org",
        .event_handle = mqtt_event_handler,
        // .user_context = (void *)your_context
    };*/

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg, queue);
    esp_mqtt_client_start(client);
}

void parse_object(cJSON *item){
    ESP_LOGI(HTTPS_TAG, "JSON: level %d", level);
    cJSON *subitem=item->child;
    while (subitem)
    {
        printf("%s = ", subitem->string);
        if (subitem->type == cJSON_String) printf("%s\r\n", subitem->valuestring);
        else if (subitem->type == cJSON_Number) printf("%d\r\n", subitem->valueint);
        else if (subitem->type == cJSON_False) printf("False\r\n");
        else if (subitem->type == cJSON_True) printf("True\r\n");
        else if (subitem->type == cJSON_NULL) printf("NULL\r\n");
        else if (subitem->type == cJSON_Object) {
            printf("{Object}\r\n");
            // handle subitem
            if (subitem->child) {
                cJSON *subitemData = cJSON_GetObjectItem(subitem,subitem->string);
                if (subitemData) {
                    level++;
                    parse_object(subitemData);
                    level--;
                }
            }
        }
        else if (subitem->type == cJSON_Array) {
            int arr_count = cJSON_GetArraySize(subitem);
            printf("[Array] of %d items\r\n", arr_count);
            int n;
            for (n = 0; n < 3; n++) {
                // Get the JSON element and then get the values as before
                cJSON *arritem = cJSON_GetArrayItem(subitem, n);
                if ((arritem) && (arritem->valuestring)) printf("   %s\n", arritem->valuestring);
                else break;
            }
            if (arr_count > 3 ) printf("   + %d more...\r\n", arr_count-3);
        }
        else printf("[?]\r\n");

        subitem=subitem->next;
    }
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