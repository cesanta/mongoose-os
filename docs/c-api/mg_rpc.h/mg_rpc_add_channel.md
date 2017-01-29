---
title: "mg_rpc_add_channel()"
decl_name: "mg_rpc_add_channel"
symbol_kind: "func"
signature: |
  void mg_rpc_add_channel(struct mg_rpc *c, const struct mg_str dst,
                          struct mg_rpc_channel *ch, bool is_trusted,
                          bool send_hello);
---

Adds a channel to the instance.
If dst is empty, it will be learned when first frame arrives from the other
end. A "default" channel, if present, will be used for frames that don't have
a better match.
If is_trusted is true, certain privileged commands will be allowed.
If send_hello is true, then a "hello" is sent to the channel and a successful
reply is required before it can be used. 

