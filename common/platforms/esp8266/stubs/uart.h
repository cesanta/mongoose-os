/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_ESP8266_STUBS_UART_H_
#define CS_COMMON_PLATFORMS_ESP8266_STUBS_UART_H_

#include <stdint.h>

void set_baud_rate(uint32_t uart_no, uint32_t baud_rate);

#define REG_UART_BASE(i) (0x60000000 + (i) *0xf00)
#define UART_FIFO_REG(i) (REG_UART_BASE(i) + 0x0)
#define UART_INT_ST_REG(i) (REG_UART_BASE(i) + 0x8)
#define UART_INT_ENA_REG(i) (REG_UART_BASE(i) + 0xC)
#define UART_RXFIFO_TOUT_INT_ENA (BIT(8))
#define UART_RXFIFO_FULL_INT_ENA (BIT(0))

#define UART_INT_CLR_REG(i) (REG_UART_BASE(i) + 0x10)

#define UART_STATUS_REG(i) (REG_UART_BASE(i) + 0x1C)

#define ETS_UART0_INUM ETS_UART_INUM

#endif /* CS_COMMON_PLATFORMS_ESP8266_STUBS_UART_H_ */
