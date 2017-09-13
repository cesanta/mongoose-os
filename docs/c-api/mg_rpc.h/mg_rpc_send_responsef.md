---
title: "mg_rpc_send_responsef()"
decl_name: "mg_rpc_send_responsef"
symbol_kind: "func"
signature: |
  bool mg_rpc_send_responsef(struct mg_rpc_request_info *ri,
                             const char *result_json_fmt, ...);
---

Respond to an incoming request.
result_json_fmt can be NULL, in which case no result is included.
`ri` is freed by the call, so it's illegal to use it afterwards. 

