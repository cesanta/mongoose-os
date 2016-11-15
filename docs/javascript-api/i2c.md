---
title: I2C
---

Constants:

- `I2C.READ`, `I2C.WRITE`: Communication mode constants
- `I2C.ACK`, `I2C.NAK`, `I2C.NONE`: Acknowledgement types
- `var i2c = new I2C(sda_gpio, scl_gpio)`: An I2C constructor

Low-level API:

- `i2c.start(addr, mode) -> ackType`: Claims the bus, puts the slave address on
  it and reads ack/nak. addr is the 7-bit address (no r/w bit), mode is either
  `I2C.READ` or `I2C.WRITE`.
- `i2c.stop()`: Puts stop condition on the bus and releases it.
- `i2c.send(data) -> ackType`:

  Send data to the bus. If `data` is a number between 0 and 255, a single byte
  is sent. If `data` is a string, all bytes from the string are sent. The return
  value is the acknowledgement sent by the receiver. When a multi-byte sequence
  (string) is sent, all bytes must be positively acknowledged by the receiver,
  except for the last one. Acknowledgement for the last byte becomes the return
  value. If one of the bytes in the middle was not acknowledged, `I2C.ERR` is
  returned.
- `i2c.readByte([ackType]) -> number`: Reads a byte from the slave and reads
  `ACK`/`NAK` as instructed. The argument is optional and defaults to `ACK`. It
  is possible to specify `NONE`, in which case the acknowledgment bit is not
  transmitted, and the call must be followed up by `sendAck`.
- `i2c.readString(n, [lastAckType]) -> string`: Reads a sequence of `n` bytes.
  n bytes except the last are `ACK`-ed, `lastAckType` specifies what to do
  with the last one and works like `ackType` does for `readByte`.
- `i2c.sendAck(ackType)`: Sends an acknowledgement. This method must be used
  after one of the `read` methods with `NONE` ack type.

High-level API:

- `i2c.read(addr, nbytes) -> string or I2C.ERR`: Issues a read request to the
  device with address `addr`, reading `nbytes` bytes. Acknowledges all incoming
  bytes except the last one.
- `i2c.write(addr, data) -> ackType`: Issues a write request to the device with
  address `addr`.  `data` is passed as is to `.send` method.
- `i2c.do(addr, req, ...) -> array`: Issues multiple requests to the same
  device, generating repeated start conditions between requests. Each request
  is an array with 2 or 3 elements: `[command, num_bytes, opt_param]`.
  `command` is `I2C.READ` or `I2C.WRITE` for read or write respectively,
  `num_bytes` is a number of bytes for read request or data to send for write
  request, `opt_param` is optional, has a different meaning for different types
  of requests: for read requests, it's `ackType` for the last read byte it
  (defaults to `I2C.NAK`), and for write requests, `ackType` to expect from the
  device after last sent byte (defaults to `I2C.ACK`). + Return value is an
  array that contains one element for each request on success (string data
  for reads, `ackType` for writes), or possibly less than that on error, in
  which case the last element will be `I2C.ERR`. Errors include:
    * Address wasn't ACK'ed (no such device on the bus).
    * Device sent NACK before all the bytes were written.
    * `ackType` for the last byte written doesn't match what was expected.

There is a detailed description of this API in
[miot_i2c_js.c](https://github.com/cesanta/mongoose-iot/blob/master/fw/src/miot_i2c_js.c).
See [temperature sensor
driver](https://github.com/cesanta/mongoose-iot/blob/master/fw/src/js/MCP9808.js)
and [EEPROM
driver](https://github.com/cesanta/mongoose-iot/blob/master/fw/src/js/MC24FC.js)
for usage example.
