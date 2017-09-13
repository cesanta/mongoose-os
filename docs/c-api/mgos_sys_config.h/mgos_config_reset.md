---
title: "mgos_config_reset()"
decl_name: "mgos_config_reset"
symbol_kind: "func"
signature: |
  void mgos_config_reset(int level);
---

Reset config down to and including |level|.
0 - defaults, 1-8 - vendor levels, 9 - user.
mgos_config_reset(MGOS_CONFIG_LEVEL_USER) will wipe user settings. 

