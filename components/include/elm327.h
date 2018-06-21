//
// Created by Ignacio Maldonado Aylwin on 5/29/18.
//

//TODO Fill in Documentation

/**
 * @file
 *
 * @brief ELM327 Main lib
 *
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

/**
 * @brief Main data struct for handling required information from CAN bus sensors and GPS
 */
typedef struct {
    uint8_t temp;  /*!< Motor Oil temperature*/
    uint8_t fuel;  /*!< Remaining Fuel in primary Tank*/
    uint8_t speed; /*!< Current Ground Speed*/
    uint8_t LONG[8];  /*!< Longitud*/
    uint8_t LAT[8];   /*!< Latitude*/
    uint8_t TIME[4];  /*!< GPS time*/
    uint8_t VIN[17];  /*!< VIN: Unique Vehicle Identification Number*/
    uint8_t fields;/*!< Byte with bitmapped available fields*/
} elm327_data_t;

/**
* @brief Enumeration for defining presence of new data
*/
typedef enum {
    TEMP_FIELD      = 0b10000000,
    FUEL_FIELD      = 0b01000000,
    SPEED_FIELD     = 0b00100000,
    LONG_FIELD      = 0b00010000,
    LAT_FIELD       = 0b00001000,
    TIME_FIELD      = 0b00000100,
    VIN_FIELD       = 0b00000010,
    MISC_FIELD      = 0b00000001,
    ALL_FIELDS      = 0b11111111,
} data_fields_t;

/**
 * @brief Initializer for UART connection, rxTask, parseTask and messaging handle structs
 *
 * @param[in] bt_handle : Pointer to BlueTooth connection handle
 *
 */
 void elm327_init();

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

/**
 * @brief Sets elm327_data_t fields to default values
 *
 * @returns pointer to elm327_data_t
 *
 */