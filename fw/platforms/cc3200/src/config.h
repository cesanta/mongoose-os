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

#ifndef MIOT_DEBUG_UART
#define MIOT_DEBUG_UART 0
#endif
#ifndef MIOT_DEBUG_UART_BAUD_RATE
#define MIOT_DEBUG_UART_BAUD_RATE 115200
#endif

#define V7_POLL_LENGTH_MS 2

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CONFIG_H_ */
