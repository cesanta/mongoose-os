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
#include <stdlib.h>

#include "mgos_debug_hal.h"
#include "mgos_debug_internal.h"

#include "common/cs_dbg.h"

#include "mongoose.h"

#include "mgos_core_dump.h"
#include "mgos_event.h"
#include "mgos_features.h"
#include "mgos_sys_config.h"
#include "mgos_system.h"
#include "mgos_time.h"
#include "mgos_uart.h"

#ifndef IRAM
#define IRAM
#endif

static struct mgos_rlock_type *s_debug_lock = NULL;
static int8_t s_stdout_uart = MGOS_DEBUG_UART;
static int8_t s_stderr_uart = MGOS_DEBUG_UART;
static int8_t s_uart_suspended = 0;
static int8_t s_in_debug = 0;

/* This overrides default dummy implementations defined in cs_dbg.c */
void cs_log_lock(void) {
  mgos_rlock(s_debug_lock);
}

void cs_log_unlock(void) {
  mgos_runlock(s_debug_lock);
}

/* From cs_dbg.c */
#if CS_ENABLE_STDIO
extern enum cs_log_level cs_log_cur_msg_level;
#endif

/* NB: Avoid *printf in this function, they use too much stack. */
void mgos_debug_write(int fd, const void *data, size_t len) {
  char buf[MGOS_DEBUG_TMP_BUF_SIZE];
  enum cs_log_level old_level;
  int uart_no = -1;
  cs_log_lock();
  if (s_in_debug) {
    cs_log_unlock();
    return;
  }
  s_in_debug = true;
  old_level = cs_log_level;
  cs_log_level = LL_NONE;
  if (s_uart_suspended <= 0) {
    if (fd == 1) {
      uart_no = s_stdout_uart;
    } else if (fd == 2) {
      uart_no = s_stderr_uart;
    }
  }
  if (uart_no >= 0) {
    mgos_uart_write(uart_no, data, len);
    mgos_uart_flush(uart_no);
  }
  if (!mgos_sys_config_is_initialized()) {
    goto out_unlock;
  }
#if MGOS_ENABLE_DEBUG_UDP
  if (mgos_sys_config_get_debug_udp_log_addr() != NULL &&
      cs_log_cur_msg_level <= mgos_sys_config_get_debug_udp_log_level()) {
    /* NB: Avoid snprintf, it uses too much stack. */
    static uint32_t s_seq = 0;
    buf[0] = '\0';
    const char *id = mgos_sys_config_get_device_id();
    uint64_t ts_us = mgos_uptime_micros();
    uint64_t ts_us_s = ts_us / 1000000;
    unsigned int ts_ms = ((unsigned int) (ts_us - (ts_us_s * 1000000))) / 1000;
    strncat(buf, (id ? id : "-"), sizeof(buf) - 1);
    int n = strlen(buf);
    buf[n++] = ' ';
    n += mgos_utoa(s_seq, buf + n, 10);
    buf[n++] = ' ';
    n += mgos_utoa((unsigned int) ts_us_s, buf + n, 10);
    buf[n++] = '.';
    if (ts_ms < 100) {
      buf[n++] = '0';
      if (ts_ms < 10) {
        buf[n++] = '0';
      }
    }
    n += mgos_utoa(ts_ms, buf + n, 10);
    buf[n++] = ' ';
    buf[n++] = '0' + fd;
    buf[n++] = ' ';
    buf[n++] =
        '0' +
        (cs_log_cur_msg_level != LL_NONE ? (char) cs_log_cur_msg_level : 0);
    buf[n++] = '|';
    mgos_debug_udp_send(mg_mk_str_n(buf, n), mg_mk_str_n(data, len));
    s_seq++;
  }
#endif /* MGOS_ENABLE_DEBUG_UDP */
  /* Invoke all registered debug_write hooks */
  /* Only send LL_INFO messages and below, to avoid loops. */
#if CS_ENABLE_STDIO
  if (cs_log_cur_msg_level <= mgos_sys_config_get_debug_event_level())
#endif
  {
    struct mgos_debug_hook_arg arg = {
      .buf = buf,
      .fd = fd,
#if CS_ENABLE_STDIO
      .level = cs_log_cur_msg_level,
#endif
      .data = data,
      .len = len,
    };
    mgos_event_trigger(MGOS_EVENT_LOG, &arg);
  }

out_unlock:
  cs_log_level = old_level;
  s_in_debug = false;
  cs_log_unlock();
}

void mgos_debug_flush(void) {
  if (s_stdout_uart >= 0) mgos_uart_flush(s_stdout_uart);
  if (s_stderr_uart >= 0) mgos_uart_flush(s_stderr_uart);
}

bool mgos_debug_uart_custom_cfg(int uart_no, struct mgos_uart_config *cfg) WEAK;
bool mgos_debug_uart_custom_cfg(int uart_no, struct mgos_uart_config *cfg) {
  (void) uart_no;
  (void) cfg;
  return true;
}

#if CS_PLATFORM != CS_P_ESP32
void __assert_func(const char *file, int line, const char *func,
                   const char *failedexpr) {
#if CS_ENABLE_STDIO
  mgos_cd_printf("assertion \"%s\" failed: file \"%s\", line %d%s%s\n",
                 failedexpr, file, line, (func ? ", function: " : ""),
                 (func ? func : ""));
#else
  (void) file;
  (void) line;
  (void) func;
  (void) failedexpr;
#endif
  abort();
}
#endif

static bool mgos_init_debug_uart(int uart_no) {
  if (uart_no < 0) return true;
  /* If already initialized, don't touch. */
  if (mgos_uart_write_avail(uart_no) > 0) return true;
  struct mgos_uart_config ucfg;
  mgos_uart_config_set_defaults(uart_no, &ucfg);
  ucfg.baud_rate = MGOS_DEBUG_UART_BAUD_RATE;
  if (!mgos_debug_uart_custom_cfg(uart_no, &ucfg)) {
    return false;
  }
  if (!mgos_uart_configure(uart_no, &ucfg)) {
    return false;
  }
  return true;
}

bool mgos_set_stdout_uart(int uart_no) {
  bool r = mgos_init_debug_uart(uart_no);
  if (r) {
    s_stdout_uart = uart_no;
  }
  return r;
}

bool mgos_set_stderr_uart(int uart_no) {
  bool r = mgos_init_debug_uart(uart_no);
  if (r) {
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

enum mgos_init_result mgos_debug_init(void) {
  s_debug_lock = mgos_rlock_create();
  return MGOS_INIT_OK;
}

enum mgos_init_result mgos_debug_uart_init(void) {
  bool res = mgos_init_debug_uart(MGOS_DEBUG_UART);
  if (res) {
    s_stdout_uart = MGOS_DEBUG_UART;
    s_stderr_uart = MGOS_DEBUG_UART;
  }
  return (res ? MGOS_INIT_OK : MGOS_INIT_UART_FAILED);
}

/* For FFI */
const void *mgos_debug_event_get_ptr(struct mgos_debug_hook_arg *arg) {
  return arg->data;
}

/* For FFI */
int mgos_debug_event_get_len(struct mgos_debug_hook_arg *arg) {
  return arg->len;
}
