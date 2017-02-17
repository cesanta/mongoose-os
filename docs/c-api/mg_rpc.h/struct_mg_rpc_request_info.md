---
title: "struct mg_rpc_request_info"
decl_name: "struct mg_rpc_request_info"
symbol_kind: "struct"
signature: |
  struct mg_rpc_request_info {
    struct mg_rpc *rpc;
    int64_t id;           /* Request id. */
    struct mg_str src;    /* Source of the request. */
    struct mg_str tag;    /* Request tag. Opaque, should be passed back as is. */
    const char *args_fmt; /* Arguments format string */
    void *user_data;      /* Place to store user pointer. Not used by mg_rpc. */
  };
---

Incoming request info.
This structure is passed to request handlers and must be passed back
when a response is ready. 

