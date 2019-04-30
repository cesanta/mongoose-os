/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

#include "rs14100_vfs_dev_qspi_flash.h"

#include <string.h>

#include "common/cs_dbg.h"

#include "mgos_core_dump.h"
#include "mgos_gpio.h"
#include "mgos_time.h"
#include "mgos_vfs_dev.h"
#include "mongoose.h"

#include "rs14100_sdk.h"

#define SPI_FLASH_PAGE_SIZE 256
#define SPI_FLASH_SECTOR_SIZE 4096
#define FLASH_BASE QSPI_AUTOM_CHIP0_ADDRESS

#define SPI_FLASH_PAGE_PROG 0x02
#define SPI_FLASH_RDSR 0x05
#define SPI_FLASH_WREN 0x06
#define SPI_FLASH_WRDI 0x04
#define SPI_FLASH_ERASE_SECTOR 0x20

// SDK headers define a bunch of macros that clash with struct field defs.
#undef QSPI_MANUAL_RD_CNT
#undef AUTO_MODE_FSM_IDLE_SCLK
#undef QSPI_AUTO_MODE

static enum mgos_vfs_dev_err rs14100_vfs_dev_qspi_flash_open(
    struct mgos_vfs_dev *dev, const char *opts) {
  (void) dev;
  (void) opts;
  return MGOS_VFS_DEV_ERR_NONE;
}

IRAM void rs14100_qspi_ctl_acquire(void) {
  mgos_ints_disable();
  while (!QSPI->QSPI_MANUAL_STATUS_b.AUTO_MODE_FSM_IDLE_SCLK) {
  }
  QSPI->QSPI_MANUAL_CONFIG1_b.HW_CTRLD_QSPI_MODE_CTRL = 0;
  QSPI->QSPI_MANUAL_CONFIG1_b.QSPI_MANUAL_QSPI_MODE = 1;
  QSPI->QSPI_BUS_MODE_b.QSPI_AUTO_MODE_FRM_REG = 0;
  // Reset FIFOs.
  QSPI->QSPI_FIFO_THRLD_b.WFIFO_RESET = 1;
  QSPI->QSPI_FIFO_THRLD_b.RFIFO_RESET = 1;
  QSPI->QSPI_FIFO_THRLD_b.WFIFO_RESET = 0;
  QSPI->QSPI_FIFO_THRLD_b.RFIFO_RESET = 0;
  QSPI->QSPI_MANUAL_CONFIG1_b.QSPI_MANUAL_CSN_SELECT = 0;
}

IRAM void rs14100_qspi_ctl_release(void) {
  QSPI->QSPI_MANUAL_CONFIG1_b.QSPI_MANUAL_QSPI_MODE = 0;
  QSPI->QSPI_BUS_MODE_b.QSPI_AUTO_MODE_FRM_REG = 1;
  while (!QSPI->QSPI_MANUAL_STATUS_b.QSPI_AUTO_MODE) {
  }
  mgos_ints_enable();
}

IRAM static void rs14100_qspi_flash_wait_ctl_manual_idle(void) {
  while (QSPI->QSPI_MANUAL_STATUS_b.QSPI_BUSY) {
  }
}

IRAM static void rs14100_qspi_flash_cs_on(void) {
  QSPI->QSPI_MANUAL_CONFIG1_b.QSPI_MANUAL_CSN = 0;  // Assert CS
}

IRAM static void rs14100_qspi_flash_cs_off(void) {
  QSPI->QSPI_MANUAL_CONFIG1_b.QSPI_MANUAL_CSN = 1;  // Deassert CS
}

IRAM static void do_write(void) {
  QSPI->QSPI_MANUAL_CONFIG1_b.QSPI_MANUAL_WR = 1;  // Trigger write
  rs14100_qspi_flash_wait_ctl_manual_idle();
  QSPI->QSPI_MANUAL_CONFIG1_b.QSPI_MANUAL_WR = 0;
}

IRAM static void rs14100_qspi_flash_write_byte(uint8_t data, uint8_t mode) {
  QSPI->QSPI_BUS_MODE_b.QSPI_MAN_MODE_CONF_CSN0 = mode;
  QSPI->QSPI_MANUAL_WRITE_DATA2 = 1 * 8;
  QSPI->QSPI_MANUAL_RDWR_FIFO[0] = data;
  do_write();
}

IRAM static void rs14100_qspi_flash_write_word(uint32_t data, uint8_t mode,
                                               bool swap) {
  QSPI->QSPI_BUS_MODE_b.QSPI_MAN_MODE_CONF_CSN0 = mode;
  QSPI->QSPI_MANUAL_CONFIG2_b.QSPI_WR_DATA_SWAP_MNL_CSN0 = swap;
  QSPI->QSPI_MANUAL_WRITE_DATA2 = 4 * 8;
  QSPI->QSPI_MANUAL_RDWR_FIFO[0] = data;
  do_write();
}

static IRAM void rs14100_qspi_flash_write_bytes(const uint8_t *data, size_t len,
                                                uint8_t mode) {
  const uint32_t *words = (const uint32_t *) data;
  while (len >= 4) {
    // Data written to the FIFO is sent out as expected, MSB first
    // but loads from memory to the register reverses the order
    // so we need to perform LE -> BE translation in the controller.
    rs14100_qspi_flash_write_word(*words++, mode, true /* swap */);
    len -= 4;
  }
  const uint8_t *bytes = (const uint8_t *) words;
  while (len > 0) {
    rs14100_qspi_flash_write_byte(*bytes++, mode);
    len--;
  }
}

IRAM static uint8_t rs14100_qspi_flash_read_byte(uint8_t mode) {
  QSPI->QSPI_BUS_MODE_b.QSPI_MAN_MODE_CONF_CSN0 = mode;
  QSPI->QSPI_MANUAL_CONFIG1_b.QSPI_MANUAL_RD_CNT = 1;
  QSPI->QSPI_MANUAL_CONFIG1_b.QSPI_MANUAL_RD = 1;  // Trigger read
  rs14100_qspi_flash_wait_ctl_manual_idle();
  QSPI->QSPI_MANUAL_CONFIG1_b.QSPI_MANUAL_RD = 0;
  return (uint8_t) QSPI->QSPI_MANUAL_RDWR_FIFO[0];
}

IRAM static void rs14100_qspi_flash_wait_idle(void) {
  rs14100_qspi_flash_cs_on();
  rs14100_qspi_flash_write_byte(SPI_FLASH_RDSR, SINGLE_MODE);
  while ((rs14100_qspi_flash_read_byte(SINGLE_MODE) & 1) != 0) {
  }
  rs14100_qspi_flash_cs_off();
}

IRAM static void rs14100_qspi_flash_wren(void) {
  rs14100_qspi_flash_cs_on();
  rs14100_qspi_flash_write_byte(SPI_FLASH_WREN, SINGLE_MODE);
  rs14100_qspi_flash_cs_off();
}

IRAM static enum mgos_vfs_dev_err rs14100_vfs_dev_qspi_flash_read(
    struct mgos_vfs_dev *dev, size_t addr, size_t len, void *dst) {
  const uint8_t *p = (const uint8_t *) (FLASH_BASE + addr);
  uint8_t *q = (uint8_t *) dst;
  // Align source to 32-bit boundary for max flash read performance.
  while (((uintptr_t) p) % 4 != 0 && len > 0) {
    *q++ = *p++;
    len--;
  }
  // Perform bulk of the copy in 32-bit reads.
  const uint32_t *p32 = (const uint32_t *) p;
  uint32_t *q32 = (uint32_t *) q;
  while (len >= 4) {
    *q32++ = *p32++;
    len -= 4;
  }
  // Byte-wise copy of the remainder.
  p = (const uint8_t *) p32;
  q = (uint8_t *) q32;
  while (len > 0) {
    *q++ = *p++;
    len--;
  }
  (void) dev;
  return MGOS_VFS_DEV_ERR_NONE;
}

IRAM NOINLINE static void rs14100_qspi_flash_prog_page(size_t addr,
                                                       const uint8_t *data,
                                                       size_t len) {
  rs14100_qspi_ctl_acquire();
  rs14100_qspi_flash_wren();
  rs14100_qspi_flash_cs_on();
  rs14100_qspi_flash_write_word(addr | (SPI_FLASH_PAGE_PROG << 24), SINGLE_MODE,
                                false /* swap */);
  rs14100_qspi_flash_write_bytes(data, len, SINGLE_MODE);
  rs14100_qspi_flash_cs_off();
  rs14100_qspi_flash_wait_idle();
  rs14100_qspi_ctl_release();
}

static enum mgos_vfs_dev_err rs14100_vfs_dev_qspi_flash_write(
    struct mgos_vfs_dev *dev, size_t addr, size_t len, const void *src) {
  const uint8_t *p = (const uint8_t *) src;
  while (len > 0) {
    // Align to page boundary.
    size_t wr = SPI_FLASH_PAGE_SIZE - (addr % SPI_FLASH_PAGE_SIZE);
    size_t wl = MIN(wr, len);
    rs14100_qspi_flash_prog_page(addr, p, wl);
    addr += wl;
    len -= wl;
    p += wl;
  }
  (void) dev;
  return MGOS_VFS_DEV_ERR_NONE;
}

IRAM NOINLINE static void rs14100_qspi_flash_erase_sector(size_t addr) {
  rs14100_qspi_ctl_acquire();
  rs14100_qspi_flash_wren();
  rs14100_qspi_flash_cs_on();
  rs14100_qspi_flash_write_word(addr | (SPI_FLASH_ERASE_SECTOR << 24),
                                SINGLE_MODE, false /* swap */);
  rs14100_qspi_flash_cs_off();
  rs14100_qspi_flash_wait_idle();
  rs14100_qspi_ctl_release();
  (void) addr;
}

static enum mgos_vfs_dev_err rs14100_vfs_dev_qspi_flash_erase(
    struct mgos_vfs_dev *dev, size_t addr, size_t len) {
  if (addr % SPI_FLASH_SECTOR_SIZE != 0 || len % SPI_FLASH_SECTOR_SIZE != 0) {
    return MGOS_VFS_DEV_ERR_INVAL;
  }
  while (len > 0) {
    rs14100_qspi_flash_erase_sector(addr);
    addr += SPI_FLASH_SECTOR_SIZE;
    len -= SPI_FLASH_SECTOR_SIZE;
  }
  (void) dev;
  return MGOS_VFS_DEV_ERR_NONE;
}

size_t rs14100_vfs_dev_qspi_flash_get_size(struct mgos_vfs_dev *dev) {
  (void) dev;
  return SPI_FLASH_SECTOR_SIZE * 1024;
}

static enum mgos_vfs_dev_err rs14100_vfs_dev_qspi_flash_close(
    struct mgos_vfs_dev *dev) {
  (void) dev;
  return MGOS_VFS_DEV_ERR_NONE;
}

static enum mgos_vfs_dev_err rs14100_vfs_dev_qspi_flash_get_erase_sizes(
    struct mgos_vfs_dev *dev, size_t sizes[MGOS_VFS_DEV_NUM_ERASE_SIZES]) {
  sizes[0] = SPI_FLASH_SECTOR_SIZE;
  (void) dev;
  return MGOS_VFS_DEV_ERR_NONE;
}

static const struct mgos_vfs_dev_ops rs14100_vfs_dev_qspi_flash_ops = {
    .open = rs14100_vfs_dev_qspi_flash_open,
    .read = rs14100_vfs_dev_qspi_flash_read,
    .write = rs14100_vfs_dev_qspi_flash_write,
    .erase = rs14100_vfs_dev_qspi_flash_erase,
    .get_size = rs14100_vfs_dev_qspi_flash_get_size,
    .close = rs14100_vfs_dev_qspi_flash_close,
    .get_erase_sizes = rs14100_vfs_dev_qspi_flash_get_erase_sizes,
};

bool rs14100_vfs_dev_qspi_flash_register_type(void) {
  return mgos_vfs_dev_register_type(MGOS_VFS_DEV_TYPE_RSI1X_FLASH,
                                    &rs14100_vfs_dev_qspi_flash_ops);
}
