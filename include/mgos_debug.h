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

#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "common/cs_dbg.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MGOS_DEBUG_UART_BAUD_RATE
#define MGOS_DEBUG_UART_BAUD_RATE 115200
#endif

#ifndef MGOS_DEBUG_TMP_BUF_SIZE
#define MGOS_DEBUG_TMP_BUF_SIZE 96
#endif

/*
 * Arguments for the `MGOS_EVENT_LOG` event, see `mgos_event_add_handler()`.
 */
struct mgos_debug_hook_arg {
  int fd;
  enum cs_log_level level;
  const char *data;
  size_t len;

  /*
   * Buffer which hooks can use for their own needs; size of the buffer is
   * MGOS_DEBUG_TMP_BUF_SIZE.
   */
  char *buf;
};

/*
 * Write debug info `buf`, `len` to the given file descriptor `fd`.
 */
void mgos_debug_write(int fd, const void *buf, size_t len);

/*
 * Flush debug UARTs, both stdout and stderr.
 */
void mgos_debug_flush(void);

/* Set UART for stdout. Negative value disables stdout. */
bool mgos_set_stdout_uart(int uart_no);

/* Set UART for stderr. Negative value disables stderr. */
bool mgos_set_stderr_uart(int uart_no);

/* Get stdout UART number; -1 indicates that stdout is disabled. */
int mgos_get_stdout_uart(void);

/* Get stderr UART number; -1 indicates that stderr is disabled. */
int mgos_get_stderr_uart(void);

/*
 * Suspend UART output (both stdout and stderr); see
 * `mgos_debug_resume_uart()`. Nested suspension is supported: UART needs to be
 * resumed as many times as it was suspended.
 */
void mgos_debug_suspend_uart(void);

/*
 * Resume previously suspended UART output, see `mgos_debug_suspend_uart()`.
 */
void mgos_debug_resume_uart(void);

/*
 * Returns whether UART output is suspended at the moment.
 */
bool mgos_debug_uart_is_suspended(void);

/*
 * User can implement this to apply custom config during debug uart init
 * (e.g. remap pins).
 */
struct mgos_uart_config;
bool mgos_debug_uart_custom_cfg(int uart_no, struct mgos_uart_config *cfg);

#ifdef __cplusplus
}
#endif /* __cplusplus */
