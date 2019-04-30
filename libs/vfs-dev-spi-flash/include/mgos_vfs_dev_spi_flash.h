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
#include <stdlib.h>

#include "mgos_spi.h"
#include "mgos_vfs_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_VFS_DEV_TYPE_SPI_FLASH "spi_flash"

bool mgos_vfs_dev_spi_flash_init(void);

enum mgos_vfs_dev_err spi_flash_dev_init(struct mgos_vfs_dev *dev,
                                         struct mgos_spi *spi, int spi_cs,
                                         int spi_freq, int spi_mode,
                                         size_t size, int wip_mask,
                                         bool dpd_en);

#ifdef __cplusplus
}
#endif
