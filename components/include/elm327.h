//
// Created by Ignacio Maldonado Aylwin on 5/29/18.
//

//TODO Fill in Documentation

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
     char VIN[17];  /*!< UART baud rate*/
     char temp;     /*!< UART baud rate*/
     char fuel;     /*!< UART baud rate*/
     char speed;    /*!< UART baud rate*/
     char LONG[8];  /*!< UART baud rate*/
     char LAT[8];   /*!< UART baud rate*/
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
