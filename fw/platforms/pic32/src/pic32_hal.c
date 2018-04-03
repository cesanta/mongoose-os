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

void mgos_wdt_feed(void) {
  /* TODO(dfrank) */
}

void mgos_wdt_set_timeout(int secs) {
  /* TODO(dfrank) */
  (void) secs;
}

size_t mgos_get_free_heap_size(void) {
  /* TODO(dfrank) */
  return 123;
}

size_t mgos_get_min_free_heap_size(void) {
  /* TODO(dfrank) */
  return 123;
}

void mgos_wifi_hal_init(void) {
  /* TODO(dfrank) */
}

void mgos_system_restart(int exit_code) {
  /* TODO(dfrank) */
  (void) exit_code;
}

void mgos_lock(void) {
  /* TODO(dfrank) */
}

void mgos_unlock(void) {
  /* TODO(dfrank) */
}

uint32_t mgos_get_cpu_freq(void) {
  /* TODO */
  return 0;
}
