---
title: "miot_wifi_scan_cb_t"
decl_name: "miot_wifi_scan_cb_t"
symbol_kind: "typedef"
signature: |
  typedef void (*miot_wifi_scan_cb_t)(const char **ssids, void *arg);
  void miot_wifi_scan(miot_wifi_scan_cb_t cb, void *arg);
---

Callback must be invoked, with list of SSIDs or NULL on error.
Caller owns SSIDS, they are not freed by the callee.
Invoking inline is ok. 

