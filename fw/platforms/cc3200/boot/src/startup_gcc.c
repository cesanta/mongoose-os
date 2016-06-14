/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <inttypes.h>

void main(void);

extern uint32_t _bss_start;
extern uint32_t _bss_end;

void ResetISR(void) {
  uint32_t *start = &_bss_start;
  uint32_t *end = &_bss_end;
  while (start < end) *start++ = 0;
  main();
}

static void IntDefaultHandler(void) {
  while (1) {
  }
}

/* clang-format off */
__attribute__ ((section(".intvecs")))
void (* const int_vectors[256])(void) = {
  (void (*)(void)) &int_vectors,          /* The initial stack pointer */
  ResetISR,                               /* The reset handler */
  IntDefaultHandler,                      /* The NMI handler */
  IntDefaultHandler,                      /* The hard fault handler */
  IntDefaultHandler,                      /* The MPU fault handler */
  IntDefaultHandler,                      /* The bus fault handler */
  IntDefaultHandler,                      /* The usage fault handler */
  0,                                      /* Reserved */
  0,                                      /* Reserved */
  0,                                      /* Reserved */
  0,                                      /* Reserved */
  IntDefaultHandler,                      /* SVCall handler */
  IntDefaultHandler,                      /* Debug monitor handler */
  0,                                      /* Reserved */
  IntDefaultHandler,                      /* The PendSV handler */
  IntDefaultHandler,                      /* The SysTick handler */
  IntDefaultHandler,                      /* GPIO Port A0 */
  IntDefaultHandler,                      /* GPIO Port A1 */
  IntDefaultHandler,                      /* GPIO Port A2 */
  IntDefaultHandler,                      /* GPIO Port A3 */
  0,                                      /* Reserved */
  IntDefaultHandler,                      /* UART0 Rx and Tx */
  IntDefaultHandler,                      /* UART1 Rx and Tx */
  0,                                      /* Reserved */
  IntDefaultHandler,                      /* I2C0 Master and Slave */
  0,0,0,0,0,                              /* Reserved */
  IntDefaultHandler,                      /* ADC Channel 0 */
  IntDefaultHandler,                      /* ADC Channel 1 */
  IntDefaultHandler,                      /* ADC Channel 2 */
  IntDefaultHandler,                      /* ADC Channel 3 */
  IntDefaultHandler,                      /* Watchdog Timer */
  IntDefaultHandler,                      /* Timer 0 subtimer A */
  IntDefaultHandler,                      /* Timer 0 subtimer B */
  IntDefaultHandler,                      /* Timer 1 subtimer A */
  IntDefaultHandler,                      /* Timer 1 subtimer B */
  IntDefaultHandler,                      /* Timer 2 subtimer A */
  IntDefaultHandler,                      /* Timer 2 subtimer B  */
  0,0,0,0,                                /* Reserved */
  IntDefaultHandler,                      /* Flash */
  0,0,0,0,0,                              /* Reserved */
  IntDefaultHandler,                      /* Timer 3 subtimer A */
  IntDefaultHandler,                      /* Timer 3 subtimer B */
  0,0,0,0,0,0,0,0,0,                      /* Reserved */
  IntDefaultHandler,                      /* uDMA Software Transfer */
  IntDefaultHandler,                      /* uDMA Error */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  IntDefaultHandler,                      /* SHA */
  0,0,                                    /* Reserved */
  IntDefaultHandler,                      /* AES */
  0,                                      /* Reserved */
  IntDefaultHandler,                      /* DES */
  0,0,0,0,0,                              /* Reserved */
  IntDefaultHandler,                      /* SDHost */
  0,                                      /* Reserved */
  IntDefaultHandler,                      /* I2S */
  0,                                      /* Reserved */
  IntDefaultHandler,                      /* Camera */
  0,0,0,0,0,0,0,                          /* Reserved */
  IntDefaultHandler,                      /* NWP to APPS Interrupt */
  IntDefaultHandler,                      /* Power, Reset and Clock module */
  0,0,                                    /* Reserved */
  IntDefaultHandler,                      /* Shared SPI */
  IntDefaultHandler,                      /* Generic SPI */
  IntDefaultHandler,                      /* Link SPI */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0,0,0,0,0,0,0,0,0,                    /* Reserved */
  0,0                                     /* Reserved */
};
