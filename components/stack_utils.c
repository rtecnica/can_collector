/*
    Copyright Verbux Soluciones Informáticas Junio 2018
*/
/**
 * @file
 * @author Ignacio Maldonado Aylwin
 *
 */

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "include/stack_utils.h"

#define MSG_SIZE sizeof(elm327_data_t)

void fStackFindTop(FILE* file){
    fseek(file,-1,SEEK_END);
    char f = fgetc(file);
    while(f == 0x0){
        fseek(file,-2,SEEK_CUR);
        f = fgetc(file);
    }
}

void fStackFindDepth(FILE* file){
    int biteNUM = 0;
    fStackFindTop(file);

    fseek(file,-1,SEEK_END);
    char f = fgetc(file);
    while(f != '#'){
        fseek(file,-2,SEEK_CUR);
        f = fgetc(file);
        biteNUM++;
    }
    fStack_depth = biteNUM / MSG_SIZE;

    ESP_LOGI("STACK_UTILS", "Stack Size: %i bytes, Messages: %f", biteNUM, (double)biteNUM/ MSG_SIZE);

    if(((float)biteNUM / MSG_SIZE) != fStack_depth){
        ESP_LOGI("STACK_UTILS", "Broken Message!");
        int bytesToDelete = biteNUM - fStack_depth * MSG_SIZE;
        ESP_LOGI("STACK_UTILS", "Deleting %i bytes...", bytesToDelete);

        fStackFindTop(file);
        fputc(0x0,file);
        for(; bytesToDelete >= 0; bytesToDelete--){
            fseek(file,-2,SEEK_CUR);
            fputc(0x0,file);
        }

        fStackFindTop(file);
        biteNUM = 0;
        f = fgetc(file);
        while(f != '#'){
            biteNUM++;
            fseek(file,-2,SEEK_CUR);
            f = fgetc(file);
        }
        fStack_depth = biteNUM / MSG_SIZE;
        if(((float)biteNUM / MSG_SIZE) != fStack_depth) {
            ESP_LOGI("STACK_UTILS", "Repair Failed!");
        } else {
            ESP_LOGI("STACK_UTILS", "Repair Success!");
        }
    }
}

void fStack_pop(elm327_data_t *data){

    FILE *stack_file = fopen(STACK_FILENAME, "r+");

    if(fStack_depth > 0) {

        uint8_t *empty = (uint8_t *) calloc(MSG_SIZE, 1);

        fStackFindTop(stack_file);
        fseek(stack_file, -MSG_SIZE, SEEK_CUR);
        fread(data, MSG_SIZE, 1, stack_file);
        fseek(stack_file, -MSG_SIZE, SEEK_CUR);
        fwrite(empty, MSG_SIZE, 1, stack_file);
        fseek(stack_file, -MSG_SIZE, SEEK_CUR);

        fStack_depth--;
    }
    else{
        ESP_LOGE("SD_TASK","File Stack Empty!!");
    }

    fclose(stack_file);

}

void fStack_push(elm327_data_t *data){

    FILE *stack_file = fopen(STACK_FILENAME, "r+");

    fStackFindTop(stack_file);
    fwrite(data,MSG_SIZE,1,stack_file);
    fStack_depth++;

    fclose(stack_file);
}

void stack_init(void) {
    ESP_LOGI("SD_TASK", "Initializing SD card");
    ESP_LOGI("SD_TASK", "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // To use 1-line SD mode, uncomment the following line
    host.flags = SDMMC_HOST_FLAG_1BIT;

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
    // Internal pull-ups are not sufficient. However, enabling internal pull-ups
    // does make a difference some boards, so we do that here.
    gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode(13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed = true,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024
    };

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    sdmmc_card_t *card;

    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE("SD_TASK", "Failed to mount filesystem. "
                                "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE("SD_TASK", "Failed to initialize the card (%s). "
                                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
    }
    sdmmc_card_print_info(stdout, card);

    FILE* stack_file = fopen(STACK_FILENAME,"r+");

    if(stack_file == NULL) {
        stack_file = fopen(STACK_FILENAME, "w+");
        char f = '#';

        fwrite(&f, 1, 1, stack_file);
    }

    fStackFindDepth(stack_file);

    fclose(stack_file);
}