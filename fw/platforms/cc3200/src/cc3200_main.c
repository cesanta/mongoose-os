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
#include "inc/hw_types.h"

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin.h"
#include "driverlib/prcm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/utils.h"
#include "driverlib/wdt.h"

#include "common/platform.h"
#include "common/cs_dbg.h"

#include "simplelink.h"
#include "device.h"

#include "oslib/osi.h"

#include "fw/src/miot_app.h"
#include "fw/src/miot_hal.h"

#include "fw/platforms/cc3200/src/config.h"
#include "fw/platforms/cc3200/src/cc3200_exc.h"
#include "fw/platforms/cc3200/src/cc3200_main_task.h"

/* These are FreeRTOS hooks for various life situations. */
void vApplicationMallocFailedHook(void) {
  fprintf(stderr, "malloc failed\n");
  exit(123);
}

void vApplicationIdleHook(void) {
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
}

int main(void) {
  MAP_IntVTableBaseSet((unsigned long) &g_pfnVectors[0]);
  cc3200_exc_init();

  /* Early init app hook. */
  miot_app_preinit();

  MAP_IntEnable(FAULT_SYSTICK);
  MAP_IntMasterEnable();
  PRCMCC3200MCUInit();
  MAP_PRCMPeripheralClkEnable(PRCM_WDT, PRCM_RUN_MODE_CLK);

  miot_wdt_set_timeout(5 /* seconds */);
  miot_wdt_enable();

#ifdef __TI_COMPILER_VERSION__
  memset(&_heap_start, 0, (char *) &_heap_end - (char *) &_heap_start);
#endif

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
