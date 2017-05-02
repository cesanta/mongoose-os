/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_FS_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_FS_H_

#include "common/spiffs/spiffs.h"

#include "fw/src/mgos_init.h"

/*
 * number of file descriptors reserved for system.
 * SPIFFS currently returns file descriptors that
 * clash with "system" fds like stdout and stderr.
 * Here we remap all spiffs fds by adding/subtracting NUM_SYS_FD
 */
#define NUM_SYS_FD 3

/* LOG_PAGE_SIZE have to be more than SPIFFS_OBJ_NAME_LEN */
#define LOG_PAGE_SIZE 256
#define SPIFFS_PAGE_HEADER_SIZE 5
#define SPIFFS_PAGE_DATA_SIZE ((LOG_PAGE_SIZE) - (SPIFFS_PAGE_HEADER_SIZE))
#define FLASH_SECTOR_SIZE 0x1000
#define FLASH_ERASE_BLOCK_SIZE 65536

int fs_init(uint32_t addr, uint32_t size);
int fs_mount(spiffs *spf, uint32_t addr, uint32_t size, uint8_t *workbuf,
             uint8_t *fds, size_t fds_size);
spiffs *cs_spiffs_get_fs(void);
void fs_umount(void);

enum mgos_init_result esp_console_init(void);

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_FS_H_ */
