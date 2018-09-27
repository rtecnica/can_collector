/*
    Copyright Verbux Soluciones Informáticas Junio 2018
*/
/**
 * @file
 * @uthor Ignacio Maldonado Aylwin
 * @brief
 *
 */

#ifndef __L80GPS_H__
#define __L80GPS_H__

#ifndef GPS_UART_NUM
//#define GPS_UART_NUM UART_NUM_2
#define GPS_UART_NUM UART_NUM_1
#endif

#ifndef GPS_TXD_PIN
#define  GPS_TXD_PIN GPIO_NUM_32
#endif

#ifndef GPS_RXD_PIN
#define  GPS_RXD_PIN GPIO_NUM_36
#endif

#ifndef GPS_RX_BUF_SIZE
#define  GPS_RX_BUF_SIZE 128
#endif

/**
 * @brief Initializes UART connected to TX and RX pins defined in header.
 *
 *
 */
void GPS_init(void);





#endif //CAN_COLLECTOR_L80GPS_H
