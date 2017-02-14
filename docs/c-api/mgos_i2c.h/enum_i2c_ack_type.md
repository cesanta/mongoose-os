---
title: "enum i2c_ack_type"
decl_name: "enum i2c_ack_type"
symbol_kind: "enum"
signature: |
  enum i2c_ack_type { I2C_ACK = 0, I2C_NAK = 1, I2C_ERR = 2, I2C_NONE = 3 };
  
---

I2C_ACK - positive answer
I2C_NAK - negative anwser
I2C_ERR - a value that can be returned in case of an error.
I2C_NONE - a special value that can be provided to read functions
  to indicate that ack should not be sent at all. 

