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
            //msg_id = esp_mqtt_client_subscribe(client, "esp32", 0);

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
            msg_id = esp_mqtt_client_publish(client, "esp32", "data", 0, 0, 0);
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
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://172.16.127.182:1883",
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
        }
        else {
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
        }
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
            if(parse_is_GPS((uint8_t *)(*buff))){
                ESP_LOGI("PARSE_TASK", "Message Type Received: GPS");
                parse_GPS((uint8_t *)(*buff),&packet);
                packet.fields = packet.fields | LAT_FIELD | LONG_FIELD | TIME_FIELD;
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

    stack_init();

    elm327_data_t message;

    int stackdepth = 0;

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

//============================================
static void https_get_task(void *pvParameters)
{
	if (!(xSemaphoreTake(http_mutex, TASK_SEMAPHORE_WAIT))) {
		ESP_LOGE(HTTPS_TAG, "*** ERROR: CANNOT GET MUTEX ***n");
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}

	char buf[512];
    char *buffer;
    int ret, flags, len, rlen=0, totlen=0;

	buffer = malloc(8192);
	if (!buffer) {
		xSemaphoreGive(http_mutex);
		ESP_LOGE(HTTPS_TAG, "*** ERROR allocating receive buffer ***");
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}
	
	mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_x509_crt cacert;
    mbedtls_ssl_config conf;
    mbedtls_net_context server_fd;

    mbedtls_ssl_init(&ssl);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    ESP_LOGI(HTTPS_TAG, "Seeding the random number generator");

    mbedtls_ssl_config_init(&conf);

    mbedtls_entropy_init(&entropy);
    if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    NULL, 0)) != 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
		xSemaphoreGive(http_mutex);
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
    }

    ESP_LOGI(HTTPS_TAG, "Loading the CA root certificate...");

    ret = mbedtls_x509_crt_parse(&cacert, server_root_cert_pem_start,
                                 server_root_cert_pem_end-server_root_cert_pem_start);

    if(ret < 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
		xSemaphoreGive(http_mutex);
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
    }

    ESP_LOGI(HTTPS_TAG, "Setting hostname for TLS session...");

    // Host name set here should match CN in server certificate
    if((ret = mbedtls_ssl_set_hostname(&ssl, WEB_SERVER)) != 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
		xSemaphoreGive(http_mutex);
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
    }

    ESP_LOGI(HTTPS_TAG, "Setting up the SSL/TLS structure...");

    if((ret = mbedtls_ssl_config_defaults(&conf,
                                          MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_STREAM,
                                          MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_config_defaults returned %d", ret);
        goto exit;
    }

    // MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
    //   a warning if CA verification fails but it will continue to connect.
    //   You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef CONFIG_MBEDTLS_DEBUG
    mbedtls_esp_enable_debug_log(&conf, 4);
#endif

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
        goto exit;
    }

    goto start;

    while(1) {
		if (!(xSemaphoreTake(http_mutex, TASK_SEMAPHORE_WAIT))) {
			ESP_LOGE(HTTPS_TAG, "===== ERROR: CANNOT GET MUTEX ===================================\n");
            vTaskDelay(30000 / portTICK_PERIOD_MS);
			continue;
		}
start:
        // ** We must be connected to Internet
        if (ppposInit() == 0) goto finished;

        ESP_LOGI(HTTPS_TAG, "===== HTTPS GET REQUEST =========================================\n");
		//netif_set_default(&ppp_netif);

        mbedtls_net_init(&server_fd);

        ESP_LOGI(HTTPS_TAG, "Connecting to %s:%s...", SSL_WEB_SERVER, SSL_WEB_PORT);

        if ((ret = mbedtls_net_connect(&server_fd, SSL_WEB_SERVER,
                                      SSL_WEB_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
        {
            ESP_LOGE(HTTPS_TAG, "mbedtls_net_connect returned -%x", -ret);
            goto exit;
        }

        ESP_LOGI(HTTPS_TAG, "Connected.");

        mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

        ESP_LOGI(HTTPS_TAG, "Performing the SSL/TLS handshake...");

        while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
        {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
                goto exit;
            }
        }

        ESP_LOGI(HTTPS_TAG, "Verifying peer X.509 certificate...");

        if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
        {
            // In real life, we probably want to close connection if ret != 0
            ESP_LOGW(HTTPS_TAG, "Failed to verify peer certificate!");
            bzero(buf, sizeof(buf));
            mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
            ESP_LOGW(HTTPS_TAG, "verification info: %s", buf);
        }
        else {
            ESP_LOGI(HTTPS_TAG, "Certificate verified.");
        }

        ESP_LOGI(HTTPS_TAG, "Writing HTTP request...");

        while((ret = mbedtls_ssl_write(&ssl, (const unsigned char *)SSL_REQUEST, strlen(SSL_REQUEST))) <= 0)
        {
            if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_write returned -0x%x", -ret);
                goto exit;
            }
        }

        len = ret;
        ESP_LOGI(HTTPS_TAG, "%d bytes written", len);
        ESP_LOGI(HTTPS_TAG, "Reading HTTP response...");

		rlen = 0;
		totlen = 0;
        do
        {
            len = sizeof(buf) - 1;
            bzero(buf, sizeof(buf));
            ret = mbedtls_ssl_read(&ssl, (unsigned char *)buf, len);

            if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
                continue;

            if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                ret = 0;
                break;
            }

            if(ret < 0)
            {
                ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_read returned -0x%x", -ret);
                break;
            }

            if(ret == 0)
            {
                ESP_LOGI(HTTPS_TAG, "connection closed");
                break;
            }

            len = ret;
            //ESP_LOGI(HTTPS_TAG, "%d bytes read", len);
			totlen += len;
			if ((rlen + len) < 8192) {
				memcpy(buffer+rlen, buf, len);
				rlen += len;
			}
        } while(1);

        mbedtls_ssl_close_notify(&ssl);

    exit:
        mbedtls_ssl_session_reset(&ssl);
        mbedtls_net_free(&server_fd);

        ESP_LOGI(HTTPS_TAG, "%d bytes read, %d in buffer", totlen, rlen);
        if(ret != 0)
        {
            mbedtls_strerror(ret, buf, 100);
            ESP_LOGE(HTTPS_TAG, "Last error was: -0x%x - %s", -ret, buf);
        }

        buffer[rlen] = '\0';
        char *json_ptr = strstr(buffer, "{\"given_cipher_suites\":");
        char *hdr_end_ptr = strstr(buffer, "\r\n\r\n");
		if (hdr_end_ptr) {
			*hdr_end_ptr = '\0';
			printf("Header:\r\n-------\r\n%s\r\n-------\r\n", buffer);
		}
		if (json_ptr) {
			ESP_LOGI(HTTPS_TAG, "JSON data received.");
            //printf("JSON: [\n%s]\n", json_ptr);
            //ToDo: Check json in latest esp-idf
			cJSON *root = cJSON_Parse(json_ptr);
			ESP_LOGI(HTTPS_TAG, "JSON string parsed.");
			if (root) {
				ESP_LOGI(HTTPS_TAG, "parsing JSON data:");
                level = 1;
				parse_object(root);
				cJSON_Delete(root);
			}
		}

		// We can disconnect from Internet now and turn off RF to save power
		//ppposDisconnect(0, 1);

finished:
        ESP_LOGI(HTTPS_TAG, "Waiting %d sec...", EXAMPLE_TASK_PAUSE);
        ESP_LOGI(HTTPS_TAG, "=================================================================\n\n");
		xSemaphoreGive(http_mutex);
        for(int countdown = EXAMPLE_TASK_PAUSE; countdown >= 0; countdown--) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

//===========================================
static void http_get_task(void *pvParameters)
{
	if (!(xSemaphoreTake(http_mutex, TASK_SEMAPHORE_WAIT))) {
		ESP_LOGE(HTTP_TAG, "*** ERROR: CANNOT GET MUTEX ***n");
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}

	const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[128];
    char *buffer;
    int rlen=0, totlen=0;

	buffer = malloc(2048);
	if (!buffer) {
		ESP_LOGE(HTTPS_TAG, "*** ERROR allocating receive buffer ***");
		xSemaphoreGive(http_mutex);
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}

	goto start;

    while(1) {
        if (!(xSemaphoreTake(http_mutex, TASK_SEMAPHORE_WAIT))) {
			ESP_LOGE(HTTP_TAG, "===== ERROR: CANNOT GET MUTEX ==================================\n");
            vTaskDelay(30000 / portTICK_PERIOD_MS);
			continue;
		}
start:
        // ** We must be connected to Internet
        if (ppposInit() == 0) goto finished;

		ESP_LOGI(HTTP_TAG, "===== HTTP GET REQUEST =========================================\n");

		//netif_set_default(&ppp_netif);
        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(HTTP_TAG, "DNS lookup failed err=%d res=%p", err, res);
            xSemaphoreGive(http_mutex);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(HTTP_TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(HTTP_TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            xSemaphoreGive(http_mutex);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(HTTP_TAG, "... allocated socket\r\n");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(HTTP_TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            xSemaphoreGive(http_mutex);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(HTTP_TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(HTTP_TAG, "... socket send failed");
            close(s);
            xSemaphoreGive(http_mutex);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(HTTP_TAG, "... socket send success");
        ESP_LOGI(HTTP_TAG, "... reading HTTP response...");

        memset(buffer, 0, 2048);
        /* Read HTTP response */
        int opt = 500;
        int first_block = 1;
        char *cont_len = NULL;
        int clen = 0;
        rlen = 0;
        totlen = 0;
        char *hdr_end_ptr = NULL;
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            totlen += r;
            if ((rlen + r) < 2048) {
                memcpy(buffer+rlen, recv_buf, r);
                rlen += r;
                buffer[rlen] = '\0';
                if (clen == 0) {
                    cont_len = strstr(buffer, "Content-Length: ");
                    if (cont_len) {
                        cont_len += 16;
                        if (strstr(cont_len, "\r\n")) {
                            char slen[16] = {0};
                            memcpy(slen, cont_len, strstr(cont_len, "\r\n")-cont_len);
                            clen = atoi(slen);
                        }
                    }
                }
                if (hdr_end_ptr == NULL) {
                    hdr_end_ptr = strstr(buffer, "\r\n\r\n");
                    if (hdr_end_ptr) {
                        *hdr_end_ptr = '\0';
                        hdr_end_ptr += 4;
                    }
                }
                else if (clen > 0) {
                    if (strlen(hdr_end_ptr) >= clen) break; 
                }
            }
            if (first_block) {
                lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &opt, sizeof(int));
                first_block = 0;
            }
        } while(r > 0);

		if (hdr_end_ptr) {
			printf("Header:\r\n-------\r\n%s\r\n-------\r\n", buffer);
			printf("Data:\r\n-----\r\n%s\r\n-----\r\n", hdr_end_ptr);
		}
        ESP_LOGI(HTTP_TAG, "... done reading from socket. %d bytes read, %d in buffer, errno=%d\r\n", totlen, rlen, errno);
        close(s);

        // We can disconnect from Internet now and turn off RF to save power
		//ppposDisconnect(0, 1);

finished:
        ESP_LOGI(HTTP_TAG, "Waiting %d sec...", EXAMPLE_TASK_PAUSE);
        ESP_LOGI(HTTP_TAG, "================================================================\n\n");
		xSemaphoreGive(http_mutex);
        for(int countdown = EXAMPLE_TASK_PAUSE; countdown >= 0; countdown--) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

void collector_SIM_task(void *queueStruct){
    http_mutex = xSemaphoreCreateMutex();

    ESP_LOGI("COLLECTOR_INIT", "SIM Task creation successful");

    if (ppposInit() == 0) {
        ESP_LOGE("PPPoS EXAMPLE", "ERROR: GSM not initialized, HALTED");
        while (1) {
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
    }
    ESP_LOGI("COLLECTOR_INIT", "ppposInit() exitoso");

    // ==== Get time from NTP server =====
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;

    time(&now);
    localtime_r(&now, &timeinfo);

    while (1) {
        printf("\r\n");
        ESP_LOGI(TIME_TAG,"OBTAINING TIME");
        ESP_LOGI(TIME_TAG, "Initializing SNTP");
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "south-america.pool.ntp.org");
        sntp_init();
        ESP_LOGI(TIME_TAG,"SNTP INITIALIZED");

        // wait for time to be set
        now = 0;
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
        }
        sntp_stop();
        break;
    }
 	// ==== Create PPPoS tasks ====
    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
    xTaskCreate(&https_get_task, "https_get_task", 16384, NULL, 4, NULL);

    while(1)
	{
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
   vTaskDelete(NULL);
}

// Inicializa el módulo UART #0 que está conectalo a la interfase USB-UART
void collector_init(void) {


    //elm327_init();
    //GPS_init();

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
    //xTaskCreate(collector_parse_task, "collector_parse_task", 1024 * 2, (void *)&msgQueues, configMAX_PRIORITIES - 2, NULL);
    //xTaskCreate(collector_card_task, "collector_card_task", 1024 * 2, (void *)&msgQueues, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(collector_SIM_task, "collector_SIM_task", 1024 * 2, (void *)&msgQueues, configMAX_PRIORITIES, NULL);
    mqtt_app_start(msgQueues.OutQueue);
    //xTaskCreate(collector_query_task, "collector_query_task", 2 * 1024, NULL, configMAX_PRIORITIES - 3, NULL);

}