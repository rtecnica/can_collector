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

    OTA_download_latest_version();

    esp_restart();
}
