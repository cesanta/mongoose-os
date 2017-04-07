---
title: "mgos_uart_config_set_rx_params()"
decl_name: "mgos_uart_config_set_rx_params"
symbol_kind: "func"
signature: |
  void mgos_uart_config_set_rx_params(struct mgos_uart_config *cfg,
                                      int rx_buf_size, bool rx_fc_ena,
                                      int rx_linger_micros);
---

Set Rx params in the provided config structure: buffer size `rx_buf_size`,
whether Rx flow control is enabled (`rx_fc_ena`), and the number of
microseconds to linger after Rx fifo is empty (default: 15). 

