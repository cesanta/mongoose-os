/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_INCLUDE_MGOS_VFS_DEV_H_
#define CS_FW_INCLUDE_MGOS_VFS_DEV_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mgos_vfs_dev {
  const struct mgos_vfs_dev_ops *ops;
  void *dev_data;
  int refs;
};

struct mgos_vfs_dev_ops {
  bool (*open)(struct mgos_vfs_dev *dev, const char *opts);
  bool (*read)(struct mgos_vfs_dev *dev, size_t offset, size_t len, void *dst);
  bool (*write)(struct mgos_vfs_dev *dev, size_t offset, size_t len,
                const void *src);
  bool (*erase)(struct mgos_vfs_dev *dev, size_t offset, size_t len);
  size_t (*get_size)(struct mgos_vfs_dev *dev);
  bool (*close)(struct mgos_vfs_dev *dev);
};

bool mgos_vfs_dev_register_type(const char *name,
                                const struct mgos_vfs_dev_ops *ops);

struct mgos_vfs_dev *mgos_vfs_dev_open(const char *name, const char *opts);

bool mgos_vfs_dev_close(struct mgos_vfs_dev *dev);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_INCLUDE_MGOS_VFS_DEV_H_ */
