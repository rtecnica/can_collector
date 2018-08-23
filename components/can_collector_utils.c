/*
    Copyright Verbux Soluciones Informáticas Junio 2018
*/
/**
 * @file
 * @author Ignacio Maldonado Aylwin
 *
 */
#include "include/can_collector_utils.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/semphr.h"

#include "driver/uart.h"

#include "netif/ppp/pppos.h"
#include "netif/ppp/ppp.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/pppapi.h"

#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#include <esp_event.h>
#include <esp_wifi.h>
#include <../include/elm327.h>

#include "apps/sntp/sntp.h"
#include "cJSON.h"

#include "../build/include/sdkconfig.h"


#define MESSAGE_QUEUE_LENGTH 5

//static const char *TIME_TAG = "[SNTP]";

static const int RX_BUF_SIZE = 128;
volatile uint32_t ulIdleCycleCount = 0UL;

static const char *TAG = "CAN_COLLECTOR_UTILS";

#define EXAMPLE_TASK_PAUSE	300		// pause between task runs in seconds
#define TASK_SEMAPHORE_WAIT 140000	// time to wait for mutex in miliseconds

QueueHandle_t http_mutex;

static const char *TIME_TAG = "[SNTP]";
static const char *HTTP_TAG = "[HTTP]";
static const char *HTTPS_TAG = "[HTTPS]";

// ===============================================================================================
// ==== Http/Https get requests ==================================================================
// ===============================================================================================

// Constants that aren't configurable in menuconfig
#define WEB_SERVER "loboris.eu"
#define WEB_PORT 80
#define WEB_URL "http://loboris.eu/ESP32/info.txt"
#define WEB_URL1 "http://loboris.eu"

#define SSL_WEB_SERVER "www.howsmyssl.com"
#define SSL_WEB_URL "https://www.howsmyssl.com/a/check"
#define SSL_WEB_PORT "443"

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

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
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

static void mqtt_app_start(QueueHandle_t queue)
{
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

void vApplicationIdleHook( void ) {
    /* This hook function does nothing but increment a counter. */
    ulIdleCycleCount++;
}

void collector_query_task(void *queueStruct){
    ESP_LOGI("COLLECTOR_INIT", "Query Task creation successful");
    elm327_query_VIN();
    vTaskDelay(3000/portTICK_PERIOD_MS);

    for(;;) {
        elm327_query_fueltank();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        elm327_query_oiltemp();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        elm327_query_speed();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        ESP_LOGI("HOUSEKEEPING", "Idle task count: %i",ulIdleCycleCount);
        ESP_LOGI("HOUSEKEEPING", "Available Heap Size: %i bytes",esp_get_free_heap_size());
        ulIdleCycleCount = 0UL;
    }
    vTaskDelete(NULL);
}

//Proceso de monitoreo de interfase UART
void collector_rx_task(void *queueStruct) {
    ESP_LOGI("COLLECTOR_INIT", "RX Task creation successful");

    uint8_t* data;
    for(;;) {

        data = (uint8_t*) pvPortMalloc(ELM_RX_BUF_SIZE+1);
        while(data == NULL){
            ESP_LOGI("RX_TASK","Waiting for available heap space...");
            vTaskDelay(100/portTICK_PERIOD_MS);
            data = (uint8_t*) pvPortMalloc(ELM_RX_BUF_SIZE+1);
        }

        int rxBytes = uart_read_bytes(ELM_UART_NUM, data, ELM_RX_BUF_SIZE, 100 / portTICK_RATE_MS);
        if (rxBytes > 0) {

            data[rxBytes] = 0;
            //ESP_LOGI("RX_TASK", "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOGI("RX_TASK", "%s", data);
            //ESP_LOG_BUFFER_HEXDUMP("RX_TASK_HEXDUMP", data, rxBytes, ESP_LOG_INFO);

            //Send through queue to data processing Task
            xQueueSend(((struct param *)queueStruct)->rxQueue,(void *)(&data),0);
            // data will be vPortFreed by receiving function
        } else {
                vPortFree(data);

        }
        /*else {
            rxBytes = uart_read_bytes(GPS_UART_NUM, data, GPS_RX_BUF_SIZE, 100 / portTICK_RATE_MS);
            if (rxBytes > 0) {

                data[rxBytes] = 0;
                //ESP_LOGI("RX_TASK", "Read %d bytes: '%s'", rxBytes, data);
                ESP_LOGI("RX_TASK", "%s", data);
                //ESP_LOG_BUFFER_HEXDUMP("RX_TASK_HEXDUMP", data, rxBytes, ESP_LOG_INFO);

                //Send through queue to data processing Task
                xQueueSend(((struct param *) queueStruct)->rxQueue, (void *) (&data), 0);
                // data will be vPortFreed by receiving function
            } else {
                vPortFree(data);
            }
        }*/
        //vPortFree(data);
        //ESP_LOGI("HOUSEKEEPING", "Available Heap Size: %i bytes",esp_get_free_heap_size());
    }
    //vPortFree(data);
    vTaskDelete(NULL);
}

static void parse_object(cJSON *item)
{
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

void collector_parse_task(void *queueStruct){
    ESP_LOGI("COLLECTOR_INIT", "Parse Task creation successful");

    void **buff = pvPortMalloc(sizeof(void *));
    BaseType_t xStatus;
    can_msg_t msg_type;

    elm327_data_t packet;
    packet.fields = MISC_FIELD;

    for(;;){
        xStatus = xQueueReceive(((struct param *)queueStruct)->rxQueue, buff, 200 / portTICK_PERIOD_MS);
        if(xStatus == pdPASS) {
            elm327_new_data(&packet);
            if(parse_is_GPS((uint8_t *)(*buff))){
                ESP_LOGI("PARSE_TASK", "Message Type Received: GPS");
                parse_GPS((uint8_t *)(*buff),&packet);
            }
            else if(parse_is_data((uint8_t *)(*buff))){
                msg_type = parse_check_msg_type((uint8_t *)*buff,6);
                ESP_LOGI("PARSE_TASK", "Message Type Received: %04x", msg_type);
                uint8_t var = parse_msg(*buff);
                switch(msg_type){
                    case FUELTANK_MSG:
                        packet.fuel = var;
                        packet.fields = packet.fields | FUEL_FIELD;
                        ESP_LOGI("FUELTANK_MSG","Fuel Level = %02x",var);
                        break;
                    case OILTEMP_MSG:
                        packet.temp = var;
                        packet.fields = packet.fields | TEMP_FIELD;
                        ESP_LOGI("OILTEMP_MSG","Oil Temp = %02x",var);
                        break;
                    case SPEED_MSG:
                        packet.speed = var;
                        packet.fields = packet.fields | SPEED_FIELD;
                        ESP_LOGI("SPEED_MSG","Speed = %02x",var);
                        break;
                    case VIN_MSG:
                        parse_vin(VIN,*buff);
                        memcpy(packet.VIN,VIN,17);
                        packet.fields = packet.fields | VIN_FIELD;
                        ESP_LOGI("VIN_MSG","VIN = %s",VIN);
                        break;
                    case UNKNOWN_MSG:
                        break;
                }
            }
            else{
                ESP_LOGI("PARSE_TASK", "No Data!");
            }
            vPortFree(*buff);

            if((packet.fields & (VIN_FIELD | SPEED_FIELD | FUEL_FIELD | TEMP_FIELD | MISC_FIELD)) == (VIN_FIELD | SPEED_FIELD | FUEL_FIELD | TEMP_FIELD | MISC_FIELD) ){
                ESP_LOGI("PARSE_TASK","Packet Ready for Sending");

                if(xQueueSend(((struct param *)queueStruct)->OutQueue,&packet,0) == pdPASS){
                    ESP_LOGI("PARSE_TASK","Packet sent to Outgoing Queue");
                }
                else{
                    if(xQueueSend(((struct param *)queueStruct)->storeQueue,&packet,0) == pdPASS){
                        ESP_LOGI("PARSE_TASK","Packet sent to Storage Queue");
                    }
                    else{
                        ESP_LOGI("PARSE_TASK","Storage Queue Full!");
                    }
                }
                packet.fields = VIN_FIELD | MISC_FIELD;
            }
        }
    }
    vTaskDelete(NULL);
}

void collector_card_task(void *queueStruct){
    ESP_LOGI("COLLECTOR_INIT", "Card Task creation successful");

    elm327_data_t message;

    int stackdepth = fStack_depth;

    for(;;) {
        if (uxQueueMessagesWaiting(((struct param *) queueStruct)->OutQueue) < MESSAGE_QUEUE_LENGTH && stackdepth > 0) {

            fStack_pop(&message);
            stackdepth--;
            ESP_LOGI("SD_TASK", "Message popped from stack");
            xQueueSend(((struct param *) queueStruct)->OutQueue, &message, 100/portTICK_PERIOD_MS);

        }

        if (xQueueReceive(((struct param *) queueStruct)->storeQueue, &message, 100/portTICK_PERIOD_MS) == pdPASS) {

            fStack_push(&message);
            stackdepth++;
            ESP_LOGI("SD_TASK", "Message pushed to stack. Stack Depth: %i",stackdepth);
        }
    }
    vTaskDelete(NULL);
}

static void initialize_sntp(void)
{
    ESP_LOGI(TIME_TAG,"OBTAINING TIME");
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    //sntp_setservername(0, "pool.ntp.org");
    sntp_setservername(0, "ntp.shoa.cl");
    sntp_init();
    ESP_LOGI(TIME_TAG,"SNTP INITIALIZED");
}

void collector_SIM_task(void *queueStruct){

    ESP_LOGI("COLLECTOR_INIT", "SIM Task creation successful");

    while (1) {
        if (ppposInit() == 0) {
            ESP_LOGE("PPPoS", "ERROR: GSM not initialized, HALTED");
            vTaskDelay(1000 / portTICK_RATE_MS);
        } else {
            break;
        }
    }
    //ESP_LOGI("COLLECTOR_INIT", "ppposInit() exitoso");

    // ==== Get time from NTP server =====
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    initialize_sntp();
    time(&now);
    localtime_r(&now, &timeinfo);

    while (1) {

        // wait for time to be set
        now = 0;
        //now = 1534165208;
        while ((timeinfo.tm_year < (2016 - 1900)) && (++retry < retry_count)) {
            ESP_LOGI(TIME_TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            time(&now);
            localtime_r(&now, &timeinfo);
            if (ppposStatus() != GSM_STATE_CONNECTED) break;
        }
        if (ppposStatus() != GSM_STATE_CONNECTED) {
            sntp_stop();
            ESP_LOGE(TIME_TAG, "Disconnected, waiting for reconnect");
            retry = 0;
            while (ppposStatus() != GSM_STATE_CONNECTED) {
                vTaskDelay(100 / portTICK_RATE_MS);
            }
            continue;
        }

        if (retry < retry_count) {
            ESP_LOGI(TIME_TAG, "TIME SET TO %s", asctime(&timeinfo));
            break;
        }
        else {
            ESP_LOGI(TIME_TAG, "ERROR OBTAINING TIME\n");
            //retry = 0;
            //sntp_stop();
            //sntp_init();
            //continue;
        }
        sntp_stop();
        //ppposDisconnect(0,0);
        //retry = 0;
        //ppposInit();
        //time(&now);
        //localtime_r(&now, &timeinfo);
        break;
    }
     // ==== Create PPPoS tasks ====
    mqtt_app_start(msgQueues.OutQueue);
    //xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
    //xTaskCreate(&https_get_task, "https_get_task", 16384, NULL, 4, NULL);

    while(1)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
   vTaskDelete(NULL);
}

// Inicializa el módulo UART #0 que está conectalo a la interfase USB-UART
void collector_init(void) {


    //elm327_init();
    GPS_init();
    stack_init();

    msgQueues.rxQueue = xQueueCreate(MESSAGE_QUEUE_LENGTH, sizeof(void *));
    if(msgQueues.rxQueue != (NULL)){
        ESP_LOGI("RX_QUEUE", "rxQueue creation successful");
    }

    msgQueues.OutQueue = xQueueCreate(MESSAGE_QUEUE_LENGTH, sizeof(elm327_data_t));
    if(msgQueues.OutQueue != (NULL)){
        ESP_LOGI("OUT_QUEUE", "OutQueue creation successful");
    }

    msgQueues.storeQueue = xQueueCreate(MESSAGE_QUEUE_LENGTH, sizeof(elm327_data_t));
    if(msgQueues.storeQueue != (NULL)){
        ESP_LOGI("STORE_QUEUE", "storeQueue creation successful");
    }

    //xTaskCreate(collector_rx_task, "collector_rx_task", 1024 * 2, (void *)&msgQueues, configMAX_PRIORITIES -1, NULL);
    xTaskCreate(collector_parse_task, "collector_parse_task", 1024 * 2, (void *)&msgQueues, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(collector_card_task, "collector_card_task", 1024 * 2, (void *)&msgQueues, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(collector_SIM_task, "collector_SIM_task", 1024 * 2, (void *)&msgQueues, configMAX_PRIORITIES, NULL);
    //xTaskCreate(collector_query_task, "collector_query_task", 2 * 1024, NULL, configMAX_PRIORITIES - 3, NULL);

}