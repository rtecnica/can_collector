/*
    Copyright Verbux Soluciones Informáticas Mayo 2018
*/
/**
 * @file
 * @author Ignacio Maldonado Aylwin
 *
 */

#include "../components/include/can_collector_utils.h"

void app_main()
{
    OTA_vars_init();

    //SIM_init();

    //collector_init();

    //ota_example_task();

    OTA_set_state(OTA_get_state());

    ESP_LOGI("NVS","OTA_STATE = %i", OTA_get_state());

    esp_restart();
}
