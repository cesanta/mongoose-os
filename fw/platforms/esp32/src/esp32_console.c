/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp32/src/esp32_console.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/reent.h>

#include "freertos/FreeRTOS.h"

#include "esp_vfs.h"

#include "common/cs_dbg.h"
#include "fw/src/miot_init.h"
#include "fw/src/miot_uart.h"

portMUX_TYPE s_uart_mux = portMUX_INITIALIZER_UNLOCKED;
static int8_t s_stdout_uart = MIOT_DEBUG_UART;
static int8_t s_stderr_uart = MIOT_DEBUG_UART;

static int uart_open(const char *path, int flags, int mode) {
  (void) path;
  (void) flags;
  (void) mode;
  return 0;
}

static ssize_t uart_read(int fd, void *dst, size_t size) {
  errno = EBADF;
  return -1;
}

static size_t uart_write(int fd, const void *data, size_t size) {
  int uart_no = -1;
  if (fd == 1) {
    uart_no = s_stdout_uart;
  } else if (fd == 2) {
    uart_no = s_stderr_uart;
  } else {
    errno = EBADF;
    return -1;
  }
  portENTER_CRITICAL(&s_uart_mux);
  if (uart_no >= 0) size = miot_uart_write(uart_no, data, size);
  portEXIT_CRITICAL(&s_uart_mux);
  return size;
}

enum miot_init_result miot_set_stdout_uart(int uart_no) {
  enum miot_init_result r = miot_init_debug_uart(uart_no);
  if (r == MIOT_INIT_OK) {
    s_stdout_uart = uart_no;
  }
  return r;
}

enum miot_init_result miot_set_stderr_uart(int uart_no) {
  enum miot_init_result r = miot_init_debug_uart(uart_no);
  if (r == MIOT_INIT_OK) {
    s_stderr_uart = uart_no;
  }
  return r;
}

enum miot_init_result esp32_console_init() {
  esp_vfs_t vfs = {
      .open = &uart_open, .read = &uart_read, .write = &uart_write,
  };
  if (esp_vfs_register("/__miot_console", &vfs, NULL) != ESP_OK) {
    return MIOT_INIT_CONSOLE_INIT_FAILED;
  }
  int fd = open("/__miot_console/test", O_RDONLY);
  /* Open cannot fail if VFS was installed. */
  assert(fd >= 0);
  /*
   * Now the tricky part: poke our own FDs inside existing std{in,out,err} FILE
   * structs. We do not reallocate them because pointers have now been copied to
   * all the existing RTOS tasks.
   */
  portENTER_CRITICAL(&s_uart_mux);
  _GLOBAL_REENT->_stdin->_file = fd;
  _GLOBAL_REENT->_stdout->_file = fd + 1;
  _GLOBAL_REENT->_stderr->_file = fd + 2;
  portEXIT_CRITICAL(&s_uart_mux);
  return miot_init_debug_uart(MIOT_DEBUG_UART);
}
