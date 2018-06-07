//
// Created by Ignacio Maldonado Aylwin on 6/7/18.
//

/**
 * @file parse_utils.h
 * @brief Parsing Utilities
 */

#include <stdint.h>

/**
 * @brief Utility for converting ASCII Character into hexidecimal
 *
 * @param[in] ASCII byte
 *
 * @return Equivalent Integer Value i.e f("1A") = 26
 *
 *
 */
uint8_t parse_char_to_hex(uint8_t bite);

/**
 * @brief Utility for checking type of ELM327 Message
 *
 * @param[in] data : Buffer Holding message
 * @param[in] len  : Length of buffer
 *
 * @return 1 for Fuel tank response, 2 for Motor Oil temp response, 3 for Speed response, 4 for VIN response, 0 otherwise and -1 if the message is too short.
 *
 *
 */
int parse_check_msg_type(uint8_t *data, int len);

/**
 * @brief
 *
 * @param[in]
 *
 * @return
 *
 *
 */
