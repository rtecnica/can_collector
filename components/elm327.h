//
// Created by Ignacio Maldonado Aylwin on 5/29/18.
//

//TODO Fill in Documentation

#define TXD_PIN GPIO_NUM_4
#define RXD_PIN GPIO_NUM_36

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
int elm327_sendData(const char* logName, unsigned char* data, const int len);