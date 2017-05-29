---
title: "mgos_aws_shadow_get()"
decl_name: "mgos_aws_shadow_get"
symbol_kind: "func"
signature: |
  bool mgos_aws_shadow_get(void);
---

Request shadow state. Response will arrive via GET_ACCEPTED topic.
Note that MGOS automatically does this on every (re)connect if
aws.shadow.get_on_connect is true (default). 

