/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_UART_H_
#define CS_FW_SRC_MGOS_UART_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "common/mbuf.h"
#include "common/platform.h"

#include "fw/src/mgos_init.h"

#if CS_PLATFORM == CS_P_CC3200
#include "fw/platforms/cc3200/src/cc3200_uart.h"
#elif CS_PLATFORM == CS_P_ESP32
#include "fw/platforms/esp32/src/esp32_uart.h"
#elif CS_PLATFORM == CS_P_ESP8266
#include "fw/platforms/esp8266/src/esp_uart.h"
#else
struct mgos_uart_dev_config {};
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mgos_uart_config {
  /* Baud rate, default: 115200 */
  int baud_rate;

  /* Size of the Rx buffer, default: 256 */
  int rx_buf_size;
  /* Enable flow control for Rx (RTS pin), default: off */
  bool rx_fc_ena;
  /*
   * Lingers for this many microseconds after RX fifo is empty in case more
   * data arrives. Default: 15.
   */
  int rx_linger_micros;

  /* Size of the Tx buffer, default: 256 */
  int tx_buf_size;
  /* Enable flow control for Tx (CTS pin), default: off */
  bool tx_fc_ena;
  
  /* Enable RS485 mode with TxEn (Tx Enable pin), default: off */
  bool rs485_ena;
  
  /* Remember whether RS485 TxEn (Tx Enable pin) is active, default: false */
  bool rs485_active;

  /* Platform-specific configuration options. */
  struct mgos_uart_dev_config dev;
};

/*
 * Apply given UART configuration.
 */
bool mgos_uart_configure(int uart_no, const struct mgos_uart_config *cfg);

/*
 * Fill provided `cfg` structure with the default values.
 */
void mgos_uart_config_set_defaults(int uart_no, struct mgos_uart_config *cfg);

/*
 * UART dispatcher gets when there is data in the input buffer
 * or space available in the output buffer.
 */
typedef void (*mgos_uart_dispatcher_t)(int uart_no, void *arg);
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
 * The _mbuf variant is a convenice function that reads into an mbuf.
 * Note: unlike write, read will not block if there are not enough bytes in the
 * input buffer.
 */
size_t mgos_uart_read(int uart_no, void *buf, size_t len);
size_t mgos_uart_read_mbuf(int uart_no, struct mbuf *mb, size_t len);
/* Returns the number of bytes available for reading. */
size_t mgos_uart_read_avail(int uart_no);

/* Controls whether UART receiver is enabled. */
void mgos_uart_set_rx_enabled(int uart_no, bool enabled);
bool mgos_uart_is_rx_enabled(int uart_no);

/* Flush the UART output buffer - waits for data to be sent. */
void mgos_uart_flush(int uart_no);

void mgos_uart_schedule_dispatcher(int uart_no, bool from_isr);

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

const struct mgos_uart_stats *mgos_uart_get_stats(int uart_no);

/* FFI-targeted API {{{ */

/*
 * Allocate and return new config structure filled with the default values.
 * Primarily useful for ffi.
 */
struct mgos_uart_config *mgos_uart_config_get_default(int uart_no);

/*
 * Set baud rate in the provided config structure.
 */
void mgos_uart_config_set_baud_rate(struct mgos_uart_config *cfg,
                                    int baud_rate);

/*
 * Set Rx params in the provided config structure: buffer size `rx_buf_size`,
 * whether Rx flow control is enabled (`rx_fc_ena`), and the number of
 * microseconds to linger after Rx fifo is empty (default: 15).
 */
void mgos_uart_config_set_rx_params(struct mgos_uart_config *cfg,
                                    int rx_buf_size, bool rx_fc_ena,
                                    int rx_linger_micros);

/*
 * Set Tx params in the provided config structure: buffer size `tx_buf_size`
 * and whether Tx flow control is enabled (`tx_fc_ena`)
 * 
 * and whether TxEn (TxEnable) pin is enabled for RS485 mode (`rs485_ena`) where
 * date is transmitted and received via the same twisted pair (half duplex).
 * For RS485 mode, it's assumed that one GPIO pin is connected to the 
 * interface device (e.g. MAX487) and is used to control DE and nRE pins
 * simultaneously.  High = transmit, LOW = receive.
 */
void mgos_uart_config_set_tx_params(struct mgos_uart_config *cfg,
                                    int tx_buf_size, bool tx_fc_ena, bool rs485_ena);

/* }}} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_UART_H_ */
