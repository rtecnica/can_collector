//
// Created by Ignacio Maldonado Aylwin on 6/14/18.
//

/**
 * @file
 *
 * @brief
 *
 */

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "stdio.h"
#include "elm327.h"

#define STACK_FILENAME "stckfile"

volatile int fStack_depth = 0;

void stack_init(void);

void fStack_pop(elm327_data_t *data);

void fStack_push(elm327_data_t *data);
