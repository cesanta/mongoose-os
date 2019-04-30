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

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_DEV_TYPE_SLFS_CONTAINER "slfs_container"

/* TI recommends rounding to nearest multiple of 4K - 512 bytes.
 * However, experiments have shown that you need to leave 1024 bytes at the end
 * otherwise additional 4K is allocated (compare AllocatedLen vs FileLen). */
#define FS_CONTAINER_SIZE(fs_size) (((((fs_size) >> 12) + 1) << 12) - 1024)

bool cc3200_vfs_dev_slfs_container_register_type(void);

void cc3200_vfs_dev_slfs_container_fname(const char *cpfx, int cidx,
                                         uint8_t *fname);

bool cc3200_vfs_dev_slfs_container_write_meta(int fh, uint64_t seq,
                                              uint32_t fs_size,
                                              uint32_t fs_block_size,
                                              uint32_t fs_page_size,
                                              uint32_t fs_erase_size);

void cc3200_vfs_dev_slfs_container_delete_container(const char *cpfx, int cidx);

void cc3200_vfs_dev_slfs_container_delete_inactive_container(const char *cpfx);

void cc3200_vfs_dev_slfs_container_flush_all(void);

bool cc3200_fs_container_mount(const char *path, const char *container_prefix);

#ifdef __cplusplus
}
#endif
