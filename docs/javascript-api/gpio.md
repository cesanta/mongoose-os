---
title: GPIO
---

- `GPIO.setMode(pin, mode[, pull]) -> true or false`: Set pin mode. 

  `mode`: 
  * `GPIO.MODE_INPUT` enables input only, 
  * `GPIO.MODE_OUTPUT` enables output only.
  
  `pull`, when configuring pin for input, specifies pull-up mode : 
  * `GPIO.PULL_NONE` leaves pin floating, 
  * `GPIO.PULL_UP` connects pin to internal pullup resistor,
  * `GPIO.PULL_DOWN` connects pin to internal pulldown resistor.
- `GPIO.read(pin_num) -> 0 or 1`: Return GPIO pin level
- `GPIO.write(pin_num, true_or_false)`: Set GPIO pin level
- `GPIO.setISR(pin, isr_type, func) -> true or false`: Assign interrruption
  handler for pin. `isr_type` is a number:
  * `GPIO.INT_NONE` disables interrupts
  * `GPIO.INT_EDGE_POS` enables interupts on positive edge
  * `GPIO.INT_EDGE_NEG` - on negative edge
  * `GPIO.INT_EDGE_ANY` - on any edge
  * `GPIO.INT_LEVEL_LO` - on low level
  * `GPIO.INT_LEVEL_HI` - on high level
  * `GPIO.CLICK` - button mode
  `func` is a callback to be called on interrupt. Its prototype is `function myisr(pin, level)`.
  See [button helper](https://github.com/cesanta/mongoose-os/blob/master/fw/src/js/gpio.js)
  for `button mode` usage example.


Note: The pin numbers are based on the CPU. In the case of a NodeMCU board, D0 is pin 16, D1 is pin 5, etc. - see the diagram at  http://rogerbit.com/wprb/wp-content/uploads/2016/07/mculed.jpg .
