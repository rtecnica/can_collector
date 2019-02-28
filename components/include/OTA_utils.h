/*
    Copyright Verbux Soluciones Informáticas Enero 2019
*/
/**
 * @file
 * @author Ignacio Maldonado
 * @brief
 *
 */

#ifndef __CAN_COLLECTOR_OTA_UTILS_H__
#define __CAN_COLLECTOR_OTA_UTILS_H__

#include "esp_ota_ops.h"

int16_t OTA_state;

void ota_example_task();

void OTA_vars_init();

#endif //CAN_COLLECTOR_OTA_UTILS_H
