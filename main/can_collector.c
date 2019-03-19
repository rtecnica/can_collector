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
    OTA_init();

    SIM_init();

    collector_init();

    OTA_set_state(OTA_get_state());

    ESP_LOGI("NVS","OTA_STATE = %0x", OTA_get_state());

    //OTA_download_latest_version();

    //esp_restart();
}
