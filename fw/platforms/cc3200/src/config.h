/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_CONFIG_H_
#define CS_FW_PLATFORMS_CC3200_CONFIG_H_

#include "hw_types.h"
#include "pin.h"

#define SYS_CLK 80000000
#define CONSOLE_BAUD_RATE 115200
#define CONSOLE_UART UARTA0_BASE
#define CONSOLE_UART_INT INT_UARTA0
#define CONSOLE_UART_PERIPH PRCM_UARTA0

/* TODO(rojer): Make runtime-configurable. */
#define I2C_SDA_PIN PIN_02
#define I2C_SCL_PIN PIN_01

#define WIFI_SCAN_INTERVAL_SECONDS 15

#define V7_POLL_LENGTH_MS 2

#endif /* CS_FW_PLATFORMS_CC3200_CONFIG_H_ */
