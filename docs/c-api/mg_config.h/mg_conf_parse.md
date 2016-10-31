---
title: "mg_conf_parse()"
decl_name: "mg_conf_parse"
symbol_kind: "func"
signature: |
  bool mg_conf_parse(const struct mg_str json, const char *acl,
                     const struct mg_conf_entry *schema, void *cfg);
---

Parses config in 'json' into 'cfg' according to rules defined in 'schema' and
checking keys against 'acl'. 

