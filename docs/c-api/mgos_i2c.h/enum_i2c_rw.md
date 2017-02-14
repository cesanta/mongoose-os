---
title: "enum i2c_rw"
decl_name: "enum i2c_rw"
symbol_kind: "enum"
signature: |
  enum i2c_rw { I2C_READ = 1, I2C_WRITE = 0 };
  
---

Set i2c Start condition and send the address on the bus.
addr is the 7-bit address of the target.
rw selects read or write mode (1 = read, 0 = write).

Returns the ack type received in response to the address.
Returns I2C_NONE in case of invalid arguments.

TODO(rojer): 10-bit address support. 

