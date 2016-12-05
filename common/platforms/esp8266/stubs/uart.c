/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "uart.h"

#define UART_CLKDIV_26MHZ(B) (52000000 + B / 2) / B

#include "rom_functions.h"

void set_baud_rate(uint32_t uart_no, uint32_t baud_rate) {
  uart_div_modify(uart_no, UART_CLKDIV_26MHZ(baud_rate));
}
