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

#include "common/cs_dbg.h"

#include "mgos_system.h"

#include "stm32_sdk_hal.h"
#include "stm32_system.h"

#if 0
#include "mgos_boot_dbg.h"
#undef LOG
#define LOG(x, y)         \
  mgos_boot_dbg_printf y; \
  mgos_boot_dbg_putc('\n')
#endif

IRAM void stm32_flash_do_erase(void) {
  mgos_ints_disable();
  stm32_flush_caches();
  FLASH->CR |= FLASH_CR_STRT;
  __DSB();
  while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != 0) {
  }
  stm32_flush_caches();
  mgos_ints_enable();
}

bool stm32_flash_erase_sector(int sector) {
  bool res = false;
  int offset = stm32_flash_get_sector_offset(sector);
  if (offset < 0) goto out;
  HAL_FLASH_Unlock();
#ifdef STM32L4
  uint32_t pnb = (sector & 0xff);
  FLASH->CR = (FLASH_CR_PER | (sector > 0xff ? FLASH_CR_BKER : 0) |
               (pnb << FLASH_CR_PNB_Pos));
#else
  FLASH->CR = FLASH_PSIZE_BYTE | FLASH_CR_SER | (sector << FLASH_CR_SNB_Pos);
#endif
  __HAL_FLASH_CLEAR_FLAG(FLASH_ERR_FLAGS);
  stm32_flash_do_erase();
  HAL_FLASH_Lock();
  if ((FLASH->SR & FLASH_ERR_FLAGS) != 0) {
    LOG(LL_ERROR, ("Flash %s error, flags: 0x%lx", "erase", FLASH->SR));
  }
  res = stm32_flash_sector_is_erased(sector);
out:
  return res;
}

bool stm32_flash_region_is_erased(int offset, int len) {
  bool res = false;
  const uint8_t *p = (const uint8_t *) (FLASH_BASE + offset);
  const uint32_t *q = NULL;
  if (offset < 0 || len < 0 || offset + len > STM32_FLASH_SIZE) goto out;
  while (len > 0 && (((uintptr_t) p) & 3) != 0) {
    if (*p != 0xff) goto out;
    len--;
    p++;
  }
  q = (const uint32_t *) p;
  while (len > 4) {
    if (*q != 0xffffffff) goto out;
    len -= 4;
    q++;
  }
  p = (const uint8_t *) q;
  while (len > 0) {
    if (*p != 0xff) goto out;
    len--;
    p++;
  }
  res = true;

out:
  return res;
}

bool stm32_flash_sector_is_erased(int sector) {
  return stm32_flash_region_is_erased(stm32_flash_get_sector_offset(sector),
                                      stm32_flash_get_sector_size(sector));
}
