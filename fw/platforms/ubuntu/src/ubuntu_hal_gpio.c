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

#include "mgos.h"
#include "mgos_gpio_hal.h"

bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode) {
  LOG(LL_INFO, ("Not implemented yet"));
  return true;

  (void) pin;
  (void) mode;
}

bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull) {
  LOG(LL_INFO, ("Not implemented yet"));
  return true;

  (void) pin;
  (void) pull;
}

bool mgos_gpio_read(int pin) {
  LOG(LL_INFO, ("Not implemented yet"));
  return true;

  (void) pin;
}

void mgos_gpio_write(int pin, bool level) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void) pin;
  (void) level;
}

bool mgos_gpio_toggle(int pin) {
  LOG(LL_INFO, ("Not implemented yet"));
  return true;

  (void) pin;
}

bool mgos_gpio_enable_int(int pin) {
  LOG(LL_INFO, ("Not implemented yet"));
  return true;

  (void) pin;
}

bool mgos_gpio_disable_int(int pin) {
  LOG(LL_INFO, ("Not implemented yet"));
  return true;

  (void) pin;
}

bool mgos_gpio_hal_set_int_mode(int pin, enum mgos_gpio_int_mode mode) {
  LOG(LL_INFO, ("Not implemented yet"));
  return true;

  (void) pin;
  (void) mode;
}

void mgos_gpio_hal_int_cb(int pin) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void) pin;
}

enum mgos_init_result mgos_gpio_hal_init(void) {
  LOG(LL_INFO, ("Not implemented yet"));
  return MGOS_INIT_OK;
}
