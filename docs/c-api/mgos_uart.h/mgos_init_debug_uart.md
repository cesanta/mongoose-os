---
title: "mgos_init_debug_uart()"
decl_name: "mgos_init_debug_uart"
symbol_kind: "func"
signature: |
  enum mgos_init_result mgos_init_debug_uart(int uart_no);
---

Will init uart_no with default config + MGOS_DEBUG_UART_BAUD_RATE
unless it's already inited. 

