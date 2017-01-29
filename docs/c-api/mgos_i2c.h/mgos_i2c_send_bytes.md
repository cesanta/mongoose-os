---
title: "mgos_i2c_send_bytes()"
decl_name: "mgos_i2c_send_bytes"
symbol_kind: "func"
signature: |
  enum i2c_ack_type mgos_i2c_send_bytes(struct mgos_i2c *conn, const uint8_t *buf,
                                        size_t buf_size);
---

Send array to I2C.
The ack type sent in response to the last transmitted byte is returned,
as well as number of bytes sent.
Receiver must positively acknowledge all bytes except, maybe, the last one.
If all the bytes have been sent, the return value is the acknowledgement
status of the last one (ACK or NAK). If a NAK was received before all the
bytes could be sent, ERR is returned instead. 

