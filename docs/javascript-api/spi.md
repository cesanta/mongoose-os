---
title: SPI
---

- `var spi = new SPI()`: SPI constructor
- `spi.tran(dataToSend, [bytesToRead, command, address]) -> number`: Sends and
  receives data within one transaction. `dataToSend` is a 32bit number to send
  to SPI. `bytesToRead` is a number of bytes to read from SPI (1-4). If the device
  requires an explicit command and address, they might be provided via `command`
  and `address` parameters.
- `spi.txn(commandLenBits, command, addrLenBits, address, dataToSendLenBits,
  dataToWrite, dataToReadLenBits, dummyBits) -> number`: Sends and receives data
  within one transaction. The same as `spi.tran`. But, it allows you to use arbitrary
  (1-32 bits) lengths. This function should be used if the device requires, for
  example, 9bit data, 7bit address, 3bit command etc.

There is a detailed description in
[`miot_spi_js.c`](https://github.com/cesanta/mongoose-iot/blob/master/fw/src/miot_spi_js.c).

See [barometer
driver](https://github.com/cesanta/mongoose-iot/blob/master/fw/platforms/esp8266/fs/MPL115A1.js)
for the usage example.

