/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cc32xx_exc.h"

#include <stdio.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/uart.h"

#include "mgos_core_dump.h"
#include "mgos_debug.h"
#include "mgos_sys_config.h"

#include "cc32xx_uart.h"

#define ARM_PERIPH_BASE 0xE000E000

#define SCB_SYSHNDCTRL (ARM_PERIPH_BASE + 0xD24)
#define CTRL_USAGE_FAULT_ENA (1 << 18)
#define CTRL_BUS_FAULT_ENA (1 << 17)
#define CTRL_MEM_FAULT_ENA (1 << 16)

#define SCB_FAULTSTAT (ARM_PERIPH_BASE + 0xD28)
#define SCB_HFAULTSTAT (ARM_PERIPH_BASE + 0xD2C)
#define SCB_FAULTADDR (ARM_PERIPH_BASE + 0xD38)

void mgos_cd_putc(int c) {
  int uart_no = (mgos_sys_config_is_initialized()
                     ? mgos_sys_config_get_debug_stderr_uart()
                     : MGOS_DEBUG_UART);
  if (uart_no < 0) return;
  uint32_t base = cc32xx_uart_get_base(uart_no);
  MAP_UARTCharPut(base, c);
  while (MAP_UARTBusy(base)) {
  }
}

void cc32xx_exc_puts(const char *s) {
  for (; *s != '\0'; s++) mgos_cd_putc(*s);
}

void cc32xx_exc_printf(const char *fmt, ...) {
  char buf[100];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  cc32xx_exc_puts(buf);
}

void abort(void) {
  fflush(stdout);
  fflush(stderr);
  mgos_debug_flush();
  cc32xx_exc_printf("\nabort() called\n");
  // Executes an illegal instruction.
  *((int *) 0xfafafafa) = 0x123;
}

void vMainAssertCalled(const char *file, uint32_t line) {
  cc32xx_exc_printf("Assert at %s:%u\r\n", file, line);
  abort();
}

void vAssertCalled(const char *file, uint32_t line) {
  vMainAssertCalled(file, line);
}

void cc32xx_exc_init(void) {
  cc32xx_uart_early_init(MGOS_DEBUG_UART, MGOS_DEBUG_UART_BAUD_RATE);
  HWREG(SCB_SYSHNDCTRL) |=
      (CTRL_USAGE_FAULT_ENA | CTRL_BUS_FAULT_ENA | CTRL_MEM_FAULT_ENA);
}
