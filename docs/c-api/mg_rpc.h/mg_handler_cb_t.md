---
title: "mg_handler_cb_t"
decl_name: "mg_handler_cb_t"
symbol_kind: "typedef"
signature: |
  typedef void (*mg_handler_cb_t)(struct mg_rpc_request_info *ri, void *cb_arg,
                                  struct mg_rpc_frame_info *fi,
                                  struct mg_str args);
---

Signature of an incoming request handler.
Note that only request_info remains valid after return from this function,
frame_info and args will be invalidated. 

