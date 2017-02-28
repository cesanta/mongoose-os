---
title: "mgos_i2c_read()"
decl_name: "mgos_i2c_read"
symbol_kind: "func"
signature: |
  bool mgos_i2c_read(struct mgos_i2c *i2c, uint16_t addr, void *data, size_t len,
                     bool stop);
---

Read specified number of bytes from the specified address.
Address should not include the R/W bit. If addr is -1, START is not
performed.
If |stop| is true, then at the end of the operation bus will be released. 

