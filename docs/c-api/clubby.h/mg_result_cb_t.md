---
title: "mg_result_cb_t"
decl_name: "mg_result_cb_t"
symbol_kind: "typedef"
signature: |
  typedef void (*mg_result_cb_t)(struct clubby *c, void *cb_arg,
                                 struct clubby_frame_info *fi,
                                 struct mg_str result, int error_code,
                                 struct mg_str error_msg);
---

Signature of the function that receives response to a request. 

