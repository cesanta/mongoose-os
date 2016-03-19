---
title: SPI
---

- `var spi = new SPI()`: SPI constructor
- `spi.tran(dataToSend, [bytesToRead, command, address]) -> number`: Send and
  receive data within one transaction. `dataToSend` is a 32bit number to send
  to SPI. `bytesToRead` is a number of bytes to read from SPI (1-4). If device
  requires explicit command and address, they might be provided via `command`
  and `address` parameters.
- `spi.txn(commandLenBits, command, addrLenBits, address, dataToSendLenBits,
  dataToWrite, dataToReadLenBits, dummyBits) -> number`: Send and receive data
  within one transaction. The same as `spi.tran`, but allows to use arbitrary
  (1-32 bits) lengths. This function should be used if device requires, for
  example, 9bit data, 7bit address, 3bit command etc.

There is a detailed description in
[`sj_spi_js.c`](https://github.com/cesanta/smart.js/blob/master/src/sj_spi_js.c).

See [barometer
driver](https://github.com/cesanta/smart.js/blob/master/platforms/esp8266/fs/MPL115A1.js)
for the usage example.

