---
title: "mgos_conf_emit_cb_t"
decl_name: "mgos_conf_emit_cb_t"
symbol_kind: "typedef"
signature: |
  typedef void (*mgos_conf_emit_cb_t)(struct mbuf *data, void *param);
  void mgos_conf_emit_cb(const void *cfg, const void *base,
                         const struct mgos_conf_entry *schema, bool pretty,
                         struct mbuf *out, mgos_conf_emit_cb_t cb,
                         void *cb_param);
  bool mgos_conf_emit_f(const void *cfg, const void *base,
                        const struct mgos_conf_entry *schema, bool pretty,
                        const char *fname);
---

Emit config in 'cfg' according to rules in 'schema'.
Keys are only emitted if their values are different from 'base'.
If 'base' is NULL then all keys are emitted. 

