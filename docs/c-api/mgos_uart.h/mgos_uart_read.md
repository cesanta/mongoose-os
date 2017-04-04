---
title: "mgos_uart_read()"
decl_name: "mgos_uart_read"
symbol_kind: "func"
signature: |
  size_t mgos_uart_read(int uart_no, void *buf, size_t len);
---

Read data from UART input buffer.
The _mbuf variant is a convenice function that reads into an mbuf.
Note: unlike write, read will not block if there are not enough bytes in the
input buffer. 

