/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "cc32xx_exc.h"

#include <stdio.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/uart.h"

#include "cc32xx_uart.h"
#include "mgos_hal.h"
#include "mgos_sys_config.h"

#ifdef CC32XX_LOOP_ON_EXCEPTION
#define EXC_ACTION \
  while (1) {      \
  }
#else
#define EXC_ACTION mgos_dev_system_restart();
#endif

#define ARM_PERIPH_BASE 0xE000E000

#define SCB_SYSHNDCTRL (ARM_PERIPH_BASE + 0xD24)
#define CTRL_USAGE_FAULT_ENA (1 << 18)
#define CTRL_BUS_FAULT_ENA (1 << 17)
#define CTRL_MEM_FAULT_ENA (1 << 16)

#define SCB_FAULTSTAT (ARM_PERIPH_BASE + 0xD28)
#define SCB_HFAULTSTAT (ARM_PERIPH_BASE + 0xD2C)
#define SCB_FAULTADDR (ARM_PERIPH_BASE + 0xD38)

void cc32xx_exc_puts(const char *s) {
  int uart_no = (mgos_sys_config_is_initialized()
                     ? mgos_sys_config_get_debug_stderr_uart()
                     : MGOS_DEBUG_UART);
  if (uart_no < 0) return;
  uint32_t base = cc32xx_uart_get_base(uart_no);
  for (; *s != '\0'; s++) {
    MAP_UARTCharPut(base, *s);
  }
  while (MAP_UARTBusy(base)) {
  }
}

void cc32xx_exc_printf(const char *fmt, ...) {
  char buf[100];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  cc32xx_exc_puts(buf);
}

void handle_exception(struct cc32xx_exc_frame *f, const char *type) {
  cc32xx_exc_printf(
      "\n\n--- %s Fault ---\n"
      "  SHCTL=0x%08x, FSTAT=0x%08x, HFSTAT=0x%08x, FADDR=%08x\n",
      type, (unsigned int) HWREG(SCB_SYSHNDCTRL),
      (unsigned int) HWREG(SCB_FAULTSTAT), (unsigned int) HWREG(SCB_HFAULTSTAT),
      (unsigned int) HWREG(SCB_FAULTADDR));
  cc32xx_exc_printf(
      "  SF @ 0x%08x:\n    R0=0x%08x R1=0x%08x R2=0x%08x R3=0x%08x "
      "R12=0x%08x\n",
      (unsigned int) f, f->r0, f->r1, f->r2, f->r3, f->r12);
  cc32xx_exc_printf("    LR=0x%08x PC=0x%08x xPSR=0x%08x\n---\n", f->lr, f->pc,
                    f->xpsr);
  EXC_ACTION
}

void cc32xx_nmi_handler(void) {
  cc32xx_exc_puts("\n\n--- NMI ---\n");
  EXC_ACTION
}

void cc32xx_hard_fault_handler_bottom(struct cc32xx_exc_frame *f) {
  handle_exception(f, "Hard");
}

void cc32xx_mem_fault_handler_bottom(struct cc32xx_exc_frame *f) {
  handle_exception(f, "Mem");
}

void cc32xx_bus_fault_handler_bottom(struct cc32xx_exc_frame *f) {
  handle_exception(f, "Bus");
}

void cc32xx_usage_fault_handler_bottom(struct cc32xx_exc_frame *f) {
  handle_exception(f, "Usage");
}

void cc32xx_unhandled_int(void) {
  cc32xx_exc_puts("\n\n--- Unhandled int ---\n");
  EXC_ACTION
}

void vMainAssertCalled(const char *file, uint32_t line) {
  cc32xx_exc_printf("Assert at %s:%u\r\n", file, line);
  mgos_system_restart();
}

void vAssertCalled(const char *file, uint32_t line) {
  vMainAssertCalled(file, line);
}

void cc32xx_exc_init(void) {
  cc32xx_uart_early_init(MGOS_DEBUG_UART, MGOS_DEBUG_UART_BAUD_RATE);
  HWREG(SCB_SYSHNDCTRL) |=
      (CTRL_USAGE_FAULT_ENA | CTRL_BUS_FAULT_ENA | CTRL_MEM_FAULT_ENA);
}
