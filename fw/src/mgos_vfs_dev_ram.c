/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 */

#include "mgos_vfs_dev_ram.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "common/cs_dbg.h"

#include "frozen/frozen.h"

#include "mgos_hal.h"
#include "mgos_vfs.h"
#include "mgos_vfs_dev.h"

struct mgos_vfs_dev_ram_data {
  size_t size;
  uint8_t *data;
  bool flash_check;
  uint8_t erase_byte;
};

static bool mgos_vfs_dev_ram_open(struct mgos_vfs_dev *dev, const char *opts) {
  bool res = false;
  struct mgos_vfs_dev_ram_data *dd = NULL;
  uint8_t fb = 0xff;
  int size = 0, flash_check = false, erase_byte = 0xff, fill_byte = 0xff;
  json_scanf(opts, strlen(opts),
             "{size: %d, erase_byte: %d, fill_byte: %d, flash_check: %B}",
             &size, &erase_byte, &fill_byte, &flash_check);
  if (size <= 0) {
    LOG(LL_ERROR, ("Size is required for RAM device"));
    goto out;
  }
  dd = (struct mgos_vfs_dev_ram_data *) calloc(1, sizeof(*dd));
  dd->size = size;
  dd->data = (uint8_t *) malloc(dd->size);
  dd->flash_check = flash_check;
  dd->erase_byte = (uint8_t) erase_byte;
  if (dd->data == NULL) goto out;
  fb = (uint8_t) fill_byte;
  memset(dd->data, fb, dd->size);
  dev->dev_data = dd;
  res = true;

out:
  if (!res && dd != NULL) {
    free(dd->data);
    free(dd);
  } else {
    LOG(LL_INFO, ("%u bytes, eb 0x%02x, fb 0x%02x, fc %s", (unsigned) dd->size,
                  dd->erase_byte, fb, (dd->flash_check ? "yes" : "no")));
  }
  return res;
}

static bool mgos_vfs_dev_ram_read(struct mgos_vfs_dev *dev, size_t offset,
                                  size_t len, void *dst) {
  bool res = false;
  struct mgos_vfs_dev_ram_data *dd =
      (struct mgos_vfs_dev_ram_data *) dev->dev_data;
  if (len > dd->size || offset + len > dd->size) {
    goto out;
  }
  memcpy(dst, dd->data + offset, len);
  res = true;

out:
  LOG((res ? LL_DEBUG : LL_ERROR),
      ("%s %u @ 0x%x = %d", "read", (unsigned) len, (unsigned) offset, res));
  return res;
}

static bool mgos_vfs_dev_ram_write(struct mgos_vfs_dev *dev, size_t offset,
                                   size_t len, const void *src) {
  bool res = false;
  uint8_t *srcb = (uint8_t *) src;
  struct mgos_vfs_dev_ram_data *dd =
      (struct mgos_vfs_dev_ram_data *) dev->dev_data;
  if (len > dd->size || offset + len > dd->size) {
    goto out;
  }
  for (size_t i = 0; i < len; i++) {
    uint8_t src_byte = srcb[i];
    uint8_t dst_byte = dd->data[offset + i];
    if (dd->flash_check) {
      if ((src_byte & dst_byte) != src_byte) {
        LOG(LL_ERROR, ("NOR flash check violation @ %u: 0x%02x -> 0x%02x",
                       (unsigned) (offset + i), dst_byte, src_byte));
        goto out;
      }
    }
    dd->data[offset + i] = src_byte;
  }

  res = true;

out:
  LOG((res ? LL_DEBUG : LL_ERROR),
      ("%s %u @ 0x%x = %d", "write", (unsigned) len, (unsigned) offset, res));
  return res;
}

static bool mgos_vfs_dev_ram_erase(struct mgos_vfs_dev *dev, size_t offset,
                                   size_t len) {
  bool res = false;
  struct mgos_vfs_dev_ram_data *dd =
      (struct mgos_vfs_dev_ram_data *) dev->dev_data;
  if (len > dd->size || offset + len > dd->size) {
    goto out;
  }
  memset(dd->data + offset, 0xff, len);
  res = true;

out:
  LOG((res ? LL_DEBUG : LL_ERROR),
      ("%s %u @ 0x%x = %d", "erase", (unsigned) len, (unsigned) offset, res));
  return res;
}

static size_t mgos_vfs_dev_ram_get_size(struct mgos_vfs_dev *dev) {
  struct mgos_vfs_dev_ram_data *dd =
      (struct mgos_vfs_dev_ram_data *) dev->dev_data;
  return dd->size;
}

static bool mgos_vfs_dev_ram_close(struct mgos_vfs_dev *dev) {
  struct mgos_vfs_dev_ram_data *dd =
      (struct mgos_vfs_dev_ram_data *) dev->dev_data;
  free(dd->data);
  free(dd);
  return true;
}

static const struct mgos_vfs_dev_ops mgos_vfs_dev_ram_ops = {
    .open = mgos_vfs_dev_ram_open,
    .read = mgos_vfs_dev_ram_read,
    .write = mgos_vfs_dev_ram_write,
    .erase = mgos_vfs_dev_ram_erase,
    .get_size = mgos_vfs_dev_ram_get_size,
    .close = mgos_vfs_dev_ram_close,
};

bool mgos_vfs_dev_ram_register_type(void) {
  return mgos_vfs_dev_register_type(MGOS_VFS_DEV_TYPE_RAM,
                                    &mgos_vfs_dev_ram_ops);
}
