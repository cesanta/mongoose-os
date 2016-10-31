---
title: "mg_uart_dev_dispatch_bottom()"
decl_name: "mg_uart_dev_dispatch_bottom"
symbol_kind: "func"
signature: |
  void mg_uart_dev_dispatch_bottom(struct mg_uart_state *us);
---

Finish this dispatch. Set up interrupts depending on the state of rx/tx bufs:
 - If rx_buf has availabel space, RX ints should be enabled.
 - if there is data to send, TX empty ints should be enabled. 

