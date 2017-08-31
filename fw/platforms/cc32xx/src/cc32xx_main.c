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
#include "mgos_debug.h"
#include "mgos_features.h"
#include "mgos_hal.h"
#include "mgos_init.h"
#include "mgos_mongoose.h"
#include "mgos_uart.h"
#include "mgos_updater_common.h"

#include "cc32xx_exc.h"
#include "cc32xx_sl_spawn.h"
#include "cc32xx_task_config.h"
#include "umm_malloc_cfg.h"

/* TODO(rojer): Refactor */
#if CS_PLATFORM == CS_P_CC3200
#include "fw/platforms/cc3200/boot/lib/boot.h"
extern struct boot_cfg g_boot_cfg;
#endif

struct mgos_event {
  mgos_cb_t cb;
  void *arg;
};

static QueueHandle_t s_main_queue = NULL;

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

void mgos_lock_init(void);

static int cc32xx_start_nwp(void) {
  int r = sl_Start(NULL, NULL, NULL);
  if (r < 0) {
    return r;
  }
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

int cc32xx_init(void) {
  mgos_lock_init();
  mgos_uart_init();
  mgos_debug_init();
  mgos_debug_uart_init();

  setvbuf(stdout, NULL, _IOLBF, 256);
  setvbuf(stderr, NULL, _IOLBF, 256);
  cs_log_set_level(MGOS_EARLY_DEBUG_LEVEL);

  if (strcmp(MGOS_APP, "mongoose-os") != 0) {
    LOG(LL_INFO, ("%s %s (%s)", MGOS_APP, build_version, build_id));
  }
  LOG(LL_INFO, ("Mongoose OS %s (%s)", mg_build_version, mg_build_id));
  LOG(LL_INFO, ("RAM: %d total, %d free", mgos_get_heap_size(),
                mgos_get_free_heap_size()));

  int r = cc32xx_start_nwp();
  if (r < 0) {
    LOG(LL_ERROR, ("Failed to start NWP: %d", r));
    return -2;
  }

  mongoose_init();

  return 0;
}

static void cc32xx_main_task(void *arg) {
  struct mgos_event e;
  cc32xx_init_func_t init_func = (cc32xx_init_func_t) arg;

  int r = init_func(true /* pre */);
  r = (r == 0 ? cc32xx_init() : r);
  r = (r == 0 ? init_func(false /* pre */) : r);
  r = (r == 0 ? mgos_init() : r);

  bool init_success = (r == 0);
  if (!init_success) LOG(LL_ERROR, ("Init failed: %d", r));

#if MGOS_ENABLE_UPDATER
  mgos_upd_boot_finish(init_success, (g_boot_cfg.flags & BOOT_F_FIRST_BOOT));
#endif

  if (!init_success) {
    /* Arbitrary delay to make potential reboot loop less tight. */
    mgos_usleep(500000);
    mgos_system_restart(0);
  }

  mgos_wdt_set_feed_on_poll(true);

  while (1) {
    mongoose_poll(0);
    while (xQueueReceive(s_main_queue, &e, 10 /* tick */)) {
      e.cb(e.arg);
    }
  }
}

bool mgos_invoke_cb(mgos_cb_t cb, void *arg, bool from_isr) {
  struct mgos_event e = {.cb = cb, .arg = arg};
  if (from_isr) {
    BaseType_t should_yield = false;
    if (!xQueueSendToBackFromISR(s_main_queue, &e, &should_yield)) {
      return false;
    }
    portYIELD_FROM_ISR(should_yield);
    return true;
  } else {
    return xQueueSendToBack(s_main_queue, &e, 10);
  }
}

void umm_oom_cb(size_t size, unsigned short int blocks_cnt) {
  (void) blocks_cnt;
  cc32xx_exc_printf("E:M %u\r\n", size, blocks_cnt);
}

void cc32xx_main(cc32xx_init_func_t init_func) {
  PRCMCC3200MCUInit();
  cc32xx_exc_init();
#ifdef __TI_COMPILER_VERSION__
  /* UMM malloc expects heap to be zeroed */
  memset(UMM_MALLOC_CFG__HEAP_ADDR, 0, UMM_MALLOC_CFG__HEAP_SIZE);
#endif
  cc32xx_exc_puts("\r\n");

  MAP_PRCMPeripheralClkEnable(PRCM_WDT, PRCM_RUN_MODE_CLK);
  mgos_wdt_set_timeout(5 /* seconds */);
  mgos_wdt_enable();

  if (!MAP_PRCMRTCInUseGet()) {
    MAP_PRCMRTCInUseSet();
    MAP_PRCMRTCSet(0, 0);
  }

  /* Early init app hook. */
  mgos_app_preinit();

  cc32xx_sl_spawn_init();

  s_main_queue =
      xQueueCreate(MGOS_TASK_QUEUE_LENGTH, sizeof(struct mgos_event));
  xTaskCreate(cc32xx_main_task, "main",
              MGOS_TASK_STACK_SIZE / sizeof(portSTACK_TYPE), init_func,
              MGOS_TASK_PRIORITY, NULL);

  vTaskStartScheduler();
  /* Not reached, but just in case... */
  cc32xx_exc_puts("Scheduler failed to start!\r\n");
  mgos_system_restart(0);
}
