/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_DEBUG_H_
#define CS_FW_SRC_MGOS_DEBUG_H_

#include <stdbool.h>
#include <stdlib.h>

#include "fw/src/mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MGOS_DEBUG_UART_BAUD_RATE
#define MGOS_DEBUG_UART_BAUD_RATE 115200
#endif

struct mgos_debug_hook_arg {
  int fd;
  const void *data;
  size_t len;
};

enum mgos_init_result mgos_debug_uart_init(void);
void mgos_debug_write(int fd, const void *buf, size_t len);
void mgos_debug_flush(void);

/* Set UART for stdout and stderr. < 0 = disable. */
enum mgos_init_result mgos_set_stdout_uart(int uart_no);
enum mgos_init_result mgos_set_stderr_uart(int uart_no);
int mgos_get_stdout_uart(void);
int mgos_get_stderr_uart(void);

void mgos_debug_suspend_uart(void);
void mgos_debug_resume_uart(void);
bool mgos_debug_uart_is_suspended(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_DEBUG_H_ */
