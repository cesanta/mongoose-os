/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __TI_COMPILER_VERSION__
#include <unistd.h>
#endif

/* Driverlib includes */
#include "hw_types.h"

#include "hw_ints.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "pin.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "uart.h"
#include "utils.h"

#include "common/platform.h"
#include "common/cs_dbg.h"

#include "simplelink.h"
#include "device.h"

#include "oslib/osi.h"

#include "fw/platforms/cc3200/src/config.h"
#include "fw/platforms/cc3200/src/cc3200_exc.h"
#include "fw/platforms/cc3200/src/cc3200_main_task.h"
#include "fw/platforms/cc3200/src/cc3200_sj_hal.h"

/* These are FreeRTOS hooks for various life situations. */
void vApplicationMallocFailedHook() {
  fprintf(stderr, "malloc failed\n");
  exit(123);
}

void vApplicationIdleHook() {
  /* Ho-hum. Twiddling our thumbs. */
}

void vApplicationStackOverflowHook(OsiTaskHandle *th, signed char *tn) {
}

void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *e) {
}

/* Int vector table, defined in startup_gcc.c */
extern void (*const g_pfnVectors[])(void);

#ifdef __TI_COMPILER_VERSION__
__attribute__((section(".heap_start"))) uint32_t _heap_start;
__attribute__((section(".heap_end"))) uint32_t _heap_end;
#endif

void umm_oom_cb(size_t size, unsigned short int blocks_cnt) {
  (void) blocks_cnt;
  LOG(LL_ERROR, ("Failed to allocate %u", size));
  abort();
}

int main() {
  MAP_IntVTableBaseSet((unsigned long) &g_pfnVectors[0]);
  cc3200_exc_init();

  MAP_IntEnable(FAULT_SYSTICK);
  MAP_IntMasterEnable();
  PRCMCC3200MCUInit();

#ifdef __TI_COMPILER_VERSION__
  memset(&_heap_start, 0, (char *) &_heap_end - (char *) &_heap_start);
#endif

  /* Console UART init. */
  MAP_PRCMPeripheralClkEnable(CONSOLE_UART_PERIPH, PRCM_RUN_MODE_CLK);
  MAP_PinTypeUART(PIN_55, PIN_MODE_3); /* PIN_55 -> UART0_TX */
  MAP_PinTypeUART(PIN_57, PIN_MODE_3); /* PIN_57 -> UART0_RX */
  MAP_UARTConfigSetExpClk(
      CONSOLE_UART, MAP_PRCMPeripheralClockGet(CONSOLE_UART_PERIPH),
      CONSOLE_BAUD_RATE,
      (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  MAP_UARTFIFOLevelSet(CONSOLE_UART, UART_FIFO_TX1_8, UART_FIFO_RX4_8);
  MAP_UARTFIFOEnable(CONSOLE_UART);

  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IOLBF, 0);
  cs_log_set_level(LL_INFO);

  VStartSimpleLinkSpawnTask(8);
  osi_TaskCreate(main_task, (const signed char *) "main", V7_STACK_SIZE + 256,
                 NULL, 3, NULL);
  osi_start();

  return 0;
}

/* FreeRTOS assert() hook. */
void vAssertCalled(const char *pcFile, unsigned long ulLine) {
  // Handle Assert here
  while (1) {
  }
}

int sj_app_init(struct v7 *v7) __attribute__((weak));
int sj_app_init(struct v7 *v7) {
  (void) v7;
  return 1;
}
