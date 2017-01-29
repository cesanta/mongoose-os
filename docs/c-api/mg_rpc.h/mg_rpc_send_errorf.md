---
title: "mg_rpc_send_errorf()"
decl_name: "mg_rpc_send_errorf"
symbol_kind: "func"
signature: |
  bool mg_rpc_send_errorf(struct mg_rpc_request_info *ri, int error_code,
                          const char *error_msg_fmt, ...);
---

Send and error response to an incoming request.
error_msg_fmt is optional and can be NULL, in which case only code is sent.
`ri` is freed by the call, so it's illegal to use it afterwards. 

