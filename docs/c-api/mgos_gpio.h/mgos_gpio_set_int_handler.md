---
title: "mgos_gpio_set_int_handler()"
decl_name: "mgos_gpio_set_int_handler"
symbol_kind: "func"
signature: |
  bool mgos_gpio_set_int_handler(int pin, enum mgos_gpio_int_mode mode,
                                 mgos_gpio_int_handler_f cb, void *arg);
---

Install a GPIO interrupt handler.

Note that this will not enable the interrupt, this must be done explicitly
with mgos_gpio_enable_int. 

