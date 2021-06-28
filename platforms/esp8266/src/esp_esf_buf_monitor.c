/*
 * Copyright (c) 2021 Deomid "rojer" Ryabkov
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "esp_esf_buf_monitor.h"

#include <stdlib.h>

#include "mgos.h"

struct pbuf;

struct esf_buf *g_pending_bufs[8] = {0};
uint32_t g_esf_buf_canary_ctr = 0;
uint8_t g_esf_buf_canary_strikes = 0;

extern struct esf_buf *__real_esf_buf_alloc(struct pbuf *p, uint32_t type,
                                            uint32_t a4);
struct esf_buf *__wrap_esf_buf_alloc(struct pbuf *p, uint32_t type,
                                     uint32_t a4) {
  struct esf_buf *eb = __real_esf_buf_alloc(p, type, a4);
  if (eb != NULL) {
    for (int i = 0; i < 8; i++) {
      if (g_pending_bufs[i] == NULL) {
        g_pending_bufs[i] = eb;
        break;
      }
    }
  }
  return eb;
}

extern void __real_esf_buf_recycle(struct esf_buf *eb, uint32_t type);
void __wrap_esf_buf_recycle(struct esf_buf *eb, uint32_t type) {
  for (int i = 0; i < 8; i++) {
    if (g_pending_bufs[i] == eb) {
      g_pending_bufs[i] = NULL;
      g_esf_buf_canary_ctr++;
    }
  }
  __real_esf_buf_recycle(eb, type);
}

static uint32_t s_esf_buf_last_canary_ctr = 0;

static void esp_esf_buf_monitor_timer_cb(void *arg) {
  int num_pending = 0;
  for (int i = 0; i < 8; i++) {
    if (g_pending_bufs[i] != NULL) num_pending++;
  }
  LOG(LL_DEBUG, ("np %d ctr %lu %lu", num_pending, s_esf_buf_last_canary_ctr,
                 g_esf_buf_canary_ctr));
  if (num_pending == 0) {
    // All clear, no problem.
    return;
  }
  if (g_esf_buf_canary_ctr != s_esf_buf_last_canary_ctr) {
    // Things are moving along, all good.
    s_esf_buf_last_canary_ctr = g_esf_buf_canary_ctr;
    return;
  }
  LOG(LL_ERROR, ("TX is stuck!"));
  static bool cb_invoked = false;
  if (!cb_invoked) {
    esp_esf_buf_monitor_failure();
    cb_invoked = true;
  }
  (void) arg;
}

void esp_esf_buf_monitor_failure(void) WEAK;
void esp_esf_buf_monitor_failure(void) {
  mgos_system_restart_after(1000);
}

extern void __real_lmacProcessTXStartData(uint8_t id);
IRAM void __wrap_lmacProcessTXStartData(uint8_t id) {
#ifdef ESP_ESF_BUF_TRIGGER_BUG
  // Introducing a delay here triggers the condition reasonably quickly
  // under moderate traffic load (3 x curl; sleep 1 in parallel).
  if (id == 0) {
    ets_delay_us(500);
  }
#endif
  __real_lmacProcessTXStartData(id);
}

void esp_esf_buf_monitor_init(void) {
  mgos_set_timer(ESP_ESF_BUF_MONITOR_INTERVAL_MS, MGOS_TIMER_REPEAT,
                 esp_esf_buf_monitor_timer_cb, NULL);
}
