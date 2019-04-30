/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _MEM_SPIFFS_H_
#define _MEM_SPIFFS_H_

#include <spiffs.h>

#include <stdbool.h>

#define FS_PAGE_SIZE 256
#define FS_BLOCK_SIZE (4 * 1024)
#define FS_ERASE_SIZE (4 * 1024)

#ifdef __cplusplus
extern "C" {
#endif

extern spiffs fs;

extern char *image; /* in memory flash image */
extern bool log_reads, log_writes, log_erases;
extern int wfail;

s32_t mem_spiffs_erase(spiffs *fs, u32_t addr, u32_t size);
int mem_spiffs_mount(int fs_size, int fs_block_size, int fs_page_size,
                     int fs_erase_size);
int mem_spiffs_mount_file(const char *fname, int fs_block_size,
                          int fs_page_size, int fs_erase_size);
bool mem_spiffs_dump(const char *fname);
u32_t list_files(void);

#ifdef __cplusplus
}
#endif

#endif
