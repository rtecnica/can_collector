/*
    Copyright Verbux Soluciones Informáticas Junio 2018
*/
/**
 * @file
 * @author Ignacio Maldonado
 * @brief Utilities implementing stack data structure in a FAT filesystem for storing
 * data on an SD card, while outgoing connection is unavailable.
 *
 */

#ifndef __STACK_UTILS_H__
#define __STACK_UTILS_H__

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "stdio.h"
#include "elm327.h"

/**
 * @brief Filename to be used in sd card
 **/
#define STACK_FILENAME "/sdcard/stack"


/**
 * @brief Global Variable for storing stack depth
 **/
volatile int fStack_depth;

/**
 * @brief Pops an item from the fileStack.
 * @param data : pointer to elm327 datatype buffer to store popped item
 *
 */
void fStack_pop(elm327_data_t *data);

/**
 * @brief Pushes an item to the fileStack.
 * @param data : pointer to elm327 datatype item to be pushed to the fileStack
 *
 */
void fStack_push(elm327_data_t *data);

/**
 * @brief Initializer for SD card, FAT filsystem and stackFile.
 */
void stack_init(void);

#endif