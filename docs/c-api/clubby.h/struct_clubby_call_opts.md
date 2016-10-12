---
title: "struct clubby_call_opts"
decl_name: "struct clubby_call_opts"
symbol_kind: "struct"
signature: |
  struct clubby_call_opts {
    struct mg_str dst; /* Destination ID. If not provided, cloud is implied. */
  };
---

Send a request.
cb is optional, in which case request is sent but response is not required.
opts can be NULL, in which case defaults are used. 

