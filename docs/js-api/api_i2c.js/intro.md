---
title: "I2C"
items:
---

 I2C API. Source C API is defined at:
 [mgos_i2c.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_i2c.h)



 **`I2C.get_default()`** - get I2C handle. Return value: opaque pointer.



 **`I2C.close()`** - close I2C handle. Return value: none.



 **`I2C.write(handle, addr, buf, size, stop)`** - Send a byte array to I2C.
 If stop is 1, the bus will be released at the end.
 Return value: success, true/false.



 **`I2C.read(handle, ack_type)`** - Read specified number of bytes
 from the specified address.
 If stop is 1, the bus will be released at the end.
 Return value: success, true/false.



 **`I2C.stop(handle)`** - Set i2c Stop condition. Releases the bus.
 Return value: none.

