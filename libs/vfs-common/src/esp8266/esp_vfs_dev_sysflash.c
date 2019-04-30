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

#include "esp_vfs_dev_sysflash.h"

#include "c_types.h"
#include "spi_flash.h"

#include "common/cs_dbg.h"
#include "mongoose.h"

#include "mgos_vfs_dev.h"
#include "mgos_vfs_fs_spiffs.h"

#include "esp_fs.h"

#define FLASH_UNIT_SIZE 4

static enum mgos_vfs_dev_err esp_vfs_dev_sysflash_open(struct mgos_vfs_dev *dev,
                                                       const char *opts) {
  (void) dev;
  (void) opts;
  return MGOS_VFS_DEV_ERR_NONE;
}

static enum mgos_vfs_dev_err esp_spi_flash_readwrite(size_t addr, size_t size,
                                                     void *data, bool write) {
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_INVAL;
  /*
   * With proper configurarion spiffs never reads or writes more than
   * MGOS_SPIFFS_DEFAULT_PAGE_SIZE
   */
  if (size > MGOS_SPIFFS_DEFAULT_PAGE_SIZE) {
    LOG(LL_ERROR, ("Invalid size provided to read/write (%d)", (int) size));
    goto out;
  }

  res = MGOS_VFS_DEV_ERR_IO;
  u32_t tmp_buf[(MGOS_SPIFFS_DEFAULT_PAGE_SIZE + FLASH_UNIT_SIZE * 2) /
                sizeof(u32_t)];
  u32_t aligned_addr = addr & (-FLASH_UNIT_SIZE);
  u32_t aligned_size =
      ((size + (FLASH_UNIT_SIZE - 1)) & -FLASH_UNIT_SIZE) + FLASH_UNIT_SIZE;

  int sres = spi_flash_read(aligned_addr, tmp_buf, aligned_size);
  if (sres != 0) {
    LOG(LL_ERROR, ("spi_flash_read failed: %d (%d, %d)", sres,
                   (int) aligned_addr, (int) aligned_size));
    goto out;
  }

  if (!write) {
    memcpy(data, ((u8_t *) tmp_buf) + (addr - aligned_addr), size);
    goto out_ok;
  }

  memcpy(((u8_t *) tmp_buf) + (addr - aligned_addr), data, size);

  sres = spi_flash_write(aligned_addr, tmp_buf, aligned_size);
  if (sres != 0) {
    LOG(LL_ERROR, ("spi_flash_write failed: %d (%d, %d)", sres,
                   (int) aligned_addr, (int) aligned_size));
    goto out;
  }
out_ok:
  res = MGOS_VFS_DEV_ERR_NONE;
out:
  LOG((res == 0 ? LL_VERBOSE_DEBUG : LL_ERROR),
      ("%s %u @ 0x%x => %d", (write ? "write" : "read"), size, addr, res));
  return res;
}

static enum mgos_vfs_dev_err esp_vfs_dev_sysflash_read(struct mgos_vfs_dev *dev,
                                                       size_t offset,
                                                       size_t size, void *dst) {
  (void) dev;
  return esp_spi_flash_readwrite(offset, size, dst, false /* write */);
}

static enum mgos_vfs_dev_err esp_vfs_dev_sysflash_write(
    struct mgos_vfs_dev *dev, size_t offset, size_t size, const void *src) {
  (void) dev;
  return esp_spi_flash_readwrite(offset, size, (void *) src, true /* write */);
}

static enum mgos_vfs_dev_err esp_vfs_dev_sysflash_erase(
    struct mgos_vfs_dev *dev, size_t offset, size_t len) {
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_INVAL;
  if (offset % FLASH_SECTOR_SIZE != 0 || len % FLASH_SECTOR_SIZE != 0) {
    LOG(LL_ERROR, ("Invalid size provided to esp_spiffs_erase (%d, %d)",
                   (int) offset, (int) len));
    goto out;
  }
  u32_t sector = (offset / FLASH_SECTOR_SIZE);
  while (sector * FLASH_SECTOR_SIZE < offset + len) {
    if (spi_flash_erase_sector(sector) != SPI_FLASH_RESULT_OK) {
      res = MGOS_VFS_DEV_ERR_IO;
      goto out;
    }
    sector++;
  }
  res = MGOS_VFS_DEV_ERR_NONE;
out:
  LOG((res == 0 ? LL_VERBOSE_DEBUG : LL_ERROR),
      ("erase %u @ %u => %d", len, offset, res));
  (void) dev;
  return res;
}

extern SpiFlashChip *flashchip;

size_t esp_vfs_dev_sysflash_get_size(struct mgos_vfs_dev *dev) {
  (void) dev;
  return flashchip->chip_size;
}

static enum mgos_vfs_dev_err esp_vfs_dev_sysflash_close(
    struct mgos_vfs_dev *dev) {
  (void) dev;
  return MGOS_VFS_DEV_ERR_NONE;
}

static enum mgos_vfs_dev_err esp_vfs_dev_sysflash_get_erase_sizes(
    struct mgos_vfs_dev *dev, size_t sizes[MGOS_VFS_DEV_NUM_ERASE_SIZES]) {
  sizes[0] = FLASH_SECTOR_SIZE;
  (void) dev;
  return MGOS_VFS_DEV_ERR_NONE;
}

static const struct mgos_vfs_dev_ops esp_vfs_dev_sysflash_ops = {
    .open = esp_vfs_dev_sysflash_open,
    .read = esp_vfs_dev_sysflash_read,
    .write = esp_vfs_dev_sysflash_write,
    .erase = esp_vfs_dev_sysflash_erase,
    .get_size = esp_vfs_dev_sysflash_get_size,
    .close = esp_vfs_dev_sysflash_close,
    .get_erase_sizes = esp_vfs_dev_sysflash_get_erase_sizes,
};

bool esp_vfs_dev_sysflash_register_type(void) {
  return mgos_vfs_dev_register_type(MGOS_VFS_DEV_TYPE_SYSFLASH,
                                    &esp_vfs_dev_sysflash_ops);
}
