---
title: "GPIO"
items:
---

 GPIO API. Source C API is defined at
 [mgos_gpio.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_gpio.h)



 **`GPIO.set_mode(pin, mode)`** - set GPIO pin mode.
 `mode` can be either `GPIO.MODE_INPUT` or `GPIO.MODE_OUTPUT`.



 **`GPIO.set_pull(pin, type)`** - set GPIO pin pull type.
 `type` can be either `GPIO.PULL_NONE`, `GPIO.PULL_UP`, or `GPIO.PULL_DOWN`.



 **`GPIO.toggle(pin)`** - toggle the level of certain GPIO pin.
 Return value: 0 or 1, indicating the resulting pin level.



 **`GPIO.write(pin, level)`** - set GPIO pin level to either 0 or 1.
 Return value: none.



 **`GPIO.read(pin)`** - read GPIO pin level. Return value: 0 or 1.



 **`GPIO.enable_int(pin)`** - enable interrupts on GPIO pin.
 This function must be called AFTER the interrupt handler is installed.
 Return value: 1 in case of success, 0 otherwise.



 **`GPIO.disable_int(pin)`** - disable interrupts on GPIO pin.
 Return value: 1 in case of success, 0 otherwise.



 **`GPIO.set_int_handler(pin, mode, handler)`**  - install
 GPIO interrupt handler. `mode` could be one of: `GPIO.INT_NONE`,
 `GPIO.INT_EDGE_POS`, `GPIO.INT_EDGE_NEG`, `GPIO.INT_EDGE_ANY`,
 `GPIO.INT_LEVEL_HI`, `GPIO.INT_LEVEL_LO`.
 Return value: 1 in case of success, 0 otherwise.
 Example:
 ```javascript
 GPIO.set_mode(pin, GPIO.MODE_INPUT);
 GPIO.set_int_handler(pin, GPIO.INT_EDGE_NEG, function(pin) {
    print('Pin', pin, 'got interrupt');
 }, null);
 GPIO.enable_int(pin);
 ```



 **`GPIO.set_button_handler(pin, pull, intmode, period, handler)`**  - install
 GPIO button handler. `pull` is pull type, `intmode` is interrupt mode,
 `period` is debounce interval in milliseconds, handler is a function that
 receives pin number.
 Return value: 1 in case of success, 0 otherwise.
 Example:
 ```javascript
 GPIO.set_button_handler(pin, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 50, function(x) {
   print('Button press, pin: ', x);
 }, null);
 ```

