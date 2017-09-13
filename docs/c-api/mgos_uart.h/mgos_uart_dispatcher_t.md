---
title: "mgos_uart_dispatcher_t"
decl_name: "mgos_uart_dispatcher_t"
symbol_kind: "typedef"
signature: |
  typedef void (*mgos_uart_dispatcher_t)(int uart_no, void *arg);
  void mgos_uart_set_dispatcher(int uart_no, mgos_uart_dispatcher_t cb,
                                void *arg);
---

UART dispatcher gets when there is data in the input buffer
or space available in the output buffer. 

