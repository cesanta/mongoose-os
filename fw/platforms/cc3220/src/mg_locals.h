/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3220_SRC_MG_LOCALS_H_
#define CS_FW_PLATFORMS_CC3220_SRC_MG_LOCALS_H_

#include "mgos_vfs.h"

#include "mgos_debug.h"
#define MG_UART_WRITE(fd, buf, len) mgos_debug_write(fd, buf, len)

#ifdef __cplusplus
extern "C" {
#endif

void fs_slfs_set_new_file_size(const char *name, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC3220_SRC_MG_LOCALS_H_ */
