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
