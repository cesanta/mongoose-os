---
title: "miot_set_stdout_uart()"
decl_name: "miot_set_stdout_uart"
symbol_kind: "func"
signature: |
  enum miot_init_result miot_set_stdout_uart(int uart_no);
---

Set UART for stdout, stderr streams.
NB: Must accept negative values as "stdout/err disabled". 

