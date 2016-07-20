---
title: UART
---

ESP8266 is equipped with two identical UART modules, `0` and `1` (but UART 1's
`RX` pin cannot be used due to conflicts with SPI flash, so it's effectively
transmit-only). The JavaScript API consists of a top-level `UART` function, which
takes one argument, a module number and returns an object that can be used to
manipulate it:

`var u = UART(0);`

This object has the following methods:

- `configure({configuration}) -> bool`:
  - `configuration`: Optional object with zero or more of the following fields:
    - `baud_rate`: Baud rate to use [default: `115200`].
    - `rx_buf_size`: Size of the receive buffer [`256`].
    - `rx_fc_ena`: Enable hardware receive flow control (RTS) [`false`].
    - `tx_buf_size`: Transmit buffer size [`256`].
    - `tx_fc_ena`: Enable hardware transmit full control (CTS) [`false`].
    - `swap_rxcts_txrts`: Swap `RX`/`CTS` and `TX`/`RTS` pins [`false`].

All fields are optional and defaults will be used for the missing ones.

Note: After configuration, UART receive interrupt is disabled and needs to be
either explicitly enabled via `setRXEnabled` or enabled implicitly by installing `onRecv` event
handler.

- `recv([max]) -> string`:
    - `max`: Optional maximum number of bytes to receive. If not specified,
      will return as much as possible from the receive buffer.

Note: a return value less than `max` does not necessarily mean no more data is
available. Only an empty return value is an indication of an empty buffer.

- `setRXEnabled(enabled)`:
    - `enabled`: If `false`, receiver will be throttled - no more receive
      events will be delivered and if the hardware receive flow control is on, the
      `RTS` pin will be deactivated.

- `sendAvail() -> number`: Returns the available space in the send buffer.

- `send(data) -> number`: Returns the number of bytes of input data consumed.
    - `data`: A string to be sent. Up to `sendAvail()` bytes will be put into the
      transmit buffer.

Event handlers:

- `onRecv(cb, [disable_rx])`: `cb` will be invoked when more data is available
  for reading. `cb` does not take any arguments and should consume data using
  `recv`. Moreover, no more callbacks will be delivered until `recv` is
  invoked.

Note: `onRecv` will enable receive interrupts. If that is not desirable, a
second argument with true value should be passed.

- `onTXEmpty(cb)`: `cb` will be invoked when the transmit buffer is empty. Note
  that this does not include UART FIFO and thus there may be up to 128 bytes
  still to be put on the wire. At present there is no way to query transmit
  FIFO length.


Example:

```javascript
// UART echo.
function doEcho(u) {
  var d, n;
  while (true) {
    n = u.sendAvail();
    if (n == 0) break;
    d = u.recv(n);
    if (!d) break;
    u.send(d);
  }
}

var u = UART(0);
u.configure({
  baud_rate: 115200,
  rx_buf_size: 1024,
  tx_buf_size: 1024,
});
u.onRecv(function() { doEcho(this); });
u.onTXEmpty(function() { doEcho(this); });
```
