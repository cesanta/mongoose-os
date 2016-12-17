/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_FS_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_FS_H_

#ifndef MIOT_MMAP_SLOTS
#define MIOT_MMAP_SLOTS 16
#endif

#include "common/spiffs/spiffs.h"

#include "fw/src/miot_init.h"

/* LOG_PAGE_SIZE have to be more than SPIFFS_OBJ_NAME_LEN */
#define LOG_PAGE_SIZE 256
#define SPIFFS_PAGE_HEADER_SIZE 5
#define SPIFFS_PAGE_DATA_SIZE ((LOG_PAGE_SIZE) - (SPIFFS_PAGE_HEADER_SIZE))
#define FLASH_SECTOR_SIZE 0x1000
#define FLASH_ERASE_BLOCK_SIZE 65536

/* If for whatever reason MMAP_BASE is moved past 0x40000000,
 * a check in flash_emul_exception_handler will need to be adjusted. */
#define MMAP_BASE ((void *) 0x10000000)
#define MMAP_END ((void *) 0x20000000)
#define MMAP_DESC_BITS 24
#define FLASH_BASE 0x40200000

#define MMAP_DESC_FROM_ADDR(addr) \
  (&mmap_descs[(((uintptr_t) addr) >> MMAP_DESC_BITS) & 0xF])
#define MMAP_ADDR_FROM_DESC(desc) \
  ((void *) ((uintptr_t) MMAP_BASE | ((desc - mmap_descs) << MMAP_DESC_BITS)))

struct mmap_desc {
  void *base;
  uint32_t pages;
  uint32_t *blocks; /* pages long */
};

extern struct mmap_desc mmap_descs[MIOT_MMAP_SLOTS];

void fs_flush_stderr(void);
int fs_init(uint32_t addr, uint32_t size);
int fs_mount(spiffs *spf, uint32_t addr, uint32_t size, uint8_t *workbuf,
             uint8_t *fds, size_t fds_size);
spiffs *get_fs(void);
void fs_umount(void);

enum miot_init_result esp_console_init();
enum miot_init_result esp_set_stdout_uart(int uart_no);
enum miot_init_result esp_set_stderr_uart(int uart_no);

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_FS_H_ */
