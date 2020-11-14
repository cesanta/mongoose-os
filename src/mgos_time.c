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

#ifndef __TI_COMPILER_VERSION__
#include <unistd.h>
#endif

#include "mgos.h"
#include "mgos_event.h"
#include "mgos_hal.h"
#include "mgos_mongoose.h"

#include "common/queue.h"

#if defined(_DEFAULT_SOURCE) || defined(_BSD_SOURCE)
#include <sys/time.h>
#endif

double mgos_uptime(void) {
  return mgos_uptime_micros() / 1000000.0;
}

int64_t mgos_time_micros(void) {
  return mg_time() * 1000000.0;
}

int mgos_strftime(char *s, int size, const char *fmt, int time) {
  time_t t = (time_t) time;
  struct tm *tmp = localtime(&t);
  return tmp == NULL ? -1 : (int) strftime(s, size, fmt, tmp);
}

int mgos_settimeofday(double time, struct timezone *tz) {
  double delta = time - mg_time();
  struct timeval tv;
  tv.tv_sec = (time_t) time;
  tv.tv_usec = (time - tv.tv_sec) * 1000000;
  int ret = settimeofday(&tv, tz);
  if (ret == 0) {
    mgos_event_trigger(MGOS_EVENT_TIME_CHANGED, &delta);
  }
  return ret;
}
