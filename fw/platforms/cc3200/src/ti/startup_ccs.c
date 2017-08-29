/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdint.h>

#include "cc32xx_exc.h"
#include "mgos_hal.h"

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

#pragma DATA_SECTION(g_pfnVectors, ".intvecs")
void (*const g_pfnVectors[256])(void) = {
    (void (*)(void))(&__STACK_END), /* The initial stack pointer */
    ResetISR,                       /* The reset handler */
    cc32xx_nmi_handler,             /* The NMI handler */
    cc32xx_hard_fault_handler_top,  /* The hard fault handler */
    cc32xx_mem_fault_handler_top,   /* The MPU fault handler */
    cc32xx_bus_fault_handler_top,   /* The hard fault handler */
    cc32xx_usage_fault_handler_top, /* The usage fault handler */
    0,                              /* Reserved */
    0,                              /* Reserved */
    0,                              /* Reserved */
    0,                              /* Reserved */
    vPortSVCHandler,                /* SVCall handler */
    cc32xx_unhandled_int,           /* Debug monitor handler */
    0,                              /* Reserved */
    xPortPendSVHandler,             /* The PendSV handler */
    xPortSysTickHandler,            /* The SysTick handler */
    cc32xx_unhandled_int,           /* GPIO Port A0 */
    cc32xx_unhandled_int,           /* GPIO Port A1 */
    cc32xx_unhandled_int,           /* GPIO Port A2 */
    cc32xx_unhandled_int,           /* GPIO Port A3 */
    0,                              /* Reserved */
    cc32xx_unhandled_int,           /* UART0 Rx and Tx */
    cc32xx_unhandled_int,           /* UART1 Rx and Tx */
    0,                              /* Reserved */
    cc32xx_unhandled_int,           /* I2C0 Master and Slave */
    0, 0, 0, 0, 0,                  /* Reserved */
    cc32xx_unhandled_int,           /* ADC Channel 0 */
    cc32xx_unhandled_int,           /* ADC Channel 1 */
    cc32xx_unhandled_int,           /* ADC Channel 2 */
    cc32xx_unhandled_int,           /* ADC Channel 3 */
    cc32xx_unhandled_int,           /* Watchdog Timer */
    cc32xx_unhandled_int,           /* Timer 0 subtimer A */
    cc32xx_unhandled_int,           /* Timer 0 subtimer B */
    cc32xx_unhandled_int,           /* Timer 1 subtimer A */
    cc32xx_unhandled_int,           /* Timer 1 subtimer B */
    cc32xx_unhandled_int,           /* Timer 2 subtimer A */
    cc32xx_unhandled_int,           /* Timer 2 subtimer B  */
    0, 0, 0, 0,                     /* Reserved */
    cc32xx_unhandled_int,           /* Flash */
    0, 0, 0, 0, 0,                  /* Reserved */
    cc32xx_unhandled_int,           /* Timer 3 subtimer A */
    cc32xx_unhandled_int,           /* Timer 3 subtimer B */
    0, 0, 0, 0, 0, 0, 0, 0, 0,      /* Reserved */
    cc32xx_unhandled_int,           /* uDMA Software Transfer */
    cc32xx_unhandled_int,           /* uDMA Error */
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
    cc32xx_unhandled_int,           /* SHA */
    0, 0,                           /* Reserved */
    cc32xx_unhandled_int,           /* AES */
    0,                              /* Reserved */
    cc32xx_unhandled_int,           /* DES */
    0, 0, 0, 0, 0,                  /* Reserved */
    cc32xx_unhandled_int,           /* SDHost */
    0,                              /* Reserved */
    cc32xx_unhandled_int,           /* I2S */
    0,                              /* Reserved */
    cc32xx_unhandled_int,           /* Camera */
    0, 0, 0, 0, 0, 0, 0,            /* Reserved */
    cc32xx_unhandled_int,           /* NWP to APPS Interrupt */
    cc32xx_unhandled_int,           /* Power, Reset and Clock module */
    0, 0,                           /* Reserved */
    cc32xx_unhandled_int,           /* Shared SPI */
    cc32xx_unhandled_int,           /* Generic SPI */
    cc32xx_unhandled_int,           /* Link SPI */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0                            /* Reserved */
};
