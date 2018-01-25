/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_VFS_DEV_RAM_H_
#define CS_FW_SRC_MGOS_VFS_DEV_RAM_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A simple heap-backed VFS device, mostly for testing. */

#define MGOS_VFS_DEV_TYPE_RAM "RAM"

bool mgos_vfs_dev_ram_register_type(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_SRC_MGOS_VFS_DEV_RAM_H_ */
