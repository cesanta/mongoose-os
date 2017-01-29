---
title: "mgos_i2c_send_byte()"
decl_name: "mgos_i2c_send_byte"
symbol_kind: "func"
signature: |
  enum i2c_ack_type mgos_i2c_send_byte(struct mgos_i2c *conn, uint8_t data);
---

Send one byte to i2c. Returns the type of ack that receiver sent. 

