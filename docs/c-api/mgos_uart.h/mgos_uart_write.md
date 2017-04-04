---
title: "mgos_uart_write()"
decl_name: "mgos_uart_write"
symbol_kind: "func"
signature: |
  size_t mgos_uart_write(int uart_no, const void *buf, size_t len);
---

Write data to the UART.
Note: if there is enough space in the output buffer, the call will return
immediately, otherwise it will wait for buffer to drain.
If you want the call to not block, check mgos_uart_write_avail() first. 

