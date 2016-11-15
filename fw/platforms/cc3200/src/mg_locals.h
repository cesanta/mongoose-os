/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_MIOT_LOCALS_H_
#define CS_FW_PLATFORMS_CC3200_SRC_MIOT_LOCALS_H_

#include "fw/platforms/cc3200/src/cc3200_fs_spiffs.h"

#include "fw/platforms/cc3200/src/cc3200_console.h"
#define MG_UART_CHAR_PUT(fd, c) cc3200_console_putc(fd, c)

#endif /* CS_FW_PLATFORMS_CC3200_SRC_MIOT_LOCALS_H_ */
