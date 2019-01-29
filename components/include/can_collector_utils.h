/*
    Copyright Verbux Soluciones Informáticas Junio 2018
*/
/**
 * @file
 * @author Ignacio Maldonado
 * @brief Main functions for data collection. Tasks for Query of info, recieving and parsing that info
 * and routing the info to a wireless connection when available, and caching in a file when no connection
 * is available.
 */

#ifndef __CAN_COLLECTOR_UTILS_H__
#define __CAN_COLLECTOR_UTILS_H__

#include "parse_utils.h"
#include "stack_utils.h"
#include "elm327.h"
#include "L80GPS.h"
#include "SIM_utils.h"
/**
* @brief Container for VIN string.
*/
uint8_t VIN[17];
volatile bool is_vin;

/**
* @brief Data struct containing intertask messaging handles.
*/
struct param {
    QueueHandle_t rxQueue;         /*!< rxTask to parseTask queue*/
    QueueHandle_t OutQueue;        /*!< parseTask and cardTask to OutgoingTask Queue*/
    QueueHandle_t storeQueue;      /*!< parseTask to cardTask Queue*/
} msgQueues;

/**
 * @brief Task for requesting data from ELM327 chip.
 *
 * @param queueStruct : Pointer to intertask messaging handle struct
 */
void collector_query_task(void *queueStruct);

/**
 * @brief Task for recieving data from ELM327 chip.
 *
 * @param queueStruct : Pointer to intertask messaging handle struct
 */
void collector_elm_rx_task(void *queueStruct);

/**
 * @brief Task for recieving data from GPS.
 *
 * @param queueStruct : Pointer to intertask messaging handle struct
 */
void collector_gps_rx_task(void *queueStruct);

/**
 * @brief Task for parsing and assembling data struct from sensor and GPS data.
 *
 * @param queueStruct : Pointer to intertask messaging handle struct.
 */
void collector_parse_task(void *queueStruct);

/**
 * @brief Task for storing data when connection fails, and requeueing data when connection.
 * is re-established
 *
 * @param queueStruct : Pointer to intertask messaging handle struct.
 */
void collector_card_task(void *queueStruct);

/**
 * @brief Task for establishing connection to cellular network and queueing outgoing messages to
 * remote database.
 *
 * @param queueStruct : Pointer to intertask messaging handle struct.
 */
void collector_SIM_task(void *queueStruct);

/**
 * @brief Task for initializing all tasks and associated queues.
 *
 */
void collector_init(void);

#endif