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

#include "stm32_flash.h"

#if defined(STM32F2) || defined(STM32F4) || defined(STM32F7)

#include "common/cs_dbg.h"
#include "common/str_util.h"

#include "mongoose.h"
#include "mgos_system.h"

#include "stm32_sdk_hal.h"
#include "stm32_system.h"

static const size_t s_stm32f_flash_layout[FLASH_SECTOR_TOTAL] = {
#if defined(STM32F2) || defined(STM32F4)
#if STM32_FLASH_SIZE == 524288
    16384,  16384,  16384, 16384, 65536,
    131072, 131072, 131072
#elif STM32_FLASH_SIZE == 1048576
    16384,  16384,  16384,  16384,  65536, 131072, 131072,
    131072, 131072, 131072, 131072, 131072
#elif STM32_FLASH_SIZE == 1572864
    16384,  16384,  16384,  16384,  65536,  131072, 131072, 131072, 131072,
    131072, 131072, 131072, 131072, 131072, 131072, 131072
#elif STM32_FLASH_SIZE == 2097152 /* dual-bank */
    16384,  16384,  16384,  16384,  65536,  131072, 131072, 131072, 131072,
    131072, 131072, 131072, 16384,  16384,  16384,  16384,  65536,  131072,
    131072, 131072, 131072, 131072, 131072, 131072
#else
#error Unsupported flash size
#endif
#else
#if STM32_FLASH_SIZE == 1048576
    32768,  32768,  32768, 32768, 131072,
    262144, 262144, 262144
#else
#error Unsupported flash size
#endif
#endif
};

int stm32_flash_get_sector(int offset) {
  int sector = -1, sector_end = 0;
  do {
    sector++;
    if (s_stm32f_flash_layout[sector] == 0) return -1;
    sector_end += s_stm32f_flash_layout[sector];
  } while (sector_end <= offset);
  return sector;
}

int stm32_flash_get_sector_offset(int sector) {
  int sector_offset = 0;
  while (sector > 0) {
    sector--;
    sector_offset += s_stm32f_flash_layout[sector];
  }
  return sector_offset;
}

int stm32_flash_get_sector_size(int sector) {
  return s_stm32f_flash_layout[sector];
}

IRAM bool stm32_flash_write_region(int offset, int len, const void *src) {
  bool res = false;
  if (offset < 0 || len < 0 || offset + len > STM32_FLASH_SIZE) goto out;
  volatile uint8_t *dst = (uint8_t *) (FLASH_BASE + offset), *p = dst;
  const uint8_t *q = (const uint8_t *) src;
  HAL_FLASH_Unlock();
  __HAL_FLASH_CLEAR_FLAG(FLASH_ERR_FLAGS);
  res = true;
  mgos_ints_disable();
  while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != 0) {
  }
  FLASH->CR = FLASH_PSIZE_BYTE | FLASH_CR_PG;
  for (int i = 0; i < len && res; i++) {
    __DSB();
    *p++ = *q++;
    __DSB();
    while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != 0) {
    }
    res = (__HAL_FLASH_GET_FLAG(FLASH_ERR_FLAGS) == 0);
    if (!res) {
      LOG(LL_ERROR, ("Flash %s error, flags: 0x%lx", "prog", FLASH->SR));
      break;
    }
  }
  CLEAR_BIT(FLASH->CR, FLASH_CR_PG);
  mgos_ints_enable();
  if (res) {
    res = (memcmp(src, (const void *) dst, len) == 0);
    if (!res) {
      LOG(LL_ERROR, ("Flash %s error, flags: 0x%lx", "verify", FLASH->SR));
    }
  }
  HAL_FLASH_Lock();
out:
  return res;
}

extern enum mgos_vfs_dev_err stm32_vfs_dev_flash_get_erase_sizes(
    struct mgos_vfs_dev *dev, size_t sizes[MGOS_VFS_DEV_NUM_ERASE_SIZES]) {
  for (int i = 0, j = -1; i < (int) ARRAY_SIZE(s_stm32f_flash_layout) && j < 8;
       i++) {
    if (j < 0 || s_stm32f_flash_layout[i] != sizes[j]) {
      sizes[++j] = s_stm32f_flash_layout[i];
    }
  }
  (void) dev;
  return MGOS_VFS_DEV_ERR_NONE;
}

#endif /* defined(STM32F4) || defined(STM32F7) */
