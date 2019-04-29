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

#include "mgos_uart_hal.h"
#include "ubuntu.h"

bool mgos_uart_hal_init(struct mgos_uart_state *us) {
  LOG(LL_INFO, ("Not implemented yet"));
  return true;

  (void) us;
}

bool mgos_uart_hal_configure(struct mgos_uart_state *us,
                             const struct mgos_uart_config *cfg) {
  LOG(LL_INFO, ("Not implemented yet"));
  return true;

  (void) us;
  (void) cfg;
}

void mgos_uart_hal_config_set_defaults(int uart_no,
                                       struct mgos_uart_config *cfg) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void) uart_no;
  (void) cfg;
}

void mgos_uart_hal_dispatch_rx_top(struct mgos_uart_state *us) {
  //  LOG(LL_INFO, ("rx: Not implemented yet"));
  return;

  (void) us;
}

void mgos_uart_hal_dispatch_tx_top(struct mgos_uart_state *us) {
  //  LOG(LL_INFO, ("tx: Not implemented yet"));
  return;

  (void) us;
}

void mgos_uart_hal_dispatch_bottom(struct mgos_uart_state *us) {
  //  LOG(LL_INFO, ("bottom: Not implemented yet"));
  return;

  (void) us;
}

void mgos_uart_hal_flush_fifo(struct mgos_uart_state *us) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void) us;
}

void mgos_uart_hal_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void) us;
  (void) enabled;
}
