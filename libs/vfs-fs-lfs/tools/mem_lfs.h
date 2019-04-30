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

#pragma once

#include <lfs.h>

#include <stdbool.h>

#define FS_PAGE_SIZE 256
#define FS_BLOCK_SIZE (4 * 1024)
#define FS_ERASE_SIZE (4 * 1024)

#ifdef __cplusplus
extern "C" {
#endif

extern bool log_reads, log_writes, log_erases;
extern int wfail;

int mem_lfs_format(int fs_size, int fs_block_size);
int mem_lfs_mount(int fs_size, int fs_block_size);
int mem_lfs_mount_file(const char *fname, int fs_block_size);
bool mem_lfs_dump(const char *fname);
lfs_t *mem_lfs_get(void);

#ifdef __cplusplus
}
#endif
