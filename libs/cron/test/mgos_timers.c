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
#include <stdio.h>
#include <string.h>
#include "mgos_timers.h"
#include "common/queue.h"
#include "ccronexpr.h"
#include "mgos_cron.h"

struct timer_info {
  int interval_ms;
  double next_invocation;
  timer_callback cb;
  void *cb_arg;
  TAILQ_ENTRY(timer_info) entries;
};

TAILQ_HEAD(s_timers, timer_info)
s_timers = TAILQ_HEAD_INITIALIZER(s_timers);

static double s_time = 0.0;

void mgos_schedule_timers(struct test_entry *te, int te_sz) {
  if (te == NULL) return;

  struct timer_info *ti;
  struct timer_info *min_ti;

  struct test_entry *p = te;

  TAILQ_FOREACH(ti, &s_timers, entries) {
    p->current = ti->next_invocation;
    p->cron_entry = ti->cb_arg;
    p++;
  }

  do {
    min_ti = NULL;
    TAILQ_FOREACH(ti, &s_timers, entries) {
      if (min_ti == NULL || ti->next_invocation < min_ti->next_invocation) {
        min_ti = ti;
      }
    }
    if (min_ti != NULL) {
      p = te;
      for (int i = 0; i < te_sz; i++) {
        if (p[i].cron_entry == min_ti->cb_arg) {
          p[i].current = min_ti->next_invocation;
          break;
        }
      }
      mgos_set_time(min_ti->next_invocation);
      min_ti->cb(min_ti->cb_arg);
    }
  } while (min_ti != NULL);
}

double mg_time(void) {
  return s_time;
}

void mgos_set_time(double t) {
  s_time = t;
}

mgos_timer_id mgos_set_timer(double msecs, int repeat, timer_callback cb,
                             void *arg) {
  struct timer_info *ti = NULL;
  TAILQ_FOREACH(ti, &s_timers, entries) {
    if (ti->cb_arg == arg) {
      mgos_clear_timer((mgos_timer_id) ti);
    }
  }

  ti = (struct timer_info *) calloc(1, sizeof(*ti));
  if (ti == NULL) return MGOS_INVALID_TIMER_ID;

  ti->next_invocation = mg_time() + msecs / 1000.0;
  ti->interval_ms = -1;
  ti->cb = cb;
  ti->cb_arg = arg;

  TAILQ_INSERT_TAIL(&s_timers, ti, entries);

  (void) repeat;
  return (mgos_timer_id) ti;
}

void mgos_clear_timer(mgos_timer_id id) {
  if (id == MGOS_INVALID_TIMER_ID) return;
  struct timer_info *ti = (struct timer_info *) id, *ti2;
  TAILQ_FOREACH(ti2, &s_timers, entries) {
    if (ti2 == ti) break;
  }
  if (ti2 == NULL) return; /* Not a valid timer */
  TAILQ_REMOVE(&s_timers, ti, entries);
  free(ti);
}
