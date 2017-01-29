---
title: "mgos_cb_t"
decl_name: "mgos_cb_t"
symbol_kind: "typedef"
signature: |
  typedef void (*mgos_cb_t)(void *arg);
  bool mgos_invoke_cb(mgos_cb_t cb, void *arg);
---

Invoke a callback in the main MGOS event loop.
Returns true if the callback has been scheduled for execution. 

