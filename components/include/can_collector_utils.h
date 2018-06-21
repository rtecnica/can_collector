//
// Created by Ignacio Maldonado Aylwin on 6/5/18.
//
/**
 * @file
 *
 * @brief Utility functions for testing setup, mainly initializing and handling bluetooth connectivity for debugging.
 */

#include "parse_utils.h"
#include "stack_utils.h"
#include "libGSM.h"

uint8_t VIN[17];

/**
* @brief Data struct containing intertask messaging handles
*/
struct param {
    QueueHandle_t rxQueue;         /*!< rxTask to parseTask queue*/
    QueueHandle_t OutQueue;        /*!< parseTask to OutgoingTask Queue*/
    QueueHandle_t storeQueue;      /*!< parseTask to OutgoingTask Queue*/
} vParams;

/**
 * @brief Task for requesting data from ELM327 chip.
 *
 * @param[in] pvParameters : Pointer to intertask messaging handle struct
 */
void collector_queryTask(void *pvParameters);

/**
 * @brief Task for recieving data from ELM327 chip.
 *
 * @param[in] pvParameters : Pointer to intertask messaging handle struct
 */
void collector_rx_task(void *pvParameters);

/**
 * @brief Task for parsing and assembling data struct from sensor and GPS data
 *
 * @param[in] pvParameters : Pointer to intertask messaging handle struct
 */
void collector_parse_task(void *pvParameters);

/**
 * @brief
 *
 * @param
 */
void collector_card_task(void *pvParameters);

/**
 * @brief
 *
 * @param
 */
void collector_SIM_task(void *pvParameters);

/**
 * @brief
 *
 * @param
 */
void collector_init(void);