/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_MG_LOCALS_H_
#define CS_FW_PLATFORMS_CC3200_SRC_MG_LOCALS_H_

#include "fw/src/mgos_vfs.h"

#include "fw/src/mgos_debug.h"
#define MG_UART_WRITE(fd, buf, len) mgos_debug_write(fd, buf, len)

void fs_slfs_set_new_file_size(const char *name, size_t size);

#endif /* CS_FW_PLATFORMS_CC3200_SRC_MG_LOCALS_H_ */
