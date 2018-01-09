/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/prcm.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/wdt.h>

#include "FreeRTOS.h"
#include "task.h"

#include "common/cs_dbg.h"
#include "common/umm_malloc/umm_malloc.h"

#include "mgos_hal.h"
#include "mgos_mongoose.h"

/* TODO(rojer): make accurate to account for vTaskDelay inaccuracy */
void mgos_usleep(uint32_t usecs) {
  int ticks = usecs / (1000000 / configTICK_RATE_HZ);
  int remainder = usecs % (1000000 / configTICK_RATE_HZ);
  if (ticks > 0) vTaskDelay(ticks);
  if (remainder > 0) MAP_UtilsDelay(remainder * (SYS_CLK / 1000000) / 3);
}

void mgos_msleep(uint32_t msecs) {
  mgos_usleep(msecs * 1000);
}

void mgos_wdt_feed(void) {
  MAP_WatchdogIntClear(WDT_BASE);
}

void mgos_wdt_set_timeout(int secs) {
  MAP_WatchdogUnlock(WDT_BASE);
  /* Reset is triggered after the timer reaches zero for the second time. */
  MAP_WatchdogReloadSet(WDT_BASE, secs * SYS_CLK / 2);
  MAP_WatchdogLock(WDT_BASE);
}

void mgos_wdt_enable(void) {
  MAP_WatchdogUnlock(WDT_BASE);
  MAP_WatchdogEnable(WDT_BASE);
  MAP_WatchdogLock(WDT_BASE);
}

void mgos_wdt_disable(void) {
  LOG(LL_ERROR, ("WDT cannot be disabled!"));
}

uint32_t mgos_get_cpu_freq(void) {
  return SYS_CLK;
}

void device_get_mac_address(uint8_t mac[6]) {
  SL_LEN_TYPE mac_len = 6;
  sl_NetCfgGet(SL_NETCFG_MAC_ADDRESS_GET, NULL, &mac_len, mac);
}

#ifdef __TI_COMPILER_VERSION__

size_t mgos_get_heap_size(void) {
  return UMM_MALLOC_CFG__HEAP_SIZE;
}

size_t mgos_get_free_heap_size(void) {
  return umm_free_heap_size();
}

size_t mgos_get_min_free_heap_size(void) {
  return umm_min_free_heap_size();
}

#else

/* Defined in linker script. */
extern unsigned long _heap;
extern unsigned long _eheap;

#include <malloc.h>

size_t mgos_get_heap_size(void) {
  return ((char *) &_eheap - (char *) &_heap);
}

size_t mgos_get_free_heap_size(void) {
  size_t avail = mgos_get_heap_size();
  struct mallinfo mi = mallinfo();
  avail -= mi.arena;    /* Claimed by allocator. */
  avail += mi.fordblks; /* Free in the area claimed by allocator. */
  return avail;
}

size_t mgos_get_min_free_heap_size(void) {
  /* Not supported */
  return 0;
}

#endif
