/*
* Copyright (c) 2016 Cesanta Software Limited
* All rights reserved
*/

#ifndef CS_SMARTJS_PLATFORMS_ESP8266_USER_ESP_UPDATER_H_
#define CS_SMARTJS_PLATFORMS_ESP8266_USER_ESP_UPDATER_H_

#include "v7/v7.h"

void init_updater(struct v7 *v7);
int finish_update();

uint8_t get_current_rom();
uint32_t get_fw_addr(uint8_t rom);
uint32_t get_fs_addr(uint8_t rom);
uint32_t get_fs_size(uint8_t rom);

#endif /* CS_SMARTJS_PLATFORMS_ESP8266_USER_ESP_UPDATER_H_ */
