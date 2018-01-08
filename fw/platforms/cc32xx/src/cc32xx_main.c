/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "cc32xx_main.h"

#include <stdbool.h>

#include <inc/hw_types.h>
#include <driverlib/prcm.h>
#include <driverlib/rom_map.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "common/cs_dbg.h"
#include "common/platform.h"

#include "mgos_app.h"
#include "mgos_debug_internal.h"
#include "mgos_features.h"
#include "mgos_hal.h"
#include "mgos_hal_freertos_internal.h"
#include "mgos_init_internal.h"
#include "mgos_mongoose_internal.h"
#include "mgos_uart_internal.h"
#include "mgos_updater_common.h"

#include "cc32xx_exc.h"
#include "cc32xx_sl_spawn.h"
#include "cc32xx_task_config.h"
#include "umm_malloc_cfg.h"

#if SL_MAJOR_VERSION_NUM < 2
#define SL_NETAPP_HTTP_SERVER_ID SL_NET_APP_HTTP_SERVER_ID
#endif

uint32_t mg_lwip_get_poll_delay_ms(struct mg_mgr *mgr) {
  uint32_t timeout_ms = ~0;
  struct mg_connection *nc;
  double min_timer = 0;
  int num_timers = 0;
  for (nc = mg_next(mgr, NULL); nc != NULL; nc = mg_next(mgr, nc)) {
    if (nc->ev_timer_time == 0) continue;
    if (num_timers == 0 || nc->ev_timer_time < min_timer) {
      min_timer = nc->ev_timer_time;
    }
    num_timers++;
  }
  double now = mg_time();
  if (num_timers > 0) {
    /* If we have a timer that is past due, do a poll ASAP. */
    if (min_timer < now) return 0;
    double timer_timeout_ms = (min_timer - now) * 1000 + 1 /* rounding */;
    if (timer_timeout_ms < timeout_ms) {
      timeout_ms = timer_timeout_ms;
    }
  }
  return timeout_ms;
}

static int cc32xx_start_nwp(void) {
  int r = sl_Start(NULL, NULL, NULL);
  if (r < 0) return r;
  /* Stop the HTTP server in case WiFi is disabled and never inited. */
  sl_NetAppStop(SL_NETAPP_HTTP_SERVER_ID);
  SlDeviceVersion_t ver;
  _u8 opt = SL_DEVICE_GENERAL_VERSION;
  SL_LEN_TYPE len = sizeof(ver);
  memset(&ver, 0, sizeof(ver));
  sl_DeviceGet(SL_DEVICE_GENERAL, &opt, &len, (void *) (&ver));
  LOG(LL_INFO, ("NWP v%lu.%lu.%lu.%lu started, host driver v%ld.%ld.%ld.%ld",
                ver.NwpVersion[0], ver.NwpVersion[1], ver.NwpVersion[2],
                ver.NwpVersion[3], SL_MAJOR_VERSION_NUM, SL_MINOR_VERSION_NUM,
                SL_VERSION_NUM, SL_SUB_VERSION_NUM));
  return 0;
}

#ifdef __TI_COMPILER_VERSION__
__attribute__((section(".heap_start"))) uint32_t _heap_start;
__attribute__((section(".heap_end"))) uint32_t _heap_end;
#endif

enum mgos_init_result mgos_hal_freertos_pre_init(void) {
  enum mgos_init_result r = cc32xx_pre_nwp_init();
  if (r != MGOS_INIT_OK) return r;

  if (cc32xx_start_nwp() != 0) {
    LOG(LL_ERROR, ("Failed to start NWP"));
    return MGOS_INIT_FS_INIT_FAILED;
  }

  return cc32xx_init();
}

void umm_oom_cb(size_t size, unsigned short int blocks_cnt) {
  (void) blocks_cnt;
  cc32xx_exc_printf("E:M %u\r\n", size, blocks_cnt);
}

void (*mgos_nsleep100)(uint32_t n);
void cc32xx_nsleep100(uint32_t n) {
  /* TODO(rojer) */
}
uint32_t mgos_bitbang_n100_cal = 0;

void cc32xx_main(void) {
  mgos_nsleep100 = cc32xx_nsleep100;
  PRCMCC3200MCUInit();
  cc32xx_exc_init();
#ifdef __TI_COMPILER_VERSION__
  /* UMM malloc expects heap to be zeroed */
  memset(UMM_MALLOC_CFG__HEAP_ADDR, 0, UMM_MALLOC_CFG__HEAP_SIZE);
#endif
  cc32xx_exc_puts("\r\n");

  MAP_PRCMPeripheralClkEnable(PRCM_WDT, PRCM_RUN_MODE_CLK);

  if (!MAP_PRCMRTCInUseGet()) {
    MAP_PRCMRTCInUseSet();
    MAP_PRCMRTCSet(0, 0);
  }

  cc32xx_sl_spawn_init();

  mgos_hal_freertos_run_mgos_task(true /* start_scheduler */);
  /* Not reached */
}
