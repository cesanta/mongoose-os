---
title: "mgos_uart_printf()"
decl_name: "mgos_uart_printf"
symbol_kind: "func"
signature: |
  int mgos_uart_printf(int uart_no, const char *fmt, ...);
---

Write data to UART, printf style.
Note: currently this requires that data is fully rendered in memory before
sending. There is no fixed limit as heap allocation is used, but be careful
when printing longer strings. 

