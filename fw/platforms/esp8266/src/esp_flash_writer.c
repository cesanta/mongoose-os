/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp8266/src/esp_flash_writer.h"

#include <c_types.h>
#include <spi_flash.h>

#include "common/cs_dbg.h"
#include "common/platforms/esp8266/esp_missing_includes.h"

#include "fw/src/mgos_hal.h"

#define FLASH_SECTOR_SIZE 0x1000
#define FLASH_BLOCK_SIZE 0x10000

bool esp_init_flash_write_ctx(struct esp_flash_write_ctx *wctx, uint32_t addr,
                              uint32_t max_size) {
  if (addr % FLASH_SECTOR_SIZE != 0) {
    LOG(LL_ERROR,
        ("Write address must be mod 0x%x, got 0x%x", FLASH_SECTOR_SIZE, addr));
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
          ("Erase block %u (0x%x) -> %d", block_no, erase_addr, ret));
      if (ret != 0) return -11;
      wctx->num_erased += FLASH_BLOCK_SIZE;
    } else
#endif
    {
      uint32_t sec_no = erase_addr / FLASH_SECTOR_SIZE;
      int ret = spi_flash_erase_sector(sec_no);
      LOG((ret == 0 ? LL_DEBUG : LL_ERROR),
          ("Erase sector %u (0x%x) -> %d", sec_no, erase_addr, ret));
      if (ret != 0) return -12;
      wctx->num_erased += FLASH_SECTOR_SIZE;
    }
  }
  /* Writes must be in chunks mod 4 from 32-bit aligned addresses. */
  int ret;
  uint32_t write_addr = wctx->addr + wctx->num_written;
  uint32_t num_written = 0;
  if (data.len >= 4) {
    uint32_t to_write = (data.len / 4) * 4;
    while (num_written < to_write) {
      uint32_t tmp[16];
      uint32_t write_size = to_write - num_written;
      if (write_size > sizeof(tmp)) write_size = sizeof(tmp);
      memcpy(tmp, data.p + num_written, write_size);
      ret = spi_flash_write(write_addr + num_written, tmp, write_size);
      if (ret != 0) break;
      num_written += write_size;
      mgos_wdt_feed();
    }
  } else {
    /* This is the last chunk, pad it to 4 bytes. */
    uint32_t tmp = 0xffffffff;
    memcpy(&tmp, data.p, data.len);
    ret = spi_flash_write(write_addr, &tmp, 4);
    num_written = data.len;
  }
  if (ret == 0) {
    wctx->num_written += num_written;
    ret = (int) num_written;
  } else {
    ret = -13;
  }
  LOG((ret > 0 ? (ret != (int) data.len ? LL_DEBUG : LL_VERBOSE_DEBUG)
               : LL_ERROR),
      ("Write %u @ 0x%x -> %d", data.len, write_addr, ret));
  return ret;
}
