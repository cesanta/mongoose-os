/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_UART_HAL_H_
#define CS_FW_SRC_MGOS_UART_HAL_H_

#include "fw/src/mgos_uart.h"

/* Set device-specific defaults. */
void mgos_uart_hal_set_defaults(struct mgos_uart_config *cfg);

/* Device-specific (de)initialization. */
bool mgos_uart_hal_init(struct mgos_uart_state *us);
void mgos_uart_hal_deinit(struct mgos_uart_state *us);

/* Read any available chars into rx_buf. Ints should be kept disabled. */
void mgos_uart_hal_dispatch_rx_top(struct mgos_uart_state *us);
/* Push chars from tx_buf out. Ints should be kept disabled. */
void mgos_uart_hal_dispatch_tx_top(struct mgos_uart_state *us);
/*
 * Finish this dispatch. Set up interrupts depending on the state of rx/tx bufs:
 *  - If rx_buf has availabel space, RX ints should be enabled.
 *  - if there is data to send, TX empty ints should be enabled.
 */
void mgos_uart_hal_dispatch_bottom(struct mgos_uart_state *us);

/* Wait for the FIFO to drain */
void mgos_uart_hal_flush_fifo(struct mgos_uart_state *us);

void mgos_uart_hal_set_rx_enabled(struct mgos_uart_state *us, bool enabled);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_UART_HAL_H_ */
