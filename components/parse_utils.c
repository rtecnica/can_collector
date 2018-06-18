//
// Created by Ignacio Maldonado Aylwin on 6/7/18.
//

/**
 * @file
 */

#include "freertos/FreeRTOS.h"
#include "freertos/portable.h"

#include "include/parse_utils.h"


uint8_t parse_char_to_hex(uint8_t bite){
    switch ((hex_char_t)bite) {
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

can_msg_t parse_check_msg_type(uint8_t *data, int len){
    if(len>4) {
        switch (((((uint32_t) parse_char_to_hex(data[0])) << 12) + (((uint32_t) parse_char_to_hex(data[1])) << 8) +
                 (((uint32_t) parse_char_to_hex(data[2])) << 4) + (((uint32_t) parse_char_to_hex(data[3]))))) {
            case FUELTANK_MSG:
                return FUELTANK_MSG;
            case OILTEMP_MSG:
                return OILTEMP_MSG;
            case SPEED_MSG:
                return SPEED_MSG;
            case VIN_MSG:
                return VIN_MSG;
            default:
                return UNKNOWN_MSG;
        }
    }
    else
        return -1;
}

void vin_parse(uint8_t *VIN_global, uint8_t *msg){
    uint8_t *tmp = (uint8_t *)pvPortMalloc(17);
    tmp[0] = (((parse_char_to_hex((msg)[22]))<<4) + (parse_char_to_hex((msg)[23])));
    tmp[1] = (((parse_char_to_hex((msg)[25]))<<4) + (parse_char_to_hex((msg)[26])));
    tmp[2] = (((parse_char_to_hex((msg)[28]))<<4) + (parse_char_to_hex((msg)[29])));
    tmp[3] = (((parse_char_to_hex((msg)[35]))<<4) + (parse_char_to_hex((msg)[36])));
    tmp[4] = (((parse_char_to_hex((msg)[38]))<<4) + (parse_char_to_hex((msg)[39])));
    tmp[5] = (((parse_char_to_hex((msg)[41]))<<4) + (parse_char_to_hex((msg)[42])));
    tmp[6] = (((parse_char_to_hex((msg)[44]))<<4) + (parse_char_to_hex((msg)[45])));
    tmp[7] = (((parse_char_to_hex((msg)[47]))<<4) + (parse_char_to_hex((msg)[48])));
    tmp[8] = (((parse_char_to_hex((msg)[50]))<<4) + (parse_char_to_hex((msg)[51])));
    tmp[9] = (((parse_char_to_hex((msg)[53]))<<4) + (parse_char_to_hex((msg)[54])));
    tmp[10] = (((parse_char_to_hex((msg)[60]))<<4) + (parse_char_to_hex((msg)[61])));
    tmp[11] = (((parse_char_to_hex((msg)[63]))<<4) + (parse_char_to_hex((msg)[64])));
    tmp[12] = (((parse_char_to_hex((msg)[66]))<<4) + (parse_char_to_hex((msg)[67])));
    tmp[13] = (((parse_char_to_hex((msg)[69]))<<4) + (parse_char_to_hex((msg)[70])));
    tmp[14] = (((parse_char_to_hex((msg)[72]))<<4) + (parse_char_to_hex((msg)[73])));
    tmp[15] = (((parse_char_to_hex((msg)[75]))<<4) + (parse_char_to_hex((msg)[76])));
    tmp[16] = (((parse_char_to_hex((msg)[78]))<<4) + (parse_char_to_hex((msg)[79])));
    memcpy(VIN_global,tmp, 17);
    vPortFree(tmp);
}

bool parse_is_data(uint8_t *data){
    uint16_t nodata = 0x4e4f;
    uint16_t is = (((uint16_t)(data[5]))<<(8)) + ((uint16_t)(data[6]));
    return (is^nodata);
}