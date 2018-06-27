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
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

#include "driver/uart.h"
#include "soc/uart_struct.h"

#include "esp_spp_api.h"

#include "mqtt_client.h"

#define TXD_PIN GPIO_NUM_32
#define RXD_PIN GPIO_NUM_36

/**
 * @brief Main data struct for handling required information from CAN bus sensors and GPS
 */
 typedef struct {
     uint8_t fields;/*!< Byte with bitmapped available fields*/
     char VIN[17];  /*!< VIN: Unique Vehicle Identification Number*/
     uint8_t temp;  /*!< Motor Oil temperature*/
     uint8_t fuel;  /*!< Remaining Fuel in primary Tank*/
     uint8_t speed; /*!< Current Ground Speed*/
     char LONG[8];  /*!< Longitud*/
     char LAT[8];   /*!< Latitude*/
     char TIME[4];  /*!< GPS time*/
 } elm327_data_t;

/**
* @brief Enumeration for defining presence of new data
*/
typedef enum {
    MISC_FIELD      = 0b10000000,
    VIN_FIELD       = 0b01000000,
    TEMP_FIELD      = 0b00100000,
    FUEL_FIELD      = 0b00010000,
    SPEED_FIELD     = 0b00001000,
    LONG_FIELD      = 0b00000100,
    LAT_FIELD       = 0b00000010,
    TIME_FIELD      = 0b00000001,
    ALL_FIELDS      = 0b11111111,
} data_fields_t;

/**
* @brief Data struct containing intertask messaging handles
*/
struct param {
    uint32_t *out_bt_handle;       /*!< BlueTooth connection handle for debug*/
    QueueHandle_t rxQueue;         /*!< rxTask to parseTask queue*/
    QueueHandle_t OutQueue;        /*!< parseTask to OutgoingTask Queue*/
    QueueHandle_t storeQueue;      /*!< parseTask to OutgoingTask Queue*/
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

/**
 * @brief Sets elm327_data_t fields to default values
 *
 * @returns pointer to elm327_data_t
 *
 */
void elm327_new_data(elm327_data_t *data);
