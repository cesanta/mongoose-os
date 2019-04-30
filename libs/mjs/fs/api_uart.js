// UART API. Source C API is defined at:
// [mgos_uart.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_uart.h)
let UART = {
  _free: ffi('void free(void *)'),
  _cdef: ffi('void *mgos_uart_config_get_default(int)'),
  _cbp: ffi('void mgos_uart_config_set_basic_params(void *, int, int, int, int)'),
  _crx: ffi('void mgos_uart_config_set_rx_params(void *, int, int, int)'),
  _ctx: ffi('void mgos_uart_config_set_tx_params(void *, int, int)'),
  _cfg: ffi('int mgos_uart_configure(int, void *)'),
  _wr: ffi('int mgos_uart_write(int, char *, int)'),
  _rd: ffi('int mgos_uart_read(int, void *, int)'),

  // ## **`UART.setConfig(uartNo, param)`**
  // Set UART config. `param` is an
  // object with the following optional fields:
  //
  // - `baudRate`: baud rate, integer, default: 115200;
  // - `numDataBits`: Number of data bits, default: 8;
  // - `parity`: Parity: 0 - none, 1 - even, 2 - odd; default: none;
  // - `numStopBits`: Number of stop bits: 1 - 1 bit, 2 - 2 bits, 3 - 1.5; default: 1;
  // - `rxBufSize`: size of the Rx buffer, integer, default: 256;
  // - `rxFlowControl`: whether Rx flow control (RTS pin) is enabled, boolean,
  //    default: false;
  // - `rxLingerMicros`: how many microseconds to linger after Rx fifo
  //   is empty, in case more data arrives. Integer, default: 15;
  // - `txBufSize`: size of the Tx buffer, integer, default: 256;
  // - `txFlowControl`: whether Tx flow control (CTS pin) is enabled, boolean,
  //   default: false;
  //
  // Other than that, there are architecture-dependent settings, grouped in
  // the objects named with the architecture name: "esp32", "esp8266", etc.
  //
  // Settings for esp32:
  //
  // ```
  //   esp32: {
  //      /*
  //       * GPIO pin numbers, default values depend on UART.
  //       *
  //       * UART 0: Rx: 3, Tx: 1, CTS: 19, RTS: 22
  //       * UART 1: Rx: 13, Tx: 14, CTS: 15, RTS: 16
  //       * UART 2: Rx: 17, Tx: 25, CTS: 26, RTS: 27
  //       */
  //      gpio: {
  //        rx: number,
  //        tx: number,
  //        cts: number,
  //        rts: number,
  //      },
  //
  //      /* Hardware FIFO tweaks */
  //      fifo: {
  //        /*
  //         * A number of bytes in the hardware Rx fifo, should be between 1 and 127.
  //         * How full hardware Rx fifo should be before "rx fifo full" interrupt is
  //         * fired.
  //         */
  //        rxFullThresh: number,
  //
  //        /*
  //         * A number of bytes in the hardware Rx fifo, should be more than
  //         * rx_fifo_full_thresh.
  //         *
  //         * How full hardware Rx fifo should be before CTS is deasserted, telling
  //         * the other side to stop sending data.
  //         */
  //        rxFcThresh: number,
  //
  //        /*
  //         * Time in uart bit intervals when "rx fifo full" interrupt fires even if
  //         * it's not full enough
  //         */
  //        rxAlarm: number,
  //
  //        /*
  //         * A number of bytes in the hardware Tx fifo, should be between 1 and 127.
  //         * When the number of bytes in Tx buffer becomes less than
  //         * tx_fifo_empty_thresh, "tx fifo empty" interrupt fires.
  //         */
  //        txEmptyThresh: number,
  //      },
  //    }
  // ```
  setConfig: function(uartNo, param) {
    let cfg = this._cdef(uartNo);

    this._cbp(cfg, param.baudRate || 115200,
                   param.numDataBits || 8,
                   param.parity || 0,
                   param.numStopBits || 1);

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

    // Apply arch-specific config
    if (this._arch !== undefined) {
      this._arch.scfg(uartNo, cfg, param);
    }

    let res = this._cfg(uartNo, cfg);

    this._free(cfg);
    cfg = null;

    return res;
  },

  // ## **`UART.setDispatcher(uartNo, callback, userdata)`**
  // Set UART dispatcher
  // callback which gets invoked when there is a new data in the input buffer
  // or when the space becomes available on the output buffer.
  //
  // Callback receives the following arguments: `(uartNo, userdata)`.
  setDispatcher: ffi('void mgos_uart_set_dispatcher(int, void(*)(int, userdata), userdata)'),

  // ## **`UART.write(uartNo, data)`**
  // Write data to the buffer. Returns number of bytes written.
  //
  // Example usage: `UART.write(1, "foobar")`, in this case, 6 bytes will be written.
  write: function(uartNo, data) {
    this._wr(uartNo, data, data.length);
  },

  // ## **`UART.writeAvail(uartNo)`**
  // Return amount of space available in the output buffer.
  writeAvail: ffi('int mgos_uart_write_avail(int)'),

  // ## **`UART.read(uartNo)`**
  // It never blocks, and returns a string containing
  // read data (which will be empty if there's no data available).
  read: function(uartNo) {
    let n = 0; let res = ''; let buf = 'xxxxxxxxxx'; // Should be > 5
    while ((n = this._rd(uartNo, buf, buf.length)) > 0) {
      res += buf.slice(0, n);
    }
    return res;
  },
  // ## **`UART.readAvail(uartNo)`**
  // Return amount of data available in the input buffer.
  readAvail: ffi('int mgos_uart_read_avail(int)'),

  // ## **`UART.setRxEnabled(uartNo)`**
  // Set whether Rx is enabled.
  setRxEnabled: ffi('void mgos_uart_set_rx_enabled(int, int)'),
  // ## **`UART.isRxEnabled(uartNo)`**
  // Returns whether Rx is enabled.
  isRxEnabled: ffi('int mgos_uart_is_rx_enabled(int)'),

  // ## **`UART.flush(uartNo)`**
  // Flush the UART output buffer, wait for the data to be sent.
  flush: ffi('void mgos_uart_flush(int)'),
};

// Load arch-specific API
load('api_arch_uart.js');
