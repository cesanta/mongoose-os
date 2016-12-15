/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef __TI_COMPILER_VERSION__
#include <malloc.h>
#endif
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"

#include "driverlib/prcm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/wdt.h"

#include "common/platform.h"
#include "common/cs_dbg.h"

#include "simplelink.h"
#include "device.h"
#include "oslib/osi.h"

#include "fw/src/miot_hal.h"
#include "fw/src/miot_v7_ext.h"

#include "fw/platforms/cc3200/src/config.h"
#include "fw/platforms/cc3200/src/cc3200_fs.h"

#include "common/umm_malloc/umm_malloc.h"

#ifdef __TI_COMPILER_VERSION__

size_t miot_get_heap_size(void) {
  return UMM_MALLOC_CFG__HEAP_SIZE;
}

size_t miot_get_free_heap_size(void) {
  return umm_free_heap_size();
}

size_t miot_get_min_free_heap_size(void) {
  return umm_min_free_heap_size();
}

#else

/* Defined in linker script. */
extern unsigned long _heap;
extern unsigned long _eheap;

size_t miot_get_heap_size(void) {
  return ((char *) &_eheap - (char *) &_heap);
}

size_t miot_get_free_heap_size(void) {
  size_t avail = miot_get_heap_size();
  struct mallinfo mi = mallinfo();
  avail -= mi.arena;    /* Claimed by allocator. */
  avail += mi.fordblks; /* Free in the area claimed by allocator. */
  return avail;
}

size_t miot_get_min_free_heap_size(void) {
  /* Not supported */
  return 0;
}

#endif

size_t miot_get_fs_memory_usage(void) {
  return 0; /* Not even sure if it's possible to tell. */
}

void miot_wdt_feed(void) {
  MAP_WatchdogIntClear(WDT_BASE);
}

void miot_wdt_set_timeout(int secs) {
  MAP_WatchdogUnlock(WDT_BASE);
  /* Reset is triggered after the timer reaches zero for the second time. */
  MAP_WatchdogReloadSet(WDT_BASE, secs * SYS_CLK / 2);
  MAP_WatchdogLock(WDT_BASE);
}

void miot_wdt_enable(void) {
  MAP_WatchdogUnlock(WDT_BASE);
  MAP_WatchdogEnable(WDT_BASE);
  MAP_WatchdogLock(WDT_BASE);
}

void miot_wdt_disable(void) {
  LOG(LL_ERROR, ("WDT cannot be disabled!"));
}

void miot_system_restart(int exit_code) {
  (void) exit_code;
  if (exit_code != 100) {
    cc3200_fs_umount();
    sl_Stop(50 /* ms */);
  }
  /* Turns out to be not that easy. In particular, using *Reset functions is
   * not a good idea.
   * https://e2e.ti.com/support/wireless_connectivity/f/968/p/424736/1516404
   * Instead, the recommended way is to enter hibernation with immediate wakeup.
   */
  MAP_PRCMHibernateIntervalSet(328 /* 32KHz ticks, 100 ms */);
  MAP_PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);
  MAP_PRCMHibernateEnter();
}

void miot_usleep(int usecs) {
  osi_Sleep(usecs / 1000 /* ms */);
}

void mongoose_poll_cb(void *arg);

static bool s_mg_poll_scheduled;

void mongoose_schedule_poll(void) {
  /* Prevent piling up of poll callbacks. */
  if (s_mg_poll_scheduled) return;
  s_mg_poll_scheduled = miot_invoke_cb(mongoose_poll_cb, NULL);
}

void mongoose_poll_cb(void *arg) {
  s_mg_poll_scheduled = false;
  (void) arg;
}
