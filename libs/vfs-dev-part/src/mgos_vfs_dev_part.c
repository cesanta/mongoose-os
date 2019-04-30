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

#include "mgos_vfs_dev_part.h"

#include "common/cs_dbg.h"

#include "frozen.h"

#include "mgos_vfs_dev.h"

struct mgos_vfs_dev_part_data {
  struct mgos_vfs_dev *io_dev;
  unsigned long offset, size;
};

enum mgos_vfs_dev_err part_dev_init(struct mgos_vfs_dev *dev,
                                    struct mgos_vfs_dev *io_dev, size_t offset,
                                    size_t size) {
  size_t dev_size;
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_INVAL;
  struct mgos_vfs_dev_part_data *dd =
      (struct mgos_vfs_dev_part_data *) calloc(1, sizeof(*dd));
  if (dd == NULL) {
    res = MGOS_VFS_DEV_ERR_NOMEM;
    goto out;
  }
  dd->io_dev = io_dev;
  dd->offset = offset;
  dd->size = size;
  dev_size = mgos_vfs_dev_get_size(dd->io_dev);
  if (dd->offset > dev_size || dd->offset + dd->size > dev_size) {
    LOG(LL_ERROR,
        ("Invalid size/offset (dev size %lu)", (unsigned long) dev_size));
    goto out;
  }
  if (dd->size == 0) {
    dd->size = dev_size - dd->offset;
  }
  dd->io_dev->refs++;
  dev->dev_data = dd;
  res = MGOS_VFS_DEV_ERR_NONE;
out:
  if (res != MGOS_VFS_DEV_ERR_NONE) free(dd);
  return res;
}

enum mgos_vfs_dev_err mgos_vfs_dev_part_open(struct mgos_vfs_dev *dev,
                                             const char *opts) {
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_INVAL;
  unsigned long offset = 0, size = 0;
  char *dev_name = NULL;
  struct mgos_vfs_dev *io_dev = NULL;
  json_scanf(opts, strlen(opts), "{dev: %Q, offset: %lu, size: %lu}", &dev_name,
             &offset, &size);
  if (dev_name == NULL) {
    LOG(LL_ERROR, ("Name is required"));
    goto out;
  }
  io_dev = mgos_vfs_dev_open(dev_name);
  if (io_dev == NULL) {
    LOG(LL_ERROR, ("Unable to open %s", dev_name));
    goto out;
  }
  res = part_dev_init(dev, io_dev, offset, size);

out:
  mgos_vfs_dev_close(io_dev);
  free(dev_name);
  return res;
}

static enum mgos_vfs_dev_err mgos_vfs_dev_part_read(struct mgos_vfs_dev *dev,
                                                    size_t offset, size_t len,
                                                    void *dst) {
  struct mgos_vfs_dev_part_data *dd =
      (struct mgos_vfs_dev_part_data *) dev->dev_data;
  if (len > dd->size || offset + len > dd->size) return MGOS_VFS_DEV_ERR_INVAL;
  return mgos_vfs_dev_read(dd->io_dev, dd->offset + offset, len, dst);
}

static enum mgos_vfs_dev_err mgos_vfs_dev_part_write(struct mgos_vfs_dev *dev,
                                                     size_t offset, size_t len,
                                                     const void *src) {
  struct mgos_vfs_dev_part_data *dd =
      (struct mgos_vfs_dev_part_data *) dev->dev_data;
  if (len > dd->size || offset + len > dd->size) return MGOS_VFS_DEV_ERR_INVAL;
  return mgos_vfs_dev_write(dd->io_dev, dd->offset + offset, len, src);
}

static enum mgos_vfs_dev_err mgos_vfs_dev_part_erase(struct mgos_vfs_dev *dev,
                                                     size_t offset,
                                                     size_t len) {
  struct mgos_vfs_dev_part_data *dd =
      (struct mgos_vfs_dev_part_data *) dev->dev_data;
  if (len > dd->size || offset + len > dd->size) return MGOS_VFS_DEV_ERR_INVAL;
  return mgos_vfs_dev_erase(dd->io_dev, dd->offset + offset, len);
}

static size_t mgos_vfs_dev_part_get_size(struct mgos_vfs_dev *dev) {
  struct mgos_vfs_dev_part_data *dd =
      (struct mgos_vfs_dev_part_data *) dev->dev_data;
  return dd->size;
}

static enum mgos_vfs_dev_err mgos_vfs_dev_part_close(struct mgos_vfs_dev *dev) {
  struct mgos_vfs_dev_part_data *dd =
      (struct mgos_vfs_dev_part_data *) dev->dev_data;
  enum mgos_vfs_dev_err res =
      (mgos_vfs_dev_close(dd->io_dev) ? MGOS_VFS_DEV_ERR_NONE
                                      : MGOS_VFS_DEV_ERR_IO);
  free(dd);
  return res;
}

static enum mgos_vfs_dev_err mgos_vfs_dev_part_get_erase_sizes(
    struct mgos_vfs_dev *dev, size_t sizes[MGOS_VFS_DEV_NUM_ERASE_SIZES]) {
  struct mgos_vfs_dev_part_data *dd =
      (struct mgos_vfs_dev_part_data *) dev->dev_data;
  return mgos_vfs_dev_get_erase_sizes(dd->io_dev, sizes);
}

static const struct mgos_vfs_dev_ops mgos_vfs_dev_part_ops = {
    .open = mgos_vfs_dev_part_open,
    .read = mgos_vfs_dev_part_read,
    .write = mgos_vfs_dev_part_write,
    .erase = mgos_vfs_dev_part_erase,
    .get_size = mgos_vfs_dev_part_get_size,
    .close = mgos_vfs_dev_part_close,
    .get_erase_sizes = mgos_vfs_dev_part_get_erase_sizes,
};

bool mgos_vfs_dev_part_init(void) {
  return mgos_vfs_dev_register_type(MGOS_VFS_DEV_TYPE_PART,
                                    &mgos_vfs_dev_part_ops);
}
