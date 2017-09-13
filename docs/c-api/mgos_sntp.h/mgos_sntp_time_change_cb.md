---
title: "mgos_sntp_time_change_cb"
decl_name: "mgos_sntp_time_change_cb"
symbol_kind: "typedef"
signature: |
  typedef void (*mgos_sntp_time_change_cb)(void *arg, double delta);
---

Callback is invoked immediately after a time adjustment has been made,
with the change as the argument. 

