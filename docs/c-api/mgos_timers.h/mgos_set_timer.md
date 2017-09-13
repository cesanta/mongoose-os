---
title: "mgos_set_timer()"
decl_name: "mgos_set_timer"
symbol_kind: "func"
signature: |
  mgos_timer_id mgos_set_timer(int msecs, int repeat, timer_callback cb,
                               void *arg);
---

Setup timer with `msecs` timeout and `cb` as a callback.

`repeat` set to 1 will repeat a call infinitely, otherwise it's a one-off.
`arg` is a parameter to pass to `cb`. Return numeric timer ID. 

