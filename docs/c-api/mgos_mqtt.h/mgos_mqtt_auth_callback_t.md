---
title: "mgos_mqtt_auth_callback_t"
decl_name: "mgos_mqtt_auth_callback_t"
symbol_kind: "typedef"
signature: |
  typedef void (*mgos_mqtt_auth_callback_t)(char **client_id, char **user,
                                            char **pass, void *arg);
  void mgos_mqtt_set_auth_callback(mgos_mqtt_auth_callback_t cb, void *cb_arg);
---

Set authentication callback. It is invoked when CONNECT message is about to
be sent, values from *user and *pass, if non-NULL, will be sent along.
Note: *user and *pass must be heap-allocated and will be free()d. 

