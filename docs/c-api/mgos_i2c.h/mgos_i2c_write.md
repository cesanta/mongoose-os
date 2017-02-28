---
title: "mgos_i2c_write()"
decl_name: "mgos_i2c_write"
symbol_kind: "func"
signature: |
  bool mgos_i2c_write(struct mgos_i2c *i2c, uint16_t addr, const void *data,
                      size_t len, bool stop);
---

Write specified number of bytes from the specified address.
Address should not include the R/W bit. If addr is -1, START is not
performed.
If |stop| is true, then at the end of the operation bus will be released. 

