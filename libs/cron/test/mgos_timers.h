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

#ifndef CS_MOS_LIBS_CRON_SRC_TEST_MGOS_TIMERS_H_
#define CS_MOS_LIBS_CRON_SRC_TEST_MGOS_TIMERS_H_

#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MGOS_INVALID_TIMER_ID (0)

struct test_entry {
  int id;
  const char *expr;
  time_t current;
  const char *expected[3];
  int count;
  struct mgos_cron_entry *cron_entry;
};

typedef void (*timer_callback)(void *param);
typedef uintptr_t mgos_timer_id;

time_t mgos_mktime(const char *date);
void mgos_schedule_timers(struct test_entry *te, int te_sz);
void mgos_set_time(double t);
double mg_time(void);

mgos_timer_id mgos_set_timer(double msecs, int repeat, timer_callback cb,
                             void *arg);

void mgos_clear_timer(mgos_timer_id id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_CRON_SRC_TEST_MGOS_TIMERS_H_ */
