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

#include <time.h>

#include "mgos.h"
#include "mgos_deps_internal.h"
#include "mgos_init.h"

enum mgos_init_result mgos_init(void) {
  if (!mgos_deps_init()) {
    return MGOS_INIT_DEPS_FAILED;
  }

  if (mgos_app_init() != MGOS_APP_INIT_SUCCESS) {
    return MGOS_INIT_APP_INIT_FAILED;
  }

  LOG(LL_INFO, ("Init done, RAM: %lu total, %lu free, %lu min free",
                (unsigned long) mgos_get_heap_size(),
                (unsigned long) mgos_get_free_heap_size(),
                (unsigned long) mgos_get_min_free_heap_size()));
  mgos_set_enable_min_heap_free_reporting(true);

  /* Invoke all registered init_done event handlers */
  mgos_event_trigger(MGOS_EVENT_INIT_DONE, NULL);

  return MGOS_INIT_OK;
}

void mgos_app_preinit(void) __attribute__((weak));
void mgos_app_preinit(void) {
}

bool mgos_deps_init(void) __attribute__((weak));
bool mgos_deps_init(void) {
  return true;
}

enum mgos_app_init_result mgos_app_init(void) __attribute__((weak));
enum mgos_app_init_result mgos_app_init(void) {
  return MGOS_APP_INIT_SUCCESS;
}
