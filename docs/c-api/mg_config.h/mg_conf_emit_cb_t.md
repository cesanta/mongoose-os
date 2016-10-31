---
title: "mg_conf_emit_cb_t"
decl_name: "mg_conf_emit_cb_t"
symbol_kind: "typedef"
signature: |
  typedef void (*mg_conf_emit_cb_t)(struct mbuf *data, void *param);
  void mg_conf_emit_cb(const void *cfg, const void *base,
                       const struct mg_conf_entry *schema, bool pretty,
                       struct mbuf *out, mg_conf_emit_cb_t cb, void *cb_param);
  bool mg_conf_emit_f(const void *cfg, const void *base,
                      const struct mg_conf_entry *schema, bool pretty,
                      const char *fname);
---

Emit config in 'cfg' according to rules in 'schema'.
Keys are only emitted if their values are different from 'base'.
If 'base' is NULL then all keys are emitted. 

