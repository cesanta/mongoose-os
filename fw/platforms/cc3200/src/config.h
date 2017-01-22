/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CONFIG_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CONFIG_H_

#include "hw_types.h"
#include "pin.h"

#define SYS_CLK 80000000

#define WIFI_SCAN_INTERVAL_SECONDS 15

#ifndef MGOS_DEBUG_UART
#define MGOS_DEBUG_UART 0
#endif
#ifndef MGOS_DEBUG_UART_BAUD_RATE
#define MGOS_DEBUG_UART_BAUD_RATE 115200
#endif

#define CC3200_POLL_LENGTH_MS 2
#define CC3200_MAIN_TASK_STACK_SIZE 8192

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CONFIG_H_ */
