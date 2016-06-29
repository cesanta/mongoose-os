---
title: GPIO
---

Example usage:

```c
  int pin = 12;
  sj_gpio_set_mode(pin, GPIO_MODE_OUTPUT, GPIO_PULL_FLOAT);
  sj_gpio_write(pin, GPIO_LEVEL_HIGH);
```

See [c_hello example](https://github.com/cesanta/mongoose-iot/blob/master/fw/examples/c_hello/src/app_main.c)
for a usage example.

See [C header file](https://github.com/cesanta/mongoose-iot/blob/master/fw/src/sj_gpio.h)
for more information.
