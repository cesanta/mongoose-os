---
title: "miot_gpio_set_int_handler()"
decl_name: "miot_gpio_set_int_handler"
symbol_kind: "func"
signature: |
  bool miot_gpio_set_int_handler(int pin, enum miot_gpio_int_mode mode,
                                 miot_gpio_int_handler_f cb, void *arg);
---

Install a GPIO interrupt handler.

Calling with cb = NULL will remove a previously installed handler.
Note that this will not enable the interrupt, this must be done explicitly
with miot_gpio_enable_int. 

