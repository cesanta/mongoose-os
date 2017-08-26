/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "cc32xx_main.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "common/cs_dbg.h"

#include "mgos_debug.h"
#include "mgos_features.h"
#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_uart.h"
#include "mgos_updater_common.h"

/* TODO(rojer): Refactor */
#if CS_PLATFORM == CS_P_CC3200
#include "fw/platforms/cc3200/boot/lib/boot.h"
extern struct boot_cfg g_boot_cfg;
#endif

#ifndef MGOS_TASK_STACK_SIZE
#define MGOS_TASK_STACK_SIZE 8192 /* in bytes */
#endif

#ifndef MGOS_TASK_PRIORITY
#define MGOS_TASK_PRIORITY 5
#endif

#ifndef MGOS_TASK_QUEUE_LENGTH
#define MGOS_TASK_QUEUE_LENGTH 32
#endif

struct mgos_event {
  mgos_cb_t cb;
  void *arg;
};

static QueueHandle_t s_main_queue = NULL;

void mgos_lock_init(void);

#ifdef __TI_COMPILER_VERSION__
__attribute__((section(".heap_start"))) uint32_t _heap_start;
__attribute__((section(".heap_end"))) uint32_t _heap_end;
#endif

static void cc32xx_main_task(void *arg) {
  struct mgos_event e;
  s_main_queue = xQueueCreate(MGOS_TASK_QUEUE_LENGTH, sizeof(e));
  cc32xx_init_func_t init_func = (cc32xx_init_func_t) arg;

  mgos_lock_init();
  mgos_uart_init();
  mgos_debug_init();

  int r = init_func();
  bool init_success = (r == 0);
  if (!init_success) LOG(LL_ERROR, ("Init failed: %d", r));

#if MGOS_ENABLE_UPDATER
  mgos_upd_boot_finish(init_success,
                       (g_boot_cfg.flags & BOOT_F_FIRST_BOOT));
#endif

  if (!init_success) {
    /* Arbitrary delay to make potential reboot loop less tight. */
    mgos_usleep(500000);
    mgos_system_restart(0);
  }

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
  // TODO(rojer): Not ok to use buffered I/O here!
  LOG(LL_ERROR, ("Failed to allocate %u", size));
}

void cc32xx_main(cc32xx_init_func_t init_func) {
#ifdef __TI_COMPILER_VERSION__
  /* UMM malloc expects heap to be zeroed */
  memset(&_heap_start, 0, (char *) &_heap_end - (char *) &_heap_start);
#endif

  setvbuf(stdout, NULL, _IOLBF, 256);
  setvbuf(stderr, NULL, _IOLBF, 256);
  cs_log_set_level(MGOS_EARLY_DEBUG_LEVEL);

#if CS_PLATFORM == CS_P_CC3200
  VStartSimpleLinkSpawnTask(MGOS_TASK_PRIORITY + 1);
#endif

  xTaskCreate(cc32xx_main_task, "main", MGOS_TASK_STACK_SIZE, init_func, MGOS_TASK_PRIORITY, NULL);
  vTaskStartScheduler();
  /* Not reached */
}
