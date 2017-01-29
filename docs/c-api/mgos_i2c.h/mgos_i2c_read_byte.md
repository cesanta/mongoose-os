---
title: "mgos_i2c_read_byte()"
decl_name: "mgos_i2c_read_byte"
symbol_kind: "func"
signature: |
  uint8_t mgos_i2c_read_byte(struct mgos_i2c *conn, enum i2c_ack_type ack_type);
---

Read one byte from the bus, finish with an ack of the specified type.
ack_type can be "none" to prevent sending ack at all, in which case
this call must be followed up by i2c_send_ack. 

