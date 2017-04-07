// UART API. Source C API is defined at:
// [mgos_uart.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_uart.h)
let UART = {
  _free: ffi('void free(void *)'),
  _cdef: ffi('void *mgos_uart_config_get_default(int)'),
  _cbr: ffi('void mgos_uart_config_set_baud_rate(void *, int)'),
  _crx: ffi('void mgos_uart_config_set_rx_params(void *, int, int, int)'),
  _ctx: ffi('void mgos_uart_config_set_tx_params(void *, int, int)'),
  _cfg: ffi('int mgos_uart_configure(int, void *)'),
  _wr: ffi('int mgos_uart_write(int, char *, int)'),
  _rd: ffi('int mgos_uart_read(int, void *, int)'),

  // **`UART.setConfig(uartNo, param)`** - set UART config. `param` is an
  // object with the following optional fields:
  //
  // - `baudRate`: baud rate, integer, default: 115200;
  // - `rxBufSize`: size of the Rx buffer, integer, default: 256;
  // - `rxFlowControl`: whether Rx flow control (RTS pin) is enabled, boolean,
  //    default: false;
  // - `rxLingerMicros`: how many microseconds to linger after Rx fifo
  //   is empty, in case more data arrives. Integer, default: 15;
  // - `txBufSize`: size of the Tx buffer, integer, default: 256;
  // - `txFlowControl`: whether Tx flow control (CTS pin) is enabled, boolean,
  //   default: false;
  //
  // TODO: implement platform-specific settings, such as pins.
  setConfig: function(uartNo, param) {
    let cfg = this._cdef(uartNo);

    this._cbr(cfg, param.baudRate || 115200);

    this._crx(
      cfg,
      param.rxBufSize || 256,
      param.rxFlowControl || false,
      param.rxLingerMicros || 15
    );

    this._ctx(
      cfg,
      param.txBufSize || 256,
      param.txFlowControl || false
    );

    let res = this._cfg(uartNo, cfg);

    this._free(cfg);
    cfg = null;

    return res;
  },

  // **`UART.setDispatcher(uartNo, callback, userdata)`** - set UART dispatcher
  // callback which gets invoked when there is a new data in the input buffer
  // or when the space becomes available on the output buffer.
  //
  // Callback receives the following arguments: `(uartNo, userdata)`.
  setDispatcher: ffi('void mgos_uart_set_dispatcher(int, void(*)(int, userdata), userdata)'),

  // **`UART.write(uartNo, data)`** - write data to the buffer. Returns number
  // of bytes written.
  //
  // Example usage: `UART.write(1, "foobar")`, in this case, 6 bytes will be written.
  write: function(uartNo, data) {
    this._wr(uartNo, data, data.length);
  },

  // **`UART.writeAvail(uartNo)`** - returns amount of space available in the
  // output buffer.
  writeAvail: ffi('int mgos_uart_write_avail(int)'),

  // **`UART.read(uartNo)`**. It never blocks, and returns a string containing
  // read data (which will be empty if there's no data available).
  read: function(uartNo) {
    let n = 0; let res = ''; let buf = 'xxxxxxxxxx'; // Should be > 5
    while ((n = this._rd(uartNo, buf, buf.length)) > 0) {
      res += buf.slice(0, n);
    }
    return res;
  },
  // **`UART.readAvail(uartNo)`** - returns amount of data available in the
  // input buffer.
  readAvail: ffi('int mgos_uart_read_avail(int)'),

  // **`UART.setRxEnabled(uartNo)`** - sets whether Rx is enabled.
  setRxEnabled: ffi('void mgos_uart_set_rx_enabled(int, int)'),
  // **`UART.isRxEnabled(uartNo)`** - returns whether Rx is enabled.
  isRxEnabled: ffi('int mgos_uart_is_rx_enabled(int)'),

  // **`UART.flush(uartNo)`** - flush the UART output buffer, wait for the
  // data to be sent.
  flush: ffi('void mgos_uart_flush(int)'),
};

