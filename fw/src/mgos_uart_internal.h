/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_UART_INTERNAL_H_
#define CS_FW_SRC_MGOS_UART_INTERNAL_H_

#include <stdbool.h>

#include "mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mgos_uart_config;

enum mgos_init_result mgos_uart_init(void);

/* FFI-targeted API {{{ */

/*
 * Allocate and return new config structure filled with the default values.
 * Primarily useful for ffi.
 */
struct mgos_uart_config *mgos_uart_config_get_default(int uart_no);

/*
 * Set baud rate in the provided config structure.
 */
void mgos_uart_config_set_basic_params(struct mgos_uart_config *cfg,
                                       int baud_rate, int num_data_bits,
                                       int parity, int num_stop_bits);

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
 * and whether Tx flow control is enabled (`tx_fc_ena`).
 */
void mgos_uart_config_set_tx_params(struct mgos_uart_config *cfg,
                                    int tx_buf_size, bool tx_fc_ena);

/* }}} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_UART_INTERNAL_H_ */
