/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "stm32_vfs_dev_flash.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "stm32f7xx_hal_conf.h"
#include "stm32f7xx_hal_flash.h"

#include "common/cs_dbg.h"

#include "frozen.h"

#include "mgos_hal.h"
#include "mgos_vfs.h"
#include "mgos_vfs_dev.h"

/* Note: FLASH_BASE_ADDR and FLASH_SIZE are defined externally. */
#if FLASH_BASE_ADDR != FLASH_BASE
#error "FLASH_BASE used by compiler and linker do not match"
#endif

struct dev_data {
  size_t addr;
  size_t size;
};

static bool check_bounds(const struct dev_data *dd, size_t offset, size_t len) {
  return (dd->addr <= FLASH_SIZE && dd->size <= FLASH_SIZE &&
          dd->addr + dd->size <= FLASH_SIZE && offset <= FLASH_SIZE &&
          len <= FLASH_SIZE && dd->addr + offset + len <= FLASH_SIZE);
}

static bool stm32_vfs_dev_flash_open(struct mgos_vfs_dev *dev,
                                     const char *opts) {
  bool res = false;
  struct dev_data *dd = (struct dev_data *) calloc(1, sizeof(*dd));
  if (opts != NULL) {
    json_scanf(opts, strlen(opts), "{addr: %u, size: %u}", &dd->addr,
               &dd->size);
  }
  if (dd->addr == 0 || dd->size == 0) {
    LOG(LL_INFO, ("addr and size are required"));
    goto clean;
  }
  if (!check_bounds(dd, 0, 0)) {
    LOG(LL_INFO, ("invalid settings: %u %u (flash size: %u)", dd->addr,
                  dd->size, FLASH_SIZE));
    goto clean;
  }
  res = true;

clean:
  if (res) {
    dev->dev_data = dd;
  } else {
    free(dd);
  }
  return res;
}

static bool stm32_vfs_dev_flash_read(struct mgos_vfs_dev *dev, size_t offset,
                                     size_t len, void *dst) {
  bool res = false;
  const struct dev_data *dd = (struct dev_data *) dev->dev_data;
  if (check_bounds(dd, offset, len)) {
    memcpy(dst, (uint8_t *) (FLASH_BASE + dd->addr + offset), len);
    res = true;
  }
  LOG((res ? LL_VERBOSE_DEBUG : LL_ERROR),
      ("%p: %s %u @ %d = %d", dev, "read", len, offset, res));
  return res;
}

static bool stm32_vfs_dev_flash_write(struct mgos_vfs_dev *dev, size_t offset,
                                      size_t len, const void *src) {
  bool res = false;
  const struct dev_data *dd = (struct dev_data *) dev->dev_data;
  if (check_bounds(dd, offset, len)) {
    res = true;
    /* Note: could be optimized to use word and half-word writes. */
    HAL_FLASH_Unlock();
    for (size_t i = 0; i < len; i++) {
      uint32_t addr = FLASH_BASE + dd->addr + offset;
      uint8_t byte = *(((const uint8_t *) src) + i);
      int st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr, byte);
      if (st != HAL_OK) {
        LOG(LL_ERROR, ("i = %u st = %d %lu 0x%lx", i, st, HAL_FLASH_GetError(),
                       FLASH->SR));
        res = false;
        break;
      }
    }
  }
  HAL_FLASH_Lock();
  LOG((res ? LL_VERBOSE_DEBUG : LL_ERROR),
      ("%p: %s %u @ %d = %d", dev, "write", len, offset, res));
  return res;
}

static bool stm32_vfs_dev_flash_erase(struct mgos_vfs_dev *dev, size_t offset,
                                      size_t len) {
  /* STM32's flash erase granularity is too coarse, erase is not supported. */
  LOG(LL_ERROR, ("flash erase is not supported"));
  (void) dev;
  (void) offset;
  (void) len;
  return false;
}

static size_t stm32_vfs_dev_flash_get_size(struct mgos_vfs_dev *dev) {
  struct dev_data *dd = (struct dev_data *) dev->dev_data;
  return dd->size;
}

static bool stm32_vfs_dev_flash_close(struct mgos_vfs_dev *dev) {
  free(dev->dev_data);
  return true;
}

static const struct mgos_vfs_dev_ops stm32_vfs_dev_flash_ops = {
    .open = stm32_vfs_dev_flash_open,
    .read = stm32_vfs_dev_flash_read,
    .write = stm32_vfs_dev_flash_write,
    .erase = stm32_vfs_dev_flash_erase,
    .get_size = stm32_vfs_dev_flash_get_size,
    .close = stm32_vfs_dev_flash_close,
};

bool stm32_vfs_dev_flash_register_type(void) {
  return mgos_vfs_dev_register_type(MGOS_VFS_DEV_TYPE_STM32_FLASH,
                                    &stm32_vfs_dev_flash_ops);
}
