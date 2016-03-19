---
title: Built-in functions
---

- `usleep(num_microseconds) -> undefined`: Sleep for `num_microseconds`
- `dsleep(num_microseconds [, dsleep_option]) -> undefined`: Deep sleep for
  `num_microseconds`. If `dsleep_option` is specified, ESP's
  `system_deep_sleep_set_option(dsleep_option)` is called prior to going to
  sleep. The most useful seems to be 4 (keep RF off on wake up, reduces power
  consumption).
- `setTimeout(callback, ms) -> id`: Schedule function call after `ms`
  milliseconds. `id` can be passed to `clearTimeout` to cancel the call.
- `setInterval(callback, ms) -> id`: Schedule function to be repeatedly invoked
  every `ms` milliseconds. `id` can be passed to `clearInterval` to cancel the
  call.
- `clearTimeout(id), clearInterval(id) -> undefined`: Stops timers created by
  `setTimeout` and `setInterval`.
- `print(arg1, ...) -> undefined`: Stringify and print arguments to the command
  prompt
- `GC.stat() -> stats_object`: Return current memory usage
- `Debug.mode(mode) -> status_number`: Set redirection for system and custom
  (stderr) error logging: `Debug.OFF` or `0` = /dev/null; `Debug.OUT` or `1` =
  uart0, `Debug.ERR` or `2` = uart1
- `Debug.print(...)`: Print information to current debug output (set by
  `Debug.mode`)

