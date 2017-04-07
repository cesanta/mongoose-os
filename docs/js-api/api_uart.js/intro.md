---
title: "UART"
items:
---

UART API. Source C API is defined at:
[mgos_uart.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_uart.h)


**`UART.setConfig(uartNo, param)`** - set UART config. `param` is an
object with the following optional fields:

- `baudRate`: baud rate, integer, default: 115200;
- `rxBufSize`: size of the Rx buffer, integer, default: 256;
- `rxFlowControl`: whether Rx flow control (RTS pin) is enabled, boolean,
   default: false;
- `rxLingerMicros`: how many microseconds to linger after Rx fifo
  is empty, in case more data arrives. Integer, default: 15;
- `txBufSize`: size of the Tx buffer, integer, default: 256;
- `txFlowControl`: whether Tx flow control (CTS pin) is enabled, boolean,
  default: false;

TODO: implement platform-specific settings, such as pins.



**`UART.setDispatcher(uartNo, callback, userdata)`** - set UART dispatcher
callback which gets invoked when there is a new data in the input buffer
or when the space becomes available on the output buffer.

Callback receives the following arguments: `(uartNo, userdata)`.



**`UART.write(uartNo, data)`** - write data to the buffer. Returns number
of bytes written.

Example usage: `UART.write(1, "foobar")`, in this case, 6 bytes will be written.



**`UART.writeAvail(uartNo)`** - returns amount of space available in the
output buffer.



**`UART.read(uartNo)`**. It never blocks, and returns a string containing
read data (which will be empty if there's no data available).



**`UART.readAvail(uartNo)`** - returns amount of data available in the
input buffer.



**`UART.setRxEnabled(uartNo)`** - sets whether Rx is enabled.



**`UART.isRxEnabled(uartNo)`** - returns whether Rx is enabled.



**`UART.flush(uartNo)`** - flush the UART output buffer, wait for the
data to be sent.

