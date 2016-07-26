---
title: GPIO
---

- `GPIO.setMode(pin, mode[, pull]) -> true or false`: Set pin mode. `mode`: `GPIO.INOUT`
  enables both input and output, `GPIO.IN` enables input only, `GPIO.OUT` enables
  output only. `pull`, when configuring pin for input, specifies pull-up mode : `GPIO.FLOAT`
  leaves pin floating, `GPIO.PULLUP` connects pin to internal pullup resistor,
  `GPIO.PULLDOWN` connects pin to internal pulldown resistor.
- `GPIO.read(pin_num) -> 0 or 1`: Return GPIO pin level
- `GPIO.write(pin_num, true_or_false) -> true of false`: Set a given pin to
  `true` or `false`, return false if paramaters are incorrect
- `GPIO.setISR(pin, isr_type, func) -> true or false`: Assign interrruption
  handler for pin. `isr_type` is a number:
  * `GPIO.OFF` disables interrupts
  * `GPIO.POSEDGE` enables interupts on positive edge
  * `GPIO.NEGEDGE` - on negative edge
  * `GPIO.ANYEDGE` - on any edge
  * `GPIO.LOLEVEL` - on low level
  * `GPIO.HILEVEL` - on high level
  * `GPIO.CLICK` - button mode
  `func` is a callback to be called on interrupt. Its prototype is `function myisr(pin, level)`.
  See [button helper](https://github.com/cesanta/mongoose-iot/blob/master/fw/src/js/gpio.js)
  for `button mode` usage example.

