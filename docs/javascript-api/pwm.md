---
title: PWM
---

- `PWM.set(pin, period, duty)`:
  - `pin`: GPIO number.
  - `period`: Period, in microseconds. 20 is the shortest supported and any
    number given will be rounded down to the nearest multiple of 10. `period =
    0` disables PWM on the pin (`duty = 0` is similar but does not perform
    internal cleanup).
  - `duty`: How many microseconds to spend in "1" state. Must be between `0`
    and `period` (inclusive). `duty = 0` is "always off", `duty = period` is
    "always on", `period / 2` is a square wave. Number will be rounded down to
    the nearest multiple of 10.
