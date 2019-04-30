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

#if defined(STM32L4)

#include "common/cs_dbg.h"
#include "common/str_util.h"

#include "mongoose.h"
#include "mgos_system.h"

#include "stm32_sdk_hal.h"
#include "stm32_system.h"

#define STM32L4_FLASH_WRITE_SIZE 8
#define STM32L4_FLASH_WRITE_ALIGN 8

#if 0
#include "mgos_boot_dbg.h"
#undef LOG
#define LOG(x, y)         \
  mgos_boot_dbg_printf y; \
  mgos_boot_dbg_putc('\n')
#endif

int stm32_flash_get_sector(int offset) {
  return offset / FLASH_PAGE_SIZE;
}

int stm32_flash_get_sector_offset(int sector) {
  return sector * FLASH_PAGE_SIZE;
}

int stm32_flash_get_sector_size(int sector) {
  (void) sector;
  return FLASH_PAGE_SIZE;
}

IRAM bool stm32_flash_write_region(int offset, int len, const void *src) {
  bool res = false;
  if (offset < 0 || len < 0 || offset + len > STM32_FLASH_SIZE) goto out;
  /* We do not support unaligned writes at the moment. */
  if (offset % STM32L4_FLASH_WRITE_ALIGN != 0 ||
      len % STM32L4_FLASH_WRITE_SIZE != 0) {
    goto out;
  }
  volatile uint32_t *dst = (uint32_t *) (FLASH_BASE + offset), *p = dst;
  const uint32_t *q = (const uint32_t *) src;
  HAL_FLASH_Unlock();
  __HAL_FLASH_CLEAR_FLAG(FLASH_ERR_FLAGS);
  res = true;
  mgos_ints_disable();
  while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != 0) {
  }
  FLASH->CR = FLASH_CR_PG;
  for (int i = 0; i < len && res; i += STM32L4_FLASH_WRITE_SIZE) {
    __DSB();
    *p++ = *q++;
    __DSB();
    *p++ = *q++;
    __DSB();
    while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != 0) {
    }
    if ((FLASH->SR & FLASH_ERR_FLAGS) != 0) {
      LOG(LL_ERROR, ("Flash %s error @ %d, flags: 0x%lx", "prog",
                     (int) offset + i, FLASH->SR));
      break;
    }
  }
  CLEAR_BIT(FLASH->CR, FLASH_CR_PG);
  mgos_ints_enable();
  if (res) {
    res = (memcmp(src, (const void *) dst, len) == 0);
    if (!res) {
      LOG(LL_ERROR, ("Flash %s error @ %d, flags: 0x%lx", "verify",
                     (int) offset, FLASH->SR));
    }
  }
  HAL_FLASH_Lock();
out:
  return res;
}

extern enum mgos_vfs_dev_err stm32_vfs_dev_flash_get_erase_sizes(
    struct mgos_vfs_dev *dev, size_t sizes[MGOS_VFS_DEV_NUM_ERASE_SIZES]) {
  sizes[0] = FLASH_PAGE_SIZE;
  (void) dev;
  return MGOS_VFS_DEV_ERR_NONE;
}
#endif /* defined(STM32L4) */
