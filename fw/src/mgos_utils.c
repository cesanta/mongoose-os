/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>

#include "common/cs_dbg.h"
#include "mgos_hal.h"
#include "mgos_timers.h"

extern enum cs_log_level cs_log_threshold;
#if CS_ENABLE_STDIO
extern FILE *cs_log_file;
#endif

static void reboot_timer_cb(void *param) {
  mgos_system_restart();
  (void) param;
}

void mgos_system_restart_after(int delay_ms) {
  LOG(LL_INFO, ("Rebooting in %d ms", delay_ms));
  mgos_set_timer(delay_ms, 0 /*repeat*/, reboot_timer_cb, NULL);
}

float mgos_rand_range(float from, float to) {
  return from + (((float) (to - from)) / RAND_MAX * rand());
}

#if CS_ENABLE_STDIO
/*
 * Intended for ffi
 */
void mgos_log(const char *filename, int line_no, int level, const char *msg) {
  if (cs_log_threshold >= level) {
    fprintf(cs_log_file, "%17s:%-3d ", filename, line_no);
    cs_log_printf("%s", msg);
  }
  // LOG(level, ("%s", msg));
};
#endif
