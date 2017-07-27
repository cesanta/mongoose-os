/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp8266/src/esp_vfs_dev_sysflash.h"

#include "c_types.h"
#include "spi_flash.h"

#include "common/cs_dbg.h"
#include "mongoose/mongoose.h"

#include "mgos_vfs_dev.h"
#include "mgos_vfs_fs_spiffs.h"

#include "fw/platforms/esp8266/src/esp_fs.h"

#define FLASH_UNIT_SIZE 4

static bool esp_vfs_dev_sysflash_open(struct mgos_vfs_dev *dev,
                                      const char *opts) {
  (void) dev;
  (void) opts;
  return true;
}

static bool esp_spi_flash_readwrite(size_t addr, size_t size, void *data,
                                    bool write) {
  bool ret = false;
  /*
   * With proper configurarion spiffs never reads or writes more than
   * MGOS_SPIFFS_DEFAULT_PAGE_SIZE
   */
  if (size > MGOS_SPIFFS_DEFAULT_PAGE_SIZE) {
    LOG(LL_ERROR, ("Invalid size provided to read/write (%d)", (int) size));
    goto out;
  }

  u32_t tmp_buf[(MGOS_SPIFFS_DEFAULT_PAGE_SIZE + FLASH_UNIT_SIZE * 2) /
                sizeof(u32_t)];
  u32_t aligned_addr = addr & (-FLASH_UNIT_SIZE);
  u32_t aligned_size =
      ((size + (FLASH_UNIT_SIZE - 1)) & -FLASH_UNIT_SIZE) + FLASH_UNIT_SIZE;

  int res = spi_flash_read(aligned_addr, tmp_buf, aligned_size);
  if (res != 0) {
    LOG(LL_ERROR, ("spi_flash_read failed: %d (%d, %d)", res,
                   (int) aligned_addr, (int) aligned_size));
    goto out;
  }

  if (!write) {
    memcpy(data, ((u8_t *) tmp_buf) + (addr - aligned_addr), size);
    ret = true;
    goto out;
  }

  memcpy(((u8_t *) tmp_buf) + (addr - aligned_addr), data, size);

  res = spi_flash_write(aligned_addr, tmp_buf, aligned_size);
  if (res != 0) {
    LOG(LL_ERROR, ("spi_flash_write failed: %d (%d, %d)", res,
                   (int) aligned_addr, (int) aligned_size));
    goto out;
  }

  ret = true;

out:
  LOG(LL_VERBOSE_DEBUG,
      ("%s %u @ 0x%x => %d", (write ? "write" : "read"), size, addr, ret));
  return ret;
}

static bool esp_vfs_dev_sysflash_read(struct mgos_vfs_dev *dev, size_t offset,
                                      size_t size, void *dst) {
  (void) dev;
  return esp_spi_flash_readwrite(offset, size, dst, false /* write */);
}

static bool esp_vfs_dev_sysflash_write(struct mgos_vfs_dev *dev, size_t offset,
                                       size_t size, const void *src) {
  (void) dev;
  return esp_spi_flash_readwrite(offset, size, (void *) src, true /* write */);
}

static bool esp_vfs_dev_sysflash_erase(struct mgos_vfs_dev *dev, size_t offset,
                                       size_t len) {
  bool ret = false;
  if (offset % FLASH_SECTOR_SIZE != 0 || len % FLASH_SECTOR_SIZE != 0) {
    LOG(LL_ERROR, ("Invalid size provided to esp_spiffs_erase (%d, %d)",
                   (int) offset, (int) len));
    goto out;
  }
  u32_t sector = (offset / FLASH_SECTOR_SIZE);
  while (sector * FLASH_SECTOR_SIZE < offset + len) {
    ret = (spi_flash_erase_sector(sector) == SPI_FLASH_RESULT_OK);
    sector++;
  }
out:
  LOG(LL_DEBUG, ("erase %u @ %u => %d", len, offset, ret));
  (void) dev;
  return ret;
}

extern SpiFlashChip *flashchip;

size_t esp_vfs_dev_sysflash_get_size(struct mgos_vfs_dev *dev) {
  (void) dev;
  return flashchip->chip_size;
}

static bool esp_vfs_dev_sysflash_close(struct mgos_vfs_dev *dev) {
  /* Nothing to do. */
  (void) dev;
  return true;
}

static const struct mgos_vfs_dev_ops esp_vfs_dev_sysflash_ops = {
    .open = esp_vfs_dev_sysflash_open,
    .read = esp_vfs_dev_sysflash_read,
    .write = esp_vfs_dev_sysflash_write,
    .erase = esp_vfs_dev_sysflash_erase,
    .get_size = esp_vfs_dev_sysflash_get_size,
    .close = esp_vfs_dev_sysflash_close,
};

bool esp_vfs_dev_sysflash_register_type(void) {
  return mgos_vfs_dev_register_type(MGOS_VFS_DEV_TYPE_SYSFLASH,
                                    &esp_vfs_dev_sysflash_ops);
}
