/*
    Copyright Verbux Soluciones Informáticas Mayo 2018
*/
/**
 * @file
 *
 *
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"

#include "../components/include/can_collector_utils.h"

#include "time.h"
#include "sys/time.h"


void app_main() {

    esp_log_level_set("*",ESP_LOG_INFO);

    bt_init();

    elm327_init(&bt_handle);

}