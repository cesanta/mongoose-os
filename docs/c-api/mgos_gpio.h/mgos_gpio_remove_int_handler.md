---
title: "mgos_gpio_remove_int_handler()"
decl_name: "mgos_gpio_remove_int_handler"
symbol_kind: "func"
signature: |
  void mgos_gpio_remove_int_handler(int pin, mgos_gpio_int_handler_f *old_cb,
                                    void **old_arg);
---

Removes a previosuly set interrupt handler.
If cb and arg are not NULL, they will contain previous handler and arg. 

