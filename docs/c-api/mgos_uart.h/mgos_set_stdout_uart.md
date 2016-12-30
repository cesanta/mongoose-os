---
title: "mgos_set_stdout_uart()"
decl_name: "mgos_set_stdout_uart"
symbol_kind: "func"
signature: |
  enum mgos_init_result mgos_set_stdout_uart(int uart_no);
---

Set UART for stdout, stderr streams.
NB: Must accept negative values as "stdout/err disabled". 

