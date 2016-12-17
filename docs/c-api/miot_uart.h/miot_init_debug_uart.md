---
title: "miot_init_debug_uart()"
decl_name: "miot_init_debug_uart"
symbol_kind: "func"
signature: |
  enum miot_init_result miot_init_debug_uart(int uart_no);
---

Will init uart_no with default config + MIOT_DEBUG_UART_BAUD_RATE
unless it's already inited. 

