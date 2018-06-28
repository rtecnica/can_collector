
/**
 * @file
 * @author Ignacio Maldonado
 * @brief Utilities implementing stack data structure in a FAT filesystem for storing
 * data on an SD card, while outgoing connection is unavailable.
 *
 */

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
 * @brief Initializer for SD card, FAT filsystem and stackFile.
 */
void stack_init(void);

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
