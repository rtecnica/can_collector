/**
 * @file
 * @author Ignacio Maldonado Aylwin
 *
 */

#ifndef CAN_COLLECTOR_SIM_UTILS_H
#define CAN_COLLECTOR_SIM_UTILS_H

#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mqtt_client.h"
#include "libGSM.h"

esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);

void mqtt_app_start(QueueHandle_t queue);

void parse_object(cJSON *item);

void initialize_sntp(void);

void SIM_init(void);

#endif //CAN_COLLECTOR_SIM_UTILS_H
