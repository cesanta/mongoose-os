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

#include "mgos_spi.h"
#include "mgos_vfs_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_VFS_DEV_TYPE_W25XXX "w25xxx"

#define W25XXX_PAGE_SIZE 2048U
#define W25XXX_BLOCK_SIZE (64 * W25XXX_PAGE_SIZE)
#define W25XXX_DIE_SIZE (1024 * W25XXX_BLOCK_SIZE)

enum mgos_vfs_dev_err w25xxx_dev_init(struct mgos_vfs_dev *dev,
                                      struct mgos_spi *spi, int spi_cs,
                                      int spi_freq, int spi_mode,
                                      int bb_reserve, bool ecc_chk);

/*
 * Adds a bad block lookup table entry.
 * NB: Both offsets are raw, with no bb_reserve correction.
 */
bool w25xxx_remap_block(struct mgos_vfs_dev *dev, size_t bad_off,
                        size_t good_off);

#ifdef __cplusplus
}
#endif
