---
title: "clubby_send_responsef()"
decl_name: "clubby_send_responsef"
symbol_kind: "func"
signature: |
  bool clubby_send_responsef(struct clubby_request_info *ri,
                                const char *result_json_fmt, ...);
---

Respond to an incoming request.
result_json_fmt can be NULL, in which case no result is included. 

