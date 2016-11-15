---
title: "miot_uart_dev_dispatch_bottom()"
decl_name: "miot_uart_dev_dispatch_bottom"
symbol_kind: "func"
signature: |
  void miot_uart_dev_dispatch_bottom(struct miot_uart_state *us);
---

Finish this dispatch. Set up interrupts depending on the state of rx/tx bufs:
 - If rx_buf has availabel space, RX ints should be enabled.
 - if there is data to send, TX empty ints should be enabled. 

