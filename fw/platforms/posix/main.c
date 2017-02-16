/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "common/cs_dbg.h"

#include "fw/src/mgos_console.h"
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_uart.h"

#include "fw.h"

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  mongoose_init();
  for (;;) {
    mongoose_poll(1000);
  }
  mongoose_destroy();
  return EXIT_SUCCESS;
}

static void dummy_handler(struct mg_connection *nc, int ev, void *ev_data) {
  (void) nc;
  (void) ev;
  (void) ev_data;
}

void mongoose_schedule_poll(void) {
  mg_broadcast(mgos_get_mgr(), dummy_handler, NULL, 0);
}

enum mgos_init_result mgos_sys_config_init_platform(struct sys_config *cfg) {
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

void mgos_uart_dev_set_defaults(struct mgos_uart_config *cfg) {
  (void) cfg;
}

bool mgos_uart_dev_init(struct mgos_uart_state *us) {
  (void) us;
  return false;
}

void mgos_uart_dev_deinit(struct mgos_uart_state *us) {
  (void) us;
}

void mgos_uart_dev_dispatch_rx_top(struct mgos_uart_state *us) {
  (void) us;
}
void mgos_uart_dev_dispatch_tx_top(struct mgos_uart_state *us) {
  (void) us;
}
void mgos_uart_dev_dispatch_bottom(struct mgos_uart_state *us) {
  (void) us;
}

void mgos_uart_dev_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  (void) us;
  (void) enabled;
}

void mgos_uart_dev_flush_fifo(struct mgos_uart_state *us) {
  (void) us;
}

enum mgos_init_result mgos_set_stdout_uart(int uart_no) {
  if (uart_no <= 0) return MGOS_INIT_OK;
  /* TODO */
  return MGOS_INIT_UART_FAILED;
}

enum mgos_init_result mgos_set_stderr_uart(int uart_no) {
  if (uart_no <= 0) return MGOS_INIT_OK;
  /* TODO */
  return MGOS_INIT_UART_FAILED;
}

void mgos_lock(void) {
}

void mgos_unlock(void) {
}
