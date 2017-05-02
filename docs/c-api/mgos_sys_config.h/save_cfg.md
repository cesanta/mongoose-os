---
title: "save_cfg()"
decl_name: "save_cfg"
symbol_kind: "func"
signature: |
  bool save_cfg(const struct sys_config *cfg, char **msg);
---

Save config. Performs diff against defaults and only saves diffs.
Reboot is required to reload the config.
If return value is false, a message may be provided in *msg.
If non-NULL, it must be free()d. 

