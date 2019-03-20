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

/*
 * See https://en.wikipedia.org/wiki/Universal_asynchronous_receiver-transmitter
 * for more information about UART.
 */

#ifndef CS_FW_INCLUDE_MGOS_UART_H_
#define CS_FW_INCLUDE_MGOS_UART_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "common/mbuf.h"
#include "common/platform.h"

#if CS_PLATFORM == CS_P_CC3200 || CS_PLATFORM == CS_P_CC3220
#include "cc32xx_uart.h"
#elif CS_PLATFORM == CS_P_ESP32
#include "esp32_uart.h"
#elif CS_PLATFORM == CS_P_ESP8266
#include "esp_uart.h"
#elif defined(RS14100)
#include "rs14100_uart.h"
#elif CS_PLATFORM == CS_P_STM32
#include "stm32_uart.h"
#else
struct mgos_uart_dev_config {};
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* UART flow control type */
enum mgos_uart_fc_type {
  MGOS_UART_FC_NONE = 0,
  MGOS_UART_FC_HW = 1, /* CTS/RTS */
  MGOS_UART_FC_SW = 2, /* XON/XOFF */
};

/* UART parity */
enum mgos_uart_parity {
  MGOS_UART_PARITY_NONE = 0,
  MGOS_UART_PARITY_EVEN = 1,
  MGOS_UART_PARITY_ODD = 2,
};

/* UART stop bits mode */
enum mgos_uart_stop_bits {
  MGOS_UART_STOP_BITS_1 = 1, /* So that 1 means 1 bit and 2 means 2. */
  MGOS_UART_STOP_BITS_2 = 2,
  MGOS_UART_STOP_BITS_1_5 = 3,
};

struct mgos_uart_config {
  int baud_rate;                      /* Baud rate. Default: 115200 */
  int num_data_bits;                  /* Number of data bits, 5-8. Default: 8 */
  enum mgos_uart_parity parity;       /* Parity. Default: none */
  enum mgos_uart_stop_bits stop_bits; /* Number of stop bits. Default: 1 */

  /* Size of the Rx buffer, default: 256 */
  int rx_buf_size;
  /* Enable flow control for Rx (RTS pin), default: off */
  enum mgos_uart_fc_type rx_fc_type;
  /*
   * Lingers for this many microseconds after RX fifo is empty in case more
   * data arrives. Default: 15.
   */
  int rx_linger_micros;

  /* Size of the Tx buffer, default: 256 */
  int tx_buf_size;
  /* Enable flow control for Tx (CTS pin), default: off */
  enum mgos_uart_fc_type tx_fc_type;

  /* Platform-specific configuration options. */
  struct mgos_uart_dev_config dev;
};

#define MGOS_UART_XON_CHAR 0x11
#define MGOS_UART_XOFF_CHAR 0x13

/*
 * Apply given UART configuration.
 *
 * Example:
 * ```c
 * int uart_no = 0;
 *
 * struct mgos_uart_config ucfg;
 * mgos_uart_config_set_defaults(uart_no, &ucfg);
 *
 * ucfg.baud_rate = 115200;
 * ucfg.rx_buf_size = 1500;
 * ucfg.tx_buf_size = 1500;
 *
 * if (!mgos_uart_configure(uart_no, &ucfg)) {
 *   LOG(LL_ERROR, ("Failed to configure UART%d", uart_no));
 * }
 * ```
 */
bool mgos_uart_configure(int uart_no, const struct mgos_uart_config *cfg);

/*
 * Fill provided `cfg` structure with the default values. See example above.
 */
void mgos_uart_config_set_defaults(int uart_no, struct mgos_uart_config *cfg);

/*
 * Fill provided `cfg` structure with the current UART config.
 * Returns false if the specified UART has not bee configured yet.
 */
bool mgos_uart_config_get(int uart_no, struct mgos_uart_config *cfg);

/* UART dispatcher signature, see `mgos_uart_set_dispatcher()` */
typedef void (*mgos_uart_dispatcher_t)(int uart_no, void *arg);

/*
 * Set UART dispatcher: a callback which gets called when there is data in the
 * input buffer or space available in the output buffer.
 */
void mgos_uart_set_dispatcher(int uart_no, mgos_uart_dispatcher_t cb,
                              void *arg);

/*
 * Write data to the UART.
 * Note: if there is enough space in the output buffer, the call will return
 * immediately, otherwise it will wait for buffer to drain.
 * If you want the call to not block, check mgos_uart_write_avail() first.
 */
size_t mgos_uart_write(int uart_no, const void *buf, size_t len);

/* Returns amount of space availabe in the output buffer. */
size_t mgos_uart_write_avail(int uart_no);

/*
 * Write data to UART, printf style.
 * Note: currently this requires that data is fully rendered in memory before
 * sending. There is no fixed limit as heap allocation is used, but be careful
 * when printing longer strings.
 */
int mgos_uart_printf(int uart_no, const char *fmt, ...);

/*
 * Read data from UART input buffer.
 * Note: unlike write, read will not block if there are not enough bytes in the
 * input buffer.
 */
size_t mgos_uart_read(int uart_no, void *buf, size_t len);

/* Like `mgos_uart_read`, but reads into an mbuf. */
size_t mgos_uart_read_mbuf(int uart_no, struct mbuf *mb, size_t len);

/* Returns the number of bytes available for reading. */
size_t mgos_uart_read_avail(int uart_no);

/* Controls whether UART receiver is enabled. */
void mgos_uart_set_rx_enabled(int uart_no, bool enabled);

/* Returns whether UART receiver is enabled. */
bool mgos_uart_is_rx_enabled(int uart_no);

/* Flush the UART output buffer - waits for data to be sent. */
void mgos_uart_flush(int uart_no);

/* Schedule a call to dispatcher on the next `mongoose_poll` */
void mgos_uart_schedule_dispatcher(int uart_no, bool from_isr);

/* UART statistics */
struct mgos_uart_stats {
  uint32_t ints;

  uint32_t rx_ints;
  uint32_t rx_bytes;
  uint32_t rx_overflows;
  uint32_t rx_linger_conts;

  uint32_t tx_ints;
  uint32_t tx_bytes;
  uint32_t tx_throttles;

  void *dev_data;
};

/* Get UART statistics */
const struct mgos_uart_stats *mgos_uart_get_stats(int uart_no);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_INCLUDE_MGOS_UART_H_ */
