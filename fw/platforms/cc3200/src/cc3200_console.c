#include "fw/platforms/cc3200/src/cc3200_console.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/uart.h>

#include "fw/src/miot_uart.h"

static int8_t s_stdout_uart = MIOT_DEBUG_UART;
static int8_t s_stderr_uart = MIOT_DEBUG_UART;

void cc3200_console_putc(int fd, char c) {
  int uart_no = -1;
  if (fd == 1) {
    uart_no = s_stdout_uart;
  } else if (fd == 2) {
    uart_no = s_stderr_uart;
  }
  if (uart_no >= 0) miot_uart_write(uart_no, &c, 1);
}

enum miot_init_result miot_set_stdout_uart(int uart_no) {
  enum miot_init_result r = miot_init_debug_uart(uart_no);
  if (r == MIOT_INIT_OK) {
    s_stdout_uart = uart_no;
  }
  return r;
}

enum miot_init_result miot_set_stderr_uart(int uart_no) {
  enum miot_init_result r = miot_init_debug_uart(uart_no);
  if (r == MIOT_INIT_OK) {
    s_stderr_uart = uart_no;
  }
  return r;
}

enum miot_init_result cc3200_console_init() {
  return miot_init_debug_uart(MIOT_DEBUG_UART);
}
