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

#include "mgos_system.h"

#include <stdbool.h>
#include <stdlib.h>

#include "common/cs_dbg.h"

#include "mgos_debug.h"
#include "mgos_event.h"
#include "mgos_hal.h"
#include "mgos_hw_timers_hal.h"
#include "mgos_time.h"
#include "mgos_vfs.h"
#ifdef MGOS_HAVE_WIFI
#include "mgos_wifi.h"
#endif

void mgos_system_restart(void) {
  mgos_event_trigger(MGOS_EVENT_REBOOT, NULL);
  mgos_vfs_umount_all();
  mgos_hw_timers_deinit();
#ifdef MGOS_HAVE_WIFI
  mgos_wifi_disconnect();
  mgos_wifi_deinit();
#endif
  LOG(LL_INFO, ("Restarting"));
  mgos_debug_flush();
  mgos_dev_system_restart();
}

static void reboot_timer_cb(void *param) {
  mgos_system_restart();
  (void) param;
}

static void trigger_ev(void *arg) {
  mgos_event_trigger(MGOS_EVENT_REBOOT_AFTER, arg);
}

void mgos_system_restart_after(int delay_ms) {
  LOG(LL_INFO, ("Rebooting in %d ms", delay_ms));
  struct mgos_event_reboot_after_arg *arg = calloc(1, sizeof(*arg));
  arg->reboot_at_uptime_micros = mgos_uptime_micros() + delay_ms * 1000;
  mgos_invoke_cb(trigger_ev, arg, false /* from_isr */);
  mgos_set_timer(delay_ms, 0 /*repeat*/, reboot_timer_cb, NULL);
}

int mgos_itoa(int value, char *out, int base) {
  if (base == 10 && value < 0) {
    *(out++) = '-';
    return mgos_utoa((unsigned int) (-value), out, base) + 1;
  }
  return mgos_utoa((unsigned int) value, out, base);
}

int mgos_utoa(unsigned int value, char *out, int base) {
  int n = 0;
  const char *digits = "0123456789abcdef";
  if (base < 2 || base > 16) return 0;
  do {
    out[n++] = digits[value % base];
    value /= base;
  } while (value > 0);
  /* Reverse output */
  for (int i = 0, j = n; i < --j; i++) {
    char c = out[i];
    out[i] = out[j];
    out[j] = c;
  }
  out[n] = '\0';
  return n;
}
