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

#include "esp_flash_writer.h"

#include <c_types.h>
#include <spi_flash.h>

#include "common/cs_dbg.h"
#include "esp_missing_includes.h"
#include "mgos_hal.h"
#include "mgos_utils.h"

#define FLASH_SECTOR_SIZE 0x1000
#define FLASH_BLOCK_SIZE 0x10000

bool esp_init_flash_write_ctx(struct esp_flash_write_ctx *wctx, uint32_t addr,
                              uint32_t max_size) {
  if (addr % FLASH_SECTOR_SIZE != 0) {
    LOG(LL_ERROR,
        ("Write address must be mod 0x%x, got 0x%lx", FLASH_SECTOR_SIZE, addr));
    return false;
  }
  memset(wctx, 0, sizeof(*wctx));
  wctx->addr = addr;
  wctx->num_erased = 0;
  wctx->max_size = max_size;
  return true;
}

/* Cache-disabling wrapper around SPIEraseBlock, a-la spi_flash_erase_sector. */
IRAM int spi_flash_erase_block(uint32_t block_no) {
  Cache_Read_Disable_2();
  int ret = SPIEraseBlock(block_no);
  Cache_Read_Enable_2();
  return ret;
}

int esp_flash_write(struct esp_flash_write_ctx *wctx,
                    const struct mg_str data) {
  if (wctx->num_written + data.len > wctx->max_size) {
    LOG(LL_ERROR, ("Exceeded max size"));
    return -10;
  }
  mgos_wdt_feed();
  while (wctx->num_erased < wctx->num_written + data.len) {
    uint32_t erase_addr = wctx->addr + wctx->num_erased;
/* If possible, erase entire block. If not, erase a page. */
#ifndef RTOS_SDK
    if (erase_addr % FLASH_BLOCK_SIZE == 0 &&
        (wctx->num_erased + FLASH_BLOCK_SIZE <= wctx->max_size)) {
      uint32_t block_no = erase_addr / FLASH_BLOCK_SIZE;
      int ret = spi_flash_erase_block(block_no);
      LOG((ret == 0 ? LL_DEBUG : LL_ERROR),
          ("Erase block %lu (0x%lx) -> %d", block_no, erase_addr, ret));
      if (ret != 0) return -11;
      wctx->num_erased += FLASH_BLOCK_SIZE;
    } else
#endif
    {
      uint32_t sec_no = erase_addr / FLASH_SECTOR_SIZE;
      int ret = spi_flash_erase_sector(sec_no);
      LOG((ret == 0 ? LL_DEBUG : LL_ERROR),
          ("Erase sector %lu (0x%lx) -> %d", sec_no, erase_addr, ret));
      if (ret != 0) return -12;
      wctx->num_erased += FLASH_SECTOR_SIZE;
    }
  }
  /* Writes must be in chunks mod 4. */
  int ret = 0;
  uint32_t align_buf[16];
  uint32_t write_addr = wctx->addr + wctx->num_written;
  uint32_t num_written = 0, to_write = data.len;
  const uint8_t *data_p = (const uint8_t *) data.p;
  /* Align flash write address first. */
  if (write_addr % 4 != 0) {
    uint32_t align_skip = (write_addr % 4);
    uint32_t align_write_addr = write_addr - align_skip;
    uint32_t align_remain = 4 - align_skip;
    uint32_t align_write_len = MIN(to_write, align_remain);
    align_buf[0] = 0xffffffff;
    memcpy(((uint8_t *) &align_buf[0]) + align_skip, data_p, align_write_len);
    ret = spi_flash_write(align_write_addr, align_buf, 4);
    if (ret == 0) {
      data_p += align_write_len;
      write_addr += align_write_len;
      num_written += align_write_len;
      to_write -= align_write_len;
    } else {
      to_write = 0;
    }
  }
  /* Write the bulk of data. If data is in flash, need to copy it to RAM. */
  bool need_copy = (((uintptr_t) data_p) > 0x40000000);
  uint32_t *write_data_p = (need_copy ? align_buf : (uint32_t *) data_p);
  while (to_write >= 4) {
    uint32_t write_len = to_write & ~3;
    if (need_copy) {
      write_len = MIN(write_len, sizeof(align_buf));
      memcpy(align_buf, data_p, write_len);
    }
    ret = spi_flash_write(write_addr, write_data_p, write_len);
    if (ret == 0) {
      data_p += write_len;
      write_addr += write_len;
      num_written += write_len;
      to_write -= write_len;
      if (!need_copy) write_data_p += write_len / 4;
    } else {
      to_write = 0;
    }
    mgos_wdt_feed();
  }
  /* Write the tail. */
  if (to_write > 0) {
    align_buf[0] = 0xffffffff;
    memcpy(align_buf, data_p, to_write);
    ret = spi_flash_write(write_addr, align_buf, 4);
    if (ret == 0) {
      num_written += to_write;
    }
  }
  if (ret == 0) {
    wctx->num_written += num_written;
    ret = (int) num_written;
  } else {
    ret = -13;
  }
  LOG((ret > 0 ? (ret != (int) data.len ? LL_DEBUG : LL_VERBOSE_DEBUG)
               : LL_ERROR),
      ("Write %u @ 0x%lx -> %d", data.len, write_addr, ret));
  return ret;
}
