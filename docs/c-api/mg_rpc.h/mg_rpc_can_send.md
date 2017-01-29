---
title: "mg_rpc_can_send()"
decl_name: "mg_rpc_can_send"
symbol_kind: "func"
signature: |
  bool mg_rpc_can_send(struct mg_rpc *c);
---

Returns true if the instance has an open default channel
and it's not currently busy. 

