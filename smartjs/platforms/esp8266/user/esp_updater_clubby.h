/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef ESP_UPDATER_CLUBBY_H_INCLUDED
#define ESP_UPDATER_CLUBBY_H_INCLUDED

#ifndef DISABLE_OTA

#include "mongoose/mongoose.h"
#include "v7/v7.h"

#define FILE_UPDATE_PREF "imp_"

enum update_status {
  US_NOT_STARTED,
  US_INITED,
  US_WAITING_METADATA,
  US_GOT_METADATA,
  US_DOWNLOADING_FW,
  US_DOWNLOADING_FS,
  US_COMPLETED,
  US_ERROR,
  US_NOTHING_TODO
};

void init_updater_clubby(struct v7 *v7);

void update_start(const char *metadata_url);

enum update_status update_get_status(void);
int finish_update();

uint8_t get_current_rom();
uint32_t get_fw_addr(uint8_t rom);
uint32_t get_fs_addr(uint8_t rom);
uint32_t get_fs_size(uint8_t rom);

void schedule_reboot();

#endif

#endif /* DISABLE_OTA */
