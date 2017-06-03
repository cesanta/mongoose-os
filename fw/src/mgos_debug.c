/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <stdlib.h>

#include "fw/src/mgos_debug.h"
#include "fw/src/mgos_debug_hal.h"

#include "common/cs_dbg.h"

#include "mongoose/mongoose.h"

#include "fw/src/mgos_features.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_hooks.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_uart.h"

#ifndef IRAM
#define IRAM
#endif

static int8_t s_stdout_uart = MGOS_DEBUG_UART;
static int8_t s_stderr_uart = MGOS_DEBUG_UART;
static int8_t s_uart_suspended = 0;
static int8_t s_in_debug = 0;

/* From cs_dbg.c */
extern enum cs_log_level cs_log_cur_msg_level;

void mgos_debug_write(int fd, const void *data, size_t len) {
  char buf[MGOS_DEBUG_TMP_BUF_SIZE];
  int uart_no = -1;
  mgos_lock();
  if (s_in_debug) {
    mgos_unlock();
    return;
  }
  s_in_debug = true;
  if (s_uart_suspended <= 0) {
    if (fd == 1) {
      uart_no = s_stdout_uart;
    } else if (fd == 2) {
      uart_no = s_stderr_uart;
    }
  }
  if (uart_no >= 0) {
    len = mgos_uart_write(uart_no, data, len);
    mgos_uart_flush(uart_no);
  }
  const struct sys_config *cfg = get_cfg();
  /* Only send LL_DEBUG messages and below, to avoid loops. */
  if (cfg == NULL || cs_log_cur_msg_level > LL_DEBUG) {
    s_in_debug = false;
    mgos_unlock();
    return;
  }
#if MGOS_ENABLE_DEBUG_UDP
  /* Only send STDERR to UDP. */
  if (fd == 2 && cfg->debug.udp_log_addr != NULL) {
    static uint32_t s_seq = 0;
    int n =
        snprintf(buf, sizeof(buf), "%s %u %.3lf %d|",
                 (cfg->device.id ? cfg->device.id : "-"), s_seq, mg_time(), fd);
    if (n > 0) {
      mgos_debug_udp_send(mg_mk_str_n(buf, n), mg_mk_str_n(data, len));
    }
    s_seq++;
  }
#endif /* MGOS_ENABLE_DEBUG_UDP */

  /* Invoke all registered debug_write hooks */
  {
    struct mgos_hook_arg arg = {
        {.debug = {
             .buf = buf, .fd = fd, .data = data, .len = len, }}};
    mgos_hook_trigger(MGOS_HOOK_DEBUG_WRITE, &arg);
  }

  s_in_debug = false;
  mgos_unlock();
}

void mgos_debug_flush(void) {
  if (s_stdout_uart >= 0) mgos_uart_flush(s_stdout_uart);
  if (s_stderr_uart >= 0) mgos_uart_flush(s_stderr_uart);
}

static enum mgos_init_result mgos_init_debug_uart(int uart_no) {
  if (uart_no < 0) return MGOS_INIT_OK;
  /* If already initialized, don't touch. */
  if (mgos_uart_write_avail(uart_no) > 0) return MGOS_INIT_OK;
  struct mgos_uart_config ucfg;
  mgos_uart_config_set_defaults(uart_no, &ucfg);
  ucfg.baud_rate = MGOS_DEBUG_UART_BAUD_RATE;
  if (!mgos_uart_configure(uart_no, &ucfg)) {
    return MGOS_INIT_UART_FAILED;
  }
  return MGOS_INIT_OK;
}

enum mgos_init_result mgos_set_stdout_uart(int uart_no) {
  enum mgos_init_result r = mgos_init_debug_uart(uart_no);
  if (r == MGOS_INIT_OK) {
    s_stdout_uart = uart_no;
  }
  return r;
}

enum mgos_init_result mgos_set_stderr_uart(int uart_no) {
  enum mgos_init_result r = mgos_init_debug_uart(uart_no);
  if (r == MGOS_INIT_OK) {
    s_stderr_uart = uart_no;
  }
  return r;
}

int mgos_get_stdout_uart(void) {
  return s_stdout_uart;
}

int mgos_get_stderr_uart(void) {
  return s_stderr_uart;
}

void mgos_debug_suspend_uart(void) {
  s_uart_suspended++;
}

void mgos_debug_resume_uart(void) {
  s_uart_suspended--;
}

IRAM bool mgos_debug_uart_is_suspended(void) {
  return (s_uart_suspended > 0);
}

enum mgos_init_result mgos_debug_uart_init(void) {
  return mgos_init_debug_uart(MGOS_DEBUG_UART);
}
