/*
    Copyright Verbux Soluciones Informáticas Junio 2018
*/
/**
 * @file
 * @author Ignacio Maldonado
 * @brief ELM327 Main lib, contains methods for sending commands to
 * ELM327 through UART and data type struct for information handling
 *
 */

#ifndef __ELM327_H__
#define __ELM327_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_event_loop.h"

#include "driver/uart.h"
#include "soc/uart_struct.h"


#ifndef ELM_UART_NUM
#define ELM_UART_NUM UART_NUM_0
#endif

#ifndef ELM_TXD_PIN
#define ELM_TXD_PIN GPIO_NUM_33
#endif

#ifndef ELM_RXD_PIN
#define ELM_RXD_PIN GPIO_NUM_16
#endif

#ifndef ELM_RX_BUF_SIZE
#define  ELM_RX_BUF_SIZE 128
#endif

/**
 * @brief Main data struct for handling required information from CAN bus sensors and GPS
 */
typedef struct {
    uint8_t temp;  /*!< Motor Oil temperature*/
    uint8_t fuel;  /*!< Remaining Fuel in primary Tank*/
    uint8_t speed; /*!< Current Ground Speed*/
    uint8_t LONG[11];  /*!< Longitud*/
    uint8_t LAT[10];   /*!< Latitude*/
    uint8_t TIME[12];  /*!< GPS time & date*/
    uint8_t VIN[17];  /*!< VIN: Unique Vehicle Identification Number*/
    uint8_t fields;/*!< Byte with bitmapped available fields*/
} elm327_data_t;

/**
* @brief Enumeration for defining presence of new data
* MISC_FIELD must always be enabled for correct function of fileStack
*/
typedef enum {
    TEMP_FIELD      = 0b10000000, /*!< Set when new temperature data is available*/
    FUEL_FIELD      = 0b01000000, /*!< Set when new fuel tank level data is available*/
    SPEED_FIELD     = 0b00100000, /*!< Set when new vehicle speed data is available*/
    LONG_FIELD      = 0b00010000, /*!< Set when new longitude data is available*/
    LAT_FIELD       = 0b00001000, /*!< Set when new latitude data is available*/
    TIME_FIELD      = 0b00000100, /*!< Set when new current time data is available*/
    VIN_FIELD       = 0b00000010, /*!< Set when VIN data has been correctly parsed*/
    MISC_FIELD      = 0b00000001, /*!< Set by default for correct functioning of fileStack*/
    ALL_FIELDS      = 0b11111111, /*!< All fields set*/
} data_fields_t;

void elm327_new_data(elm327_data_t *data);

/**
 * @brief Utility function for sending arbitrary data to the ELM327 via the UART connection
 *
 * @param logName : Name of log to send debug info
 * @param data : Buffer for data to send
 * @param len : Length of buffer
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
 * @brief Inits UART and resets elm327
 *
 */
void elm327_init(void);

void elm327_print(elm327_data_t packet);

#endif