/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_CONSOLE_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_CONSOLE_H_

#include "fw/src/miot_init.h"

void cc3200_console_putc(int fd, char c);

enum miot_init_result cc3200_console_init();

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_CONSOLE_H_ */
