---
title: "mgos_aws_shadow_updatef()"
decl_name: "mgos_aws_shadow_updatef"
symbol_kind: "func"
signature: |
  bool mgos_aws_shadow_updatef(uint64_t version, const char *state_jsonf, ...);
---

Send an update. Format string should define the value of the "state" key,
i.e. it should be an object with "reported" and/or "desired" keys, e.g.:
`mgos_aws_shadow_updatef("{reported:{foo: %d, bar: %d}}", foo, bar)`.
Response will arrive via UPDATE_ACCEPTED or REJECTED topic.
If you want the update to be aplied only if a particular version is current,
specify the version. Otherwise set it to 0 to apply to any version. 

