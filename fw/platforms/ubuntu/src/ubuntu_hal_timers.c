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

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "mgos_hw_timers_hal.h"
#include "ubuntu.h"

bool mgos_hw_timers_dev_set(struct mgos_hw_timer_info *ti, int usecs,
                            int flags) {
  LOG(LL_INFO, ("Not implemented yet"));
  return true;

  (void) ti;
  (void) usecs;
  (void) flags;
}

void mgos_hw_timers_isr(struct mgos_hw_timer_info *ti) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void) ti;
}

void mgos_hw_timers_dev_isr_bottom(struct mgos_hw_timer_info *ti) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void) ti;
}

void mgos_hw_timers_dev_clear(struct mgos_hw_timer_info *ti) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void) ti;
}

bool mgos_hw_timers_dev_init(struct mgos_hw_timer_info *ti) {
  LOG(LL_INFO, ("Not implemented yet"));
  return true;

  (void) ti;
}

enum mgos_init_result mgos_hw_timers_init(void) {
  LOG(LL_INFO, ("Not implemented yet"));
  return MGOS_INIT_OK;
}

void mgos_hw_timers_deinit(void) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;
}

void mgos_clear_hw_timer(mgos_timer_id id) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void) id;
}
