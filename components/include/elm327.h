//
// Created by Ignacio Maldonado Aylwin on 5/29/18.
//

//TODO Fill in Documentation

/**
 * @file elm327.h
 * @brief ELM327 Main lib
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "driver/uart.h"
#include "soc/uart_struct.h"

#include "esp_spp_api.h"

#define TXD_PIN GPIO_NUM_4
#define RXD_PIN GPIO_NUM_36

/**
 * @brief Main data struct for handling required information from CAN bus sensors and GPS
 */
 typedef struct {
     char VIN[17];  /*!< VIN: Unique Vehicle Identification Number*/
     char temp;     /*!< Motor Oil temperature*/
     char fuel;     /*!< Remaining Fuel in primary Tank*/
     char speed;    /*!< Current Ground Speed*/
     char LONG[8];  /*!< Longitud*/
     char LAT[8];   /*!< Latitude*/
     char TIME[4];  /*!< GPS time*/
 } elm327_data_t;

/**
* @brief Data struct containing intertask messaging handles
*/
struct param {
    uint32_t *out_bt_handle;       /*!< BlueTooth connection handle for debug*/
    QueueHandle_t rxQueue;         /*!< rxTask to parseTask queue*/
    QueueHandle_t Outgoing_Queue;  /*!< parseTask to OutgoingTask Queue*/
} vParams;

/**
 * @brief Task for recieving data from ELM327 chip.
 *
 * @param[in] pvParameters : Pointer to intertask messaging handle struct
 */
void elm327_rx_task(void *pvParameters);

/**
 * @brief Task for parsing and assembling data struct from sensor and GPS data
 *
 * @param[in] pvParameters : Pointer to intertask messaging handle struct
 */
void elm327_parse_task(void *pvParameters);

/**
 * @brief Initializer for UART connection, rxTask, parseTask and messaging handle structs
 *
 * @param[in] bt_handle : Pointer to BlueTooth connection handle
 *
 */
 void elm327_init(uint32_t *bt_handle);

/**
 * @brief Utility function for sendind arbitrary data to the ELM327 via the UART connection
 *
 * @param[in] logName : Name of log to send debug info
 * @param[in] data : Buffer for data to send
 * @param[in] len : Length of buffer
 *
 * @returns True if sent appropiate amount of bytes, False otherwise
 */
bool elm327_sendData(const char* logName, unsigned char* data, const int len);

/**
 * @brief Send reset AT command to ELM327 i.e "AT Z"
 *
 * @returns True if sent appropiate amount of bytes, False otherwise
 */
bool elm327_reset(void);

/**
 * @brief Set ELM327 protocol via AT command i.e "AT TP 6"
 *
 * @returns True if sent appropiate amount of bytes, False otherwise
 *
 */
bool elm327_setCAN(void);

/**
 * @brief Request Motor Oil Temperature info from CAN bus
 *
 * @returns True if sent appropiate amount of bytes, False otherwise
 *
 */
bool elm327_query_oiltemp(void);

/**
 * @brief Request Fuel Tank Level info from CAN bus
 *
 * @returns True if sent appropiate amount of bytes, False otherwise
 *
 */
bool elm327_query_fueltank(void);

/**
 * @brief Request Vehicle Speed info from CAN bus
 *
 * @returns True if sent appropiate amount of bytes, False otherwise
 *
 */
bool elm327_query_speed(void);

/**
 * @brief Request Unique Vehicle Identification Number info from CAN bus
 *
 * @returns True if sent appropiate amount of bytes, False otherwise
 *
 */
bool elm327_query_VIN(void);

/**
 * @brief Request Time and Position from GPS
 *
 * @returns True if sent appropiate amount of bytes, False otherwise
 *
 */
bool elm327_query_GPS(void);

