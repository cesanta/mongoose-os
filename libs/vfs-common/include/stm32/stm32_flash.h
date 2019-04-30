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

#ifdef __cplusplus
extern "C" {
#endif

/* Get sector number that covers the specified offset. -1 if invalid. */
int stm32_flash_get_sector(int offset);

/* Get starting offset of the specified sector. -1 if invalid. */
int stm32_flash_get_sector_offset(int sector);

/* Get the size of the specified sector. -1 if invalid. */
int stm32_flash_get_sector_size(int sector);

/* Write a region of flash memory */
bool stm32_flash_write_region(int offset, int len, const void *src);

/* Erase the specified sector. */
bool stm32_flash_erase_sector(int sector);

/* Verify that a region is erased */
bool stm32_flash_region_is_erased(int offset, int len);

/* Verify that a sector is erased */
bool stm32_flash_sector_is_erased(int sector);

#ifdef STM32L4
#define FLASH_ERR_FLAGS                                        \
  (FLASH_FLAG_OPERR | FLASH_FLAG_PROGERR | FLASH_FLAG_WRPERR | \
   FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | FLASH_FLAG_PGSERR | \
   FLASH_FLAG_MISERR | FLASH_FLAG_FASTERR)
#else
#define FLASH_ERR_FLAGS \
  (FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR)
#endif

#ifdef __cplusplus
}
#endif
