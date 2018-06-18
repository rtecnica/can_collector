//
// Created by Ignacio Maldonado Aylwin on 6/14/18.
//

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "stdio.h"
#include "elm327.h"

void card_init(void);

void fStack_pop(FILE* file, elm327_data_t *data);

void fStack_push(FILE* file, elm327_data_t *data);
