/*
    Copyright Verbux Soluciones Informáticas Mayo 2018
*/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "../components/include/elm327.h"
#include "../components/include/can_collector_utils.h"

#include "time.h"
#include "sys/time.h"


volatile uint32_t ulIdleCycleCount = 0UL;

void vApplicationIdleHook( void ) {
    /* This hook function does nothing but increment a counter. */
    ulIdleCycleCount++;
}

void app_main() {

    esp_log_level_set("*",ESP_LOG_INFO);

    bt_init();

    elm327_init(&bt_handle);

}