/*
 * Copyright 2019 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>

#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_net_hal.h"
#include "mgos_time.h"

static struct timeval s_boottime;

bool ubuntu_set_boottime(void) {
  gettimeofday(&s_boottime, NULL);
  return true;
}

int64_t mgos_uptime_micros(void) {
  struct timeval now;

  gettimeofday(&now, NULL);
  return (int64_t)(now.tv_sec * 1e6 + now.tv_usec -
                   (s_boottime.tv_sec * 1e6 + s_boottime.tv_usec));
}

int mg_ssl_if_mbed_random(void *ctx, unsigned char *buf, size_t len) {
  while (len-- > 0) {
    *buf++ = (unsigned char) rand();
  }
  (void) ctx;
  return 0;
}

void mgos_cd_putc(int c) {
  fputc(c, stderr);
}
