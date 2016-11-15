---
title: "miot_conf_emit_cb_t"
decl_name: "miot_conf_emit_cb_t"
symbol_kind: "typedef"
signature: |
  typedef void (*miot_conf_emit_cb_t)(struct mbuf *data, void *param);
  void miot_conf_emit_cb(const void *cfg, const void *base,
                         const struct miot_conf_entry *schema, bool pretty,
                         struct mbuf *out, miot_conf_emit_cb_t cb,
                         void *cb_param);
  bool miot_conf_emit_f(const void *cfg, const void *base,
                        const struct miot_conf_entry *schema, bool pretty,
                        const char *fname);
---

Emit config in 'cfg' according to rules in 'schema'.
Keys are only emitted if their values are different from 'base'.
If 'base' is NULL then all keys are emitted. 

