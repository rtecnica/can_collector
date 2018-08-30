/*
    Copyright Verbux Soluciones Informáticas Agosto 2018
*/
 /**
 * @file
 * @author Ignacio Maldonado Aylwin
 *
 */
#ifndef CAN_COLLECTOR_SIM_UTILS_H
#define CAN_COLLECTOR_SIM_UTILS_H

#include "apps/sntp/sntp.h"
#include "cJSON.h"
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
#include "mqtt_client.h"
#include "libGSM.h"
#include "netif/ppp/pppos.h"
#include "netif/ppp/ppp.h"

//static const char *TIME_TAG = "[SNTP]";



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

static void initialize_sntp(void);

static void parse_object(cJSON *item);

static void mqtt_app_start(QueueHandle_t queue);

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);

#endif //CAN_COLLECTOR_SIM_UTILS_H
