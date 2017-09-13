---
title: "mgos_config_validator_fn"
decl_name: "mgos_config_validator_fn"
symbol_kind: "typedef"
signature: |
  typedef bool (*mgos_config_validator_fn)(const struct sys_config *cfg,
                                           char **msg);
  void mgos_register_config_validator(mgos_config_validator_fn fn);
---

Register a config validator.
Validators will be invoked before saving config and if any of them
returns false, config will not be saved.
An error message may be *msg may be set to error message.
Note: if non-NULL, *msg will be freed. Remember to use strdup and asprintf. 

