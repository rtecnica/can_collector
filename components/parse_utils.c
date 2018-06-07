//
// Created by Ignacio Maldonado Aylwin on 6/7/18.
//

/**
 * @file
 */
#include "include/parse_utils.h"

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

typedef enum {
    FUELTANK_MSG    = 0x012F,
    OILTEMP_MSG     = 0x015C,
    SPEED_MSG       = 0x010D,
    VIN_MSG         = 0x0902,
} can_msg_t;

uint8_t parse_char_to_hex(uint8_t bite){
    hex_char_t val = (hex_char_t) bite;
    switch (val) {
        case HEX_CHAR_0:
            return 0x0;
        case HEX_CHAR_1:
            return 0x1;
        case HEX_CHAR_2:
            return 0x2;
        case HEX_CHAR_3:
            return 0x3;
        case HEX_CHAR_4:
            return 0x4;
        case HEX_CHAR_5:
            return 0x5;
        case HEX_CHAR_6:
            return 0x6;
        case HEX_CHAR_7:
            return 0x7;
        case HEX_CHAR_8:
            return 0x8;
        case HEX_CHAR_9:
            return 0x9;
        case HEX_CHAR_A:
            return 0xA;
        case HEX_CHAR_B:
            return 0xB;
        case HEX_CHAR_C:
            return 0xC;
        case HEX_CHAR_D:
            return 0xD;
        case HEX_CHAR_E:
            return 0xE;
        case HEX_CHAR_F:
            return 0xF;
        default:
            return 0x0;
    }
}

int parse_check_msg_type(uint8_t *data, int len){
    if(len>4) {
        switch (((((uint32_t) parse_char_to_hex(data[0])) << 12) + (((uint32_t) parse_char_to_hex(data[1])) << 8) +
                 (((uint32_t) parse_char_to_hex(data[2])) << 4) + (((uint32_t) parse_char_to_hex(data[3]))))) {
            case FUELTANK_MSG:
                return 1;
            case OILTEMP_MSG:
                return 2;
            case SPEED_MSG:
                return 3;
            case VIN_MSG:
                return 4;
            default:
                return 0;
        }
    }
    else
        return -1;
}