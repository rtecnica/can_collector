
/**
 * @file
 * @author Ignacio Maldonado
 * @brief Utilities for parsing information from ELM327 chip.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/**
* @brief Char to hexadecimal correspondence
*/
typedef enum {
    HEX_CHAR_0      = 0x30,
    HEX_CHAR_1      = 0x31,
    HEX_CHAR_2      = 0x32,
    HEX_CHAR_3      = 0x33,
    HEX_CHAR_4      = 0x34,
    HEX_CHAR_5      = 0x35,
    HEX_CHAR_6      = 0x36,
    HEX_CHAR_7      = 0x37,
    HEX_CHAR_8      = 0x38,
    HEX_CHAR_9      = 0x39,
    HEX_CHAR_A      = 0x41,
    HEX_CHAR_B      = 0x42,
    HEX_CHAR_C      = 0x43,
    HEX_CHAR_D      = 0x44,
    HEX_CHAR_E      = 0x45,
    HEX_CHAR_F      = 0x46,
} hex_char_t;

/**
* @brief Enumeration of possible response message types.
*/
typedef enum {
    FUELTANK_MSG    = 0x012F,
    OILTEMP_MSG     = 0x015C,
    SPEED_MSG       = 0x010D,
    VIN_MSG         = 0x0902,
    UNKNOWN_MSG     = 0X0,
} can_msg_t;

/**
 * @brief Utility for converting ASCII Character into hexidecimal.
 *
 * @param ASCII byte
 *
 * @return Equivalent Integer Value i.e f("1A") = 26
 *
 *
 */
uint8_t parse_char_to_hex(uint8_t bite);

/**
 * @brief Utility for checking type of ELM327 Response Message.
 *
 * @param data : Buffer Holding message
 * @param len  : Length of buffer
 *
 * @return 1 for Fuel tank response, 2 for Motor Oil temp response, 3 for Speed response, 4 for VIN response, 0 otherwise and -1 if the message is too short.
 *
 *
 */
can_msg_t parse_check_msg_type(uint8_t *data, int len);

/**
 * @brief Utility for parsing VIN string from ELM327 response.
 *
 * @param msg : Pointer to string that contains ELM327 response
 * @param VIN_global : Pointer to VIN global variable container
 *
 */
void vin_parse(uint8_t *VIN_global, uint8_t *msg);

/**
 * @brief Utility for checking whether or not ELM327 response contains data.
 *
 * @param data : pointer to string that contains ELM327 response
 *
 * @return true if message contains data, false otherwise
 *
 */
bool parse_is_data(uint8_t *data);
