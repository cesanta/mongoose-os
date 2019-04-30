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

#include "mgos_vfs_dev_ram.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "common/cs_dbg.h"

#include "frozen.h"

#include "mgos_hal.h"
#include "mgos_vfs.h"
#include "mgos_vfs_dev.h"

struct mgos_vfs_dev_ram_data {
  size_t size;
  uint8_t *data;
  uint8_t own_data : 1;
  uint8_t flash_check : 1;
  uint8_t erase_byte;
};

static enum mgos_vfs_dev_err mgos_vfs_dev_ram_open(struct mgos_vfs_dev *dev,
                                                   const char *opts) {
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_INVAL;
  struct mgos_vfs_dev_ram_data *dd = NULL;
  uint8_t fb = 0xff;
  unsigned long addr = 0, size = 0;
  int flash_check = false, erase_byte = 0xff, fill_byte = 0xff;
  json_scanf(
      opts, strlen(opts),
      "{addr: %lu, size: %lu, erase_byte: %d, fill_byte: %d, flash_check: %B}",
      &addr, &size, &erase_byte, &fill_byte, &flash_check);
  if (size <= 0) {
    LOG(LL_ERROR, ("Size is required for RAM device"));
    goto out;
  }
  dd = (struct mgos_vfs_dev_ram_data *) calloc(1, sizeof(*dd));
  dd->size = size;
  dd->flash_check = flash_check;
  dd->erase_byte = (uint8_t) erase_byte;
  if (addr != 0) {
    dd->data = (uint8_t *) addr;
  } else {
    dd->data = (uint8_t *) malloc(dd->size);
    if (dd->data == NULL) {
      res = MGOS_VFS_DEV_ERR_NOMEM;
      goto out;
    }
    dd->own_data = true;
  }
  fb = (uint8_t) fill_byte;
  memset(dd->data, fb, dd->size);
  dev->dev_data = dd;
  res = MGOS_VFS_DEV_ERR_NONE;

out:
  if (res != 0 && dd != NULL) {
    free(dd->data);
    free(dd);
  } else {
    LOG(LL_INFO,
        ("%u bytes @ %p, eb 0x%02x, fb 0x%02x, fc %s", (unsigned) dd->size,
         dd->data, dd->erase_byte, fb, (dd->flash_check ? "yes" : "no")));
  }
  return res;
}

static enum mgos_vfs_dev_err mgos_vfs_dev_ram_read(struct mgos_vfs_dev *dev,
                                                   size_t offset, size_t len,
                                                   void *dst) {
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_INVAL;
  struct mgos_vfs_dev_ram_data *dd =
      (struct mgos_vfs_dev_ram_data *) dev->dev_data;
  if (len > dd->size || offset + len > dd->size) {
    goto out;
  }
  memcpy(dst, dd->data + offset, len);
  res = MGOS_VFS_DEV_ERR_NONE;

out:
  LOG((res == 0 ? LL_VERBOSE_DEBUG : LL_ERROR),
      ("%s %u @ 0x%x = %d", "read", (unsigned) len, (unsigned) offset, res));
  return res;
}

static enum mgos_vfs_dev_err mgos_vfs_dev_ram_write(struct mgos_vfs_dev *dev,
                                                    size_t offset, size_t len,
                                                    const void *src) {
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_INVAL;
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

  res = MGOS_VFS_DEV_ERR_NONE;

out:
  LOG((res == 0 ? LL_VERBOSE_DEBUG : LL_ERROR),
      ("%s %u @ 0x%x = %d", "write", (unsigned) len, (unsigned) offset, res));
  return res;
}

static enum mgos_vfs_dev_err mgos_vfs_dev_ram_erase(struct mgos_vfs_dev *dev,
                                                    size_t offset, size_t len) {
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_INVAL;
  struct mgos_vfs_dev_ram_data *dd =
      (struct mgos_vfs_dev_ram_data *) dev->dev_data;
  if (len > dd->size || offset + len > dd->size) {
    goto out;
  }
  memset(dd->data + offset, 0xff, len);
  res = MGOS_VFS_DEV_ERR_NONE;

out:
  LOG((res == 0 ? LL_VERBOSE_DEBUG : LL_ERROR),
      ("%s %u @ 0x%x = %d", "erase", (unsigned) len, (unsigned) offset, res));
  return res;
}

static size_t mgos_vfs_dev_ram_get_size(struct mgos_vfs_dev *dev) {
  struct mgos_vfs_dev_ram_data *dd =
      (struct mgos_vfs_dev_ram_data *) dev->dev_data;
  return dd->size;
}

static enum mgos_vfs_dev_err mgos_vfs_dev_ram_close(struct mgos_vfs_dev *dev) {
  struct mgos_vfs_dev_ram_data *dd =
      (struct mgos_vfs_dev_ram_data *) dev->dev_data;
  if (dd->own_data) free(dd->data);
  free(dd);
  return MGOS_VFS_DEV_ERR_NONE;
}

static enum mgos_vfs_dev_err mgos_vfs_dev_ram_get_erase_sizes(
    struct mgos_vfs_dev *dev, size_t sizes[MGOS_VFS_DEV_NUM_ERASE_SIZES]) {
  sizes[0] = 1;
  sizes[1] = 128;
  sizes[2] = 1024;
  (void) dev;
  return MGOS_VFS_DEV_ERR_NONE;
}

static const struct mgos_vfs_dev_ops mgos_vfs_dev_ram_ops = {
    .open = mgos_vfs_dev_ram_open,
    .read = mgos_vfs_dev_ram_read,
    .write = mgos_vfs_dev_ram_write,
    .erase = mgos_vfs_dev_ram_erase,
    .get_size = mgos_vfs_dev_ram_get_size,
    .close = mgos_vfs_dev_ram_close,
    .get_erase_sizes = mgos_vfs_dev_ram_get_erase_sizes,
};

bool mgos_vfs_dev_ram_init(void) {
  return mgos_vfs_dev_register_type(MGOS_VFS_DEV_TYPE_RAM,
                                    &mgos_vfs_dev_ram_ops);
}
