---
title: GPIO
---

- `GPIO.setmode(pin, mode, pull) -> true or false`: Set pin mode. 'mode' is
  number,  0 enables both input and output, 1 enables input only, 2 enabled
  output only, 3 enables interruptions on pin, see `GPUI.setisr`. `pull` is a
  number, 0 leaves pin floating, 1 connects pin to internal pullup resistor, 2
  connects pin to internal pulldown resistor.
- `GPIO.read(pin_num) -> 0 or 1`: Return GPIO pin level
- `GPIO.write(pin_num, true_or_false) -> true of false`: Set a given pin to
  `true` or `false`, return false if paramaters are incorrect
- `GPIO.setisr(pin, isr_type, func) -> true or false`: Assign interrruption
  handler for pin. `isr_type` is a number:
  * 0 disables interrupts
  * 1 enables interupts on positive edge
  * 2 - on negative edge
  * 3 - on any edge
  * 4 - on low level
  * 5 - on high level
  * 6 - button mode + `func` is callback to be called on interrupt, its
    prototype is `function myisr(pin, level)`. See [button
    helper](https://github.com/cesanta/smart.js/blob/master/platforms/esp8266/fs/gpio.js)
    for `button mode` usage example.

