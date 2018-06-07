//
// Created by Ignacio Maldonado Aylwin on 5/29/18.
//

//TODO Fill in Documentation

/**
 *  A test class. A more elaborate class description.
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
 * @brief
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
 * @brief
 *
 * @param
 *
 * @return
 *
 *
 */
void elm327_rx_task(void *pvParameters);

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 *
 */
void elm327_parse_task(void *pvParameters);

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 *
 */
 void elm327_init(uint32_t *bt_handle);

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 *
 */
bool elm327_sendData(const char* logName, unsigned char* data, const int len);

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 *
 */
bool elm327_reset(void);

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 *
 */

bool elm327_setCAN(void);
/**
 * @brief
 *
 * @param
 *
 * @return
 *
 *
 */
bool elm327_query_oiltemp(void);

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 *
 */
bool elm327_query_fueltank(void);

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 *
 */
bool elm327_query_speed(void);

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 *
 */
bool elm327_query_VIN(void);

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 *
 */
bool elm327_query_GPS(void);

