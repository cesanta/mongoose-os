---
title: "mgos_uart_dev_dispatch_bottom()"
decl_name: "mgos_uart_dev_dispatch_bottom"
symbol_kind: "func"
signature: |
  void mgos_uart_dev_dispatch_bottom(struct mgos_uart_state *us);
---

Finish this dispatch. Set up interrupts depending on the state of rx/tx bufs:
 - If rx_buf has availabel space, RX ints should be enabled.
 - if there is data to send, TX empty ints should be enabled. 

