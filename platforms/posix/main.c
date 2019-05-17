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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "common/cs_dbg.h"

#include "mgos_init.h"
#include "mgos_mongoose_internal.h"
#include "mgos_sys_config.h"
#include "mgos_uart_hal.h"

#include "fw.h"

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  mongoose_init();
  for (;;) {
    mongoose_poll(1000);
  }
  return EXIT_SUCCESS;
}

static void dummy_handler(struct mg_connection *nc, int ev, void *ev_data,
                          void *user_data) {
  (void) nc;
  (void) ev;
  (void) ev_data;
  (void) user_data;
}

void mongoose_schedule_poll(bool from_isr) {
  (void) from_isr;
  mg_broadcast(mgos_get_mgr(), dummy_handler, NULL, 0);
}

enum mgos_init_result mgos_sys_config_init_platform(struct mgos_config *cfg) {
  cs_log_set_level(cfg->debug.level);
  return MGOS_INIT_OK;
}

void device_get_mac_address(uint8_t mac[6]) {
  int i;
  srand(time(NULL));
  for (i = 0; i < 6; i++) {
    mac[i] = (double) rand() / RAND_MAX * 255;
  }
}

void device_set_mac_address(uint8_t mac[6]) {
  // TODO set mac address
}

void mgos_uart_hal_set_defaults(struct mgos_uart_config *cfg) {
  (void) cfg;
}

bool mgos_uart_hal_init(struct mgos_uart_state *us) {
  (void) us;
  return false;
}

bool mgos_uart_hal_configure(struct mgos_uart_state *us,
                             const struct mgos_uart_config *cfg) {
  (void) us;
  (void) cfg;
  return false;
}

void mgos_uart_hal_config_set_defaults(int uart_no,
                                       struct mgos_uart_config *cfg) {
  (void) uart_no;
  (void) cfg;
}

void mgos_uart_hal_dispatch_rx_top(struct mgos_uart_state *us) {
  (void) us;
}
void mgos_uart_hal_dispatch_tx_top(struct mgos_uart_state *us) {
  (void) us;
}
void mgos_uart_hal_dispatch_bottom(struct mgos_uart_state *us) {
  (void) us;
}

void mgos_uart_hal_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  (void) us;
  (void) enabled;
}

void mgos_uart_hal_flush_fifo(struct mgos_uart_state *us) {
  (void) us;
}
