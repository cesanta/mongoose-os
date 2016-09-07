#include "fw/platforms/cc3200/src/cc3200_console.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/uart.h>

#include "fw/src/sj_sys_config.h"
#include "fw/src/mg_uart.h"
#include "fw/platforms/cc3200/src/config.h"

void cc3200_console_putc(int fd, char c) {
  struct sys_config *scfg = get_cfg();
  int uart_no = -1;
  if (fd == 1) {
    uart_no = scfg ? scfg->debug.stdout_uart : MG_DEBUG_UART;
  } else if (fd == 2) {
    uart_no = scfg ? scfg->debug.stderr_uart : MG_DEBUG_UART;
  }
  if (uart_no < 0) return;
  mg_uart_write(uart_no, &c, 1);
}
