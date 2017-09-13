---
title: "mg_rpc_parse_frame()"
decl_name: "mg_rpc_parse_frame"
symbol_kind: "func"
signature: |
  bool mg_rpc_parse_frame(const struct mg_str f, struct mg_rpc_frame *frame);
---

Parses frame `f` and stores result into `frame`. Returns true in case of
success, false otherwise. 

