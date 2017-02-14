---
title: "Timer"
items:
---

 Timer API. Source C API is defined at:
 [mgos_timers.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_timers.h).



 **`Timer.set(milliseconds, repeat, handler)`**  -
 setup timer with `milliseconds` timeout and `handler` as a callback.
 `repeat` set to 1 will repeat a call infinitely, otherwise it's a one-off.

 Return value: numeric timer ID.

 Example:
 ```javascript
 // Call every second
 Timer.set(1000, 1, function() {
   let value = GPIO.toggle(2);
   print(value ? 'Tick' : 'Tock');
 }, null);
 ```

