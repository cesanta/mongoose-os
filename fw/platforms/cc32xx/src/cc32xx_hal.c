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
#include "semphr.h"
#include "task.h"

#include "common/cs_dbg.h"
#include "common/umm_malloc/umm_malloc.h"

#include "mgos_hal.h"
#include "mgos_mongoose.h"

SemaphoreHandle_t s_mgos_mux = NULL;

void mgos_lock_init(void) {
  s_mgos_mux = xSemaphoreCreateRecursiveMutex();
}

void mgos_lock(void) {
  xSemaphoreTakeRecursive(s_mgos_mux, portMAX_DELAY);
}

void mgos_unlock(void) {
  xSemaphoreGiveRecursive(s_mgos_mux);
}

struct mgos_rlock_type *mgos_new_rlock(void) {
  return (struct mgos_rlock_type *) xSemaphoreCreateRecursiveMutex();
}

void mgos_rlock(struct mgos_rlock_type *l) {
  xSemaphoreTakeRecursive((SemaphoreHandle_t) l, portMAX_DELAY);
}

void mgos_runlock(struct mgos_rlock_type *l) {
  xSemaphoreGiveRecursive((SemaphoreHandle_t) l);
}

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

void mgos_ints_disable(void) {
  portENTER_CRITICAL();
}

void mgos_ints_enable(void) {
  portEXIT_CRITICAL();
}

void mongoose_poll_cb(void *arg);

static bool s_mg_poll_scheduled;

void mongoose_schedule_poll(bool from_isr) {
  /* Prevent piling up of poll callbacks. */
  if (s_mg_poll_scheduled) return;
  s_mg_poll_scheduled = mgos_invoke_cb(mongoose_poll_cb, NULL, from_isr);
}

void mongoose_poll_cb(void *arg) {
  s_mg_poll_scheduled = false;
  (void) arg;
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
