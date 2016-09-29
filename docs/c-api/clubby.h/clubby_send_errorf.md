---
title: "clubby_send_errorf()"
decl_name: "clubby_send_errorf"
symbol_kind: "func"
signature: |
  bool clubby_send_errorf(struct clubby_request_info *ri, int error_code,
                          const char *error_msg_fmt, ...);
---

Send and error response to an incoming request.
error_msg_fmt is optional and can be NULL, in which case only code is sent. 

