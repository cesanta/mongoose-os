/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_UART_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_UART_H_

#include <inttypes.h>
#include <stdbool.h>

uint32_t cc3200_uart_get_base(int uart_no);
bool cc3200_uart_cts(int uart_no);
uint32_t cc3200_uart_raw_ints(int uart_no);
uint32_t cc3200_uart_int_mask(int uart_no);

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_UART_H_ */
