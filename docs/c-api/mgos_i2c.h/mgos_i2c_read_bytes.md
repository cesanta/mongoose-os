---
title: "mgos_i2c_read_bytes()"
decl_name: "mgos_i2c_read_bytes"
symbol_kind: "func"
signature: |
  void mgos_i2c_read_bytes(struct mgos_i2c *conn, size_t n, uint8_t *buf,
                           enum i2c_ack_type last_ack_type);
---

Read n bytes from the connection.
Each byte except the last one is acked, for the last one the user has
the choice whether to ack it, nack it or not send ack at all, in which case
this call must be followed up by i2c_send_ack. 

