/*
    Copyright Verbux Soluciones Informáticas Mayo 2018
*/
/**
 * @file
 * @author Ignacio Maldonado Aylwin
 *
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "../components/include/can_collector_utils.h"


void app_main() {

    esp_log_level_set("*",ESP_LOG_VERBOSE);

    collector_init();
}