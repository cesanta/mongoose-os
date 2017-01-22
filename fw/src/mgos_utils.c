/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_console.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_timers.h"

static void reboot_timer_cb(void *param) {
  mgos_system_restart(0);
  (void) param;
}

void mgos_system_restart_after(int delay_ms) {
  CONSOLE_LOG(LL_INFO, ("Rebooting in %d ms", delay_ms));
  mgos_set_timer(delay_ms, 0 /*repeat*/, reboot_timer_cb, NULL);
}
