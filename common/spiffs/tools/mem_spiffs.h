/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef _MEM_SPIFFS_H_
#define _MEM_SPIFFS_H_

#include <spiffs.h>

#include <stdbool.h>

#define LOG_PAGE_SIZE 256
#define FLASH_BLOCK_SIZE (4 * 1024)

#ifdef __cplusplus
extern "C" {
#endif

extern spiffs fs;

extern char *image; /* in memory flash image */
extern size_t image_size;
extern bool log_reads, log_writes, log_erases;
extern int wfail;

s32_t mem_spiffs_erase(spiffs *fs, u32_t addr, u32_t size);
int mem_spiffs_mount(void);
bool mem_spiffs_dump(const char *fname);
u32_t list_files(void);

#ifdef __cplusplus
}
#endif

#endif
