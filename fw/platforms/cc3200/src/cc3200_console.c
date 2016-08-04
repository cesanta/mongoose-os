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
#include "fw/platforms/cc3200/src/config.h"

void cc3200_console_putc(int fd, char c) {
  MAP_UARTCharPut(CONSOLE_UART, c);
  (void) fd;
}
