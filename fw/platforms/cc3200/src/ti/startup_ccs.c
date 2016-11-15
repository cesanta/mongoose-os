#include <inttypes.h>

#include "fw/platforms/cc3200/src/cc3200_exc.h"
#include "fw/src/miot_hal.h"

/* From exc_handler_top.asm */
extern void hard_fault_handler_top(void);
extern void mem_fault_handler_top(void);
extern void bus_fault_handler_top(void);
extern void usage_fault_handler_top(void);

#ifdef CC3200_LOOP_ON_EXCEPTION
#define EXC_ACTION \
  while (1) {      \
  }
#else
#define EXC_ACTION miot_system_restart(100);
#endif

extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);

/* Linker variable that marks the top of the stack. */
extern unsigned long __STACK_END;

void ResetISR(void) {
  __asm(
      "  .global _c_int00\n"
      "  b.w     _c_int00");
}

static void NMIHandler(void) {
  uart_puts("\n\n--- NMI ---\n");
  EXC_ACTION
}

void hard_fault_handler_bottom(struct exc_frame *f) {
  handle_exception(f, "Hard");
  EXC_ACTION
}

void mem_fault_handler_bottom(struct exc_frame *f) {
  handle_exception(f, "Mem");
  EXC_ACTION
}

void bus_fault_handler_bottom(struct exc_frame *f) {
  handle_exception(f, "Bus");
  EXC_ACTION
}

void usage_fault_handler_bottom(struct exc_frame *f) {
  handle_exception(f, "Usage");
  EXC_ACTION
}

static void IntDefaultHandler(void) {
  uart_puts("\n\n--- Unhandled int ---\n");
  EXC_ACTION
}
#pragma DATA_SECTION(g_pfnVectors, ".intvecs")
void (*const g_pfnVectors[256])(void) = {
    (void (*)(void))(&__STACK_END), /* The initial stack pointer */
    ResetISR,                       /* The reset handler */
    NMIHandler,                     /* The NMI handler */
    hard_fault_handler_top,         /* The hard fault handler */
    mem_fault_handler_top,          /* The MPU fault handler */
    bus_fault_handler_top,          /* The hard fault handler */
    usage_fault_handler_top,        /* The usage fault handler */
    0,                              /* Reserved */
    0,                              /* Reserved */
    0,                              /* Reserved */
    0,                              /* Reserved */
    vPortSVCHandler,                /* SVCall handler */
    IntDefaultHandler,              /* Debug monitor handler */
    0,                              /* Reserved */
    xPortPendSVHandler,             /* The PendSV handler */
    xPortSysTickHandler,            /* The SysTick handler */
    IntDefaultHandler,              /* GPIO Port A0 */
    IntDefaultHandler,              /* GPIO Port A1 */
    IntDefaultHandler,              /* GPIO Port A2 */
    IntDefaultHandler,              /* GPIO Port A3 */
    0,                              /* Reserved */
    IntDefaultHandler,              /* UART0 Rx and Tx */
    IntDefaultHandler,              /* UART1 Rx and Tx */
    0,                              /* Reserved */
    IntDefaultHandler,              /* I2C0 Master and Slave */
    0, 0, 0, 0, 0,                  /* Reserved */
    IntDefaultHandler,              /* ADC Channel 0 */
    IntDefaultHandler,              /* ADC Channel 1 */
    IntDefaultHandler,              /* ADC Channel 2 */
    IntDefaultHandler,              /* ADC Channel 3 */
    IntDefaultHandler,              /* Watchdog Timer */
    IntDefaultHandler,              /* Timer 0 subtimer A */
    IntDefaultHandler,              /* Timer 0 subtimer B */
    IntDefaultHandler,              /* Timer 1 subtimer A */
    IntDefaultHandler,              /* Timer 1 subtimer B */
    IntDefaultHandler,              /* Timer 2 subtimer A */
    IntDefaultHandler,              /* Timer 2 subtimer B  */
    0, 0, 0, 0,                     /* Reserved */
    IntDefaultHandler,              /* Flash */
    0, 0, 0, 0, 0,                  /* Reserved */
    IntDefaultHandler,              /* Timer 3 subtimer A */
    IntDefaultHandler,              /* Timer 3 subtimer B */
    0, 0, 0, 0, 0, 0, 0, 0, 0,      /* Reserved */
    IntDefaultHandler,              /* uDMA Software Transfer */
    IntDefaultHandler,              /* uDMA Error */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    IntDefaultHandler,              /* SHA */
    0, 0,                           /* Reserved */
    IntDefaultHandler,              /* AES */
    0,                              /* Reserved */
    IntDefaultHandler,              /* DES */
    0, 0, 0, 0, 0,                  /* Reserved */
    IntDefaultHandler,              /* SDHost */
    0,                              /* Reserved */
    IntDefaultHandler,              /* I2S */
    0,                              /* Reserved */
    IntDefaultHandler,              /* Camera */
    0, 0, 0, 0, 0, 0, 0,            /* Reserved */
    IntDefaultHandler,              /* NWP to APPS Interrupt */
    IntDefaultHandler,              /* Power, Reset and Clock module */
    0, 0,                           /* Reserved */
    IntDefaultHandler,              /* Shared SPI */
    IntDefaultHandler,              /* Generic SPI */
    IntDefaultHandler,              /* Link SPI */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0                            /* Reserved */
};
