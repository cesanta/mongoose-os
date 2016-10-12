---
title: "struct clubby_request_info"
decl_name: "struct clubby_request_info"
symbol_kind: "struct"
signature: |
  struct clubby_request_info {
    struct clubby *clubby;
    struct mg_str src; /* Source of the request. */
    int64_t id;        /* Request id. */
    void *user_data;   /* Place to store user pointer. Not used by Clubby. */
  };
---

Incoming request info.
This structure is passed to request handlers and must be passed back
when a response is ready. 

