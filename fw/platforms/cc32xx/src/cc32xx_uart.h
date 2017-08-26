/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_UART_H_
#define CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_UART_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mgos_uart_dev_config {
/* TODO(rojer) */
#if 0
  int8_t tx_pin;
  int8_t rx_pin;
  int8_t cts_pin;
  int8_t rts_pin;
#endif
};

uint32_t cc32xx_uart_get_base(int uart_no);
bool cc32xx_uart_cts(int uart_no);
uint32_t cc32xx_uart_raw_ints(int uart_no);
uint32_t cc32xx_uart_int_mask(int uart_no);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_UART_H_ */
