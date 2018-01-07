/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "mgos_core_dump.h"

#include "stm32_uart.h"

/*
 * There isn't much here, and here's why.
 * 1) Exceptions are routed to arm_exc_handler_top, which is written in asm,
 *    determines the stack pointer, populates most of the GDB frame and...
 * 2) ...hands off to arm_exc_handler_bottom, which prints exception info,
 *    dumps core and reboots.
 *
 * Platform code only needs to provide character printing and low-level reboot
 *functions.
 */

void mgos_cd_putc(int c) {
  stm32_uart_dputc(c);
}
