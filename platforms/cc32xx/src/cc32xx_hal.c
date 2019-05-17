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

#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/prcm.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/wdt.h>

#include "FreeRTOS.h"
#include "task.h"

#include "common/cs_dbg.h"
#include "umm_malloc.h"

#include "mgos_hal.h"
#include "mgos_mongoose.h"

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

void device_set_mac_address(uint8_t mac[6]) {
  // TODO set mac address
  (void) mac;
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
