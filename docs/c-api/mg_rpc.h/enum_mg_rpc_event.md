---
title: "enum mg_rpc_event"
decl_name: "enum mg_rpc_event"
symbol_kind: "enum"
signature: |
  enum mg_rpc_event {
    MG_RPC_EV_CHANNEL_OPEN,   /* struct mg_str *dst */
    MG_RPC_EV_CHANNEL_CLOSED, /* struct mg_str *dst */
  };
  
---

mg_rpc event observer. 

