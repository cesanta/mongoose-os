/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_MGOS_LOCALS_H_
#define CS_FW_PLATFORMS_CC3200_SRC_MGOS_LOCALS_H_

#include "fw/platforms/cc3200/src/cc3200_fs_spiffs.h"

#include "fw/src/mgos_debug.h"
#define MG_UART_WRITE(fd, buf, len) mgos_debug_write(fd, buf, len)

#endif /* CS_FW_PLATFORMS_CC3200_SRC_MGOS_LOCALS_H_ */
