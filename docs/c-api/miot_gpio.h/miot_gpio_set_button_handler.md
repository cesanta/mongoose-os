---
title: "miot_gpio_set_button_handler()"
decl_name: "miot_gpio_set_button_handler"
symbol_kind: "func"
signature: |
  bool miot_gpio_set_button_handler(int pin, enum miot_gpio_pull_type pull_type,
                                    enum miot_gpio_int_mode int_mode,
                                    int debounce_ms, miot_gpio_int_handler_f cb,
                                    void *arg);
---

Handle a button on the specified pin.
Configures the pin for input with specified pull-up and performs debouncing:
upon first triggering user's callback is invoked immediately but further
interrupts are inhibited for the following debounce_ms millseconds.
Typically 50 ms of debouncing time is sufficient.
int_mode is one of the MIOT_GPIO_INT_EDGE_* values and will specify whether
the handler triggers when button is pressed, released or both.
Which is which depends on how the button is wired: if the normal state is
pull-up (typical), then MIOT_GPIO_INT_EDGE_NEG is press and _POS is release.

Calling with cb = NULL will remove a previously installed handler.

Note: implicitly enables the interrupt. 

