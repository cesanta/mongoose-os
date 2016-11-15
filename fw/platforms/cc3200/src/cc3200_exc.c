#include "fw/platforms/cc3200/src/cc3200_exc.h"

#include <stdio.h>

#include "hw_types.h"
#include "hw_memmap.h"
#include "rom.h"
#include "rom_map.h"
#include "uart.h"

#include "fw/platforms/cc3200/src/cc3200_uart.h"
#include "fw/platforms/cc3200/src/config.h"
#include "fw/src/miot_sys_config.h"

#define ARM_PERIPH_BASE 0xE000E000

#define SCB_SYSHNDCTRL (ARM_PERIPH_BASE + 0xD24)
#define CTRL_USAGE_FAULT_ENA (1 << 18)
#define CTRL_BUS_FAULT_ENA (1 << 17)
#define CTRL_MEM_FAULT_ENA (1 << 16)

#define SCB_FAULTSTAT (ARM_PERIPH_BASE + 0xD28)
#define SCB_HFAULTSTAT (ARM_PERIPH_BASE + 0xD2C)
#define SCB_FAULTADDR (ARM_PERIPH_BASE + 0xD38)

void uart_puts(const char *s) {
  int uart_no = (get_cfg() ? get_cfg()->debug.stderr_uart : MIOT_DEBUG_UART);
  if (uart_no < 0) return;
  uint32_t base = cc3200_uart_get_base(uart_no);
  for (; *s != '\0'; s++) {
    MAP_UARTCharPut(base, *s);
  }
}

void handle_exception(struct exc_frame *f, const char *type) {
  char buf[100];
  snprintf(buf, sizeof(buf),
           "\n\n--- %s Fault ---\n"
           "  SHCTL=0x%08x, FSTAT=0x%08x, HFSTAT=0x%08x, FADDR=%08x\n",
           type, (unsigned int) HWREG(SCB_SYSHNDCTRL),
           (unsigned int) HWREG(SCB_FAULTSTAT),
           (unsigned int) HWREG(SCB_HFAULTSTAT),
           (unsigned int) HWREG(SCB_FAULTADDR));
  uart_puts(buf);
  snprintf(buf, sizeof(buf),
           "  SF @ 0x%08x:\n    R0=0x%08x R1=0x%08x R2=0x%08x R3=0x%08x "
           "R12=0x%08x\n",
           (unsigned int) f, f->r0, f->r1, f->r2, f->r3, f->r12);
  uart_puts(buf);
  snprintf(buf, sizeof(buf), "    LR=0x%08x PC=0x%08x xPSR=0x%08x\n---\n",
           f->lr, f->pc, f->xpsr);
  uart_puts(buf);
}

void cc3200_exc_init(void) {
  HWREG(SCB_SYSHNDCTRL) |=
      (CTRL_USAGE_FAULT_ENA | CTRL_BUS_FAULT_ENA | CTRL_MEM_FAULT_ENA);
}
