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

#include "arm_exc.h"
#include "mgos_app.h"
#include "mgos_core_dump.h"
#include "mgos_debug_internal.h"
#include "mgos_features.h"
#include "mgos_hal.h"
#include "mgos_freertos.h"
#include "mgos_init_internal.h"
#include "mgos_mongoose_internal.h"
#include "mgos_uart_internal.h"

#include "cc32xx_exc.h"
#include "cc32xx_hash.h"
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
#if CS_PLATFORM == CS_P_CC3200
__attribute__((section(".bss_start"))) uint32_t _bss_start;
__attribute__((section(".bss_end"))) uint32_t _bss_end;
#endif

/*
 * This is invoked very, very early - before C/C++ init.
 * C++ init requires heap, so we need to zero the UMM's arena.
 * On CC3200 we also zero bss, on CC3220 .cinit takes care of that.
 */
int _system_pre_init(void) {
#if CS_PLATFORM == CS_P_CC3200
  memset(&_bss_start, 0, ((char *) &_bss_end - (char *) &_bss_start));
#endif
  /* C++ init requires heap, so it needs to be inited early. */
  memset(UMM_MALLOC_CFG__HEAP_ADDR, 0, UMM_MALLOC_CFG__HEAP_SIZE);
  return 1;
}
#endif

enum mgos_init_result mgos_freertos_pre_init(void) {
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
  cc32xx_exc_printf("E:M %u %u\r\n", size, blocks_cnt);
  abort();
}

static void cc32xx_dump_sram(void) {
  mgos_cd_write_section("SRAM", (void *) SRAM_BASE_ADDR, SRAM_SIZE);
}

uint32_t mgos_bitbang_n100_cal = 0;
extern void mgos_nsleep100_cal(void);

void cc32xx_main(void) {
  PRCMCC3200MCUInit();
  cc32xx_exc_init();
  cc32xx_exc_puts("\r\n");

  mgos_nsleep100_cal();

  MAP_PRCMPeripheralClkEnable(PRCM_WDT, PRCM_RUN_MODE_CLK);

  if (!MAP_PRCMRTCInUseGet()) {
    MAP_PRCMRTCInUseSet();
    MAP_PRCMRTCSet(0, 0);
  }

  mgos_cd_register_section_writer(arm_exc_dump_regs);
  mgos_cd_register_section_writer(cc32xx_dump_sram);

  cc32xx_hash_module_init();
  cc32xx_sl_spawn_init();

  mgos_freertos_run_mgos_task(true /* start_scheduler */);
  /* Not reached */
}
