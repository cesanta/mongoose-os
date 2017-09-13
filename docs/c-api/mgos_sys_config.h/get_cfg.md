---
title: "get_cfg()"
decl_name: "get_cfg"
symbol_kind: "func"
signature: |
  struct sys_config *get_cfg(void);
---

Returns global instance of the config.
Note: Will return NULL before mgos_sys_config_init. 

