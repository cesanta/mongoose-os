---
title: "mgos_i2c_send_ack()"
decl_name: "mgos_i2c_send_ack"
symbol_kind: "func"
signature: |
  void mgos_i2c_send_ack(struct mgos_i2c *conn, enum i2c_ack_type ack_type);
---

Send an ack of the specified type. Meant to be used after i2c_read_byte{,s}
with ack_type of "none". 

