---
title: "mgos_uart_config_set_tx_params()"
decl_name: "mgos_uart_config_set_tx_params"
symbol_kind: "func"
signature: |
  void mgos_uart_config_set_tx_params(struct mgos_uart_config *cfg,
                                      int tx_buf_size, bool tx_fc_ena);
---

Set Tx params in the provided config structure: buffer size `tx_buf_size`
and whether Tx flow control is enabled (`tx_fc_ena`). 

