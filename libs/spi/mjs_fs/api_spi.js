load('api_sys.js')

let SPI = {
  _run: ffi('bool mgos_spi_run_txn(void *, bool, void *)'),
  _ctxn: ffi('void *mgos_spi_create_txn(int, int, int)'),
  _shd: ffi('void mgos_spi_set_hd_txn(void *, int, void *, int, int, void *)'),
  _sfd: ffi('void mgos_spi_set_fd_txn(void *, int, void *, void *)'),

  // ## **`SPI.get()`**
  // Get SPI bus handle. Return value: opaque pointer.
  get: ffi('void *mgos_spi_get_global(void)'),

  // ## **`SPI.runTransaction(spi, param)`**
  // Run SPI transaction, which might be a half-duplex or a full-duplex one.
  //
  // Half-duplex transaction includes one or more of the following:
  // - Writing data,
  // - Waiting for dummy bytes,
  // - Reading data.
  //
  // Full-duplex transaction performs writing and reading at the same time.
  //
  // Whether the transaction is half-duplex or full-duplex is determined by
  // given params: if "fd" property is set, it's a full-duplex transaction;
  // otherwise "hd" property should be set (see details below).
  //
  // `spi` is an SPI instance, e.g. the one returned by `SPI.get()`.
  // `param` is an object with the following parameters:
  //
  // ```javascript
  // {
  //   // Which CS line to use, 0, 1 or 2. use -1 to not assert any CS
  //   // during transaction, it is assumed to be done externally.
  //   // Note: this is not a GPIO number, mapping from cs to GPIO is set in
  //   // the device configuration.
  //   cs: 0,
  //
  //   // Mode, 0-3. This controls clock phase and polarity.
  //   mode: 0,
  //
  //   // Clock frequency to use. 0 means don't change.
  //   freq: 2000000,
  //
  //   // Half-duplex transaction parameters
  //   hd: {
  //     // A string with data to transmit. If undefined, no data is transmitted.
  //     tx_data: "foobar",
  //
  //     // Number of dummy bytes to wait for. If undefined, 0 is assumed.
  //     dummy_len: 1,
  //
  //     // Number of bytes to read.
  //     rx_len: 3,
  //   },
  //
  //   // Full-duplex transaction parameters
  //   fd: {
  //     // A string with data to transmit. Equal number of bytes will be read.
  //     tx_data: "foo",
  //   },
  // }
  // ```
  //
  // Return value: a string with the data read (an empty string if no read was
  // requested), or `false` in case of an error.
  runTransaction: function(spi, param) {
    // If spi instance was not given, assume global
    spi = spi || this.get();

    if (spi === null) {
      print("SPI is disabled");
      return false;
    }

    if (!param.hd && !param.fd) {
      print("Neither hd nor fd is given");
      return false;
    }

    // Prepare transaction with the given params
    let txn = this._ctxn(
      param.cs !== undefined ? param.cs : -1,
      param.mode || 0,
      param.freq || 0
    );

    let is_fd = false;
    let tx_len = 0, tx_data = null;
    let dummy_len = 0;
    let rx_len = 0, rx_buf = null;
    if (param.fd) {
      /* Full-duplex */
      let fdp = param.fd;
      is_fd = true;
      if (!fdp.tx_data) return false;
      tx_data = fdp.tx_data;
      rx_len = tx_data.length;
      if (rx_len === 0) return false;
      /* This string must survive between calls to _sfd and _run,
       * force it to be un-inlined. Bleh. */
      if (tx_data.length < 6) tx_data += "      ";
      rx_buf = Sys._sbuf(rx_len);
    } else {
      /* Half-duplex */
      /* the presence of param.hd is checked above */
      let hdp = param.hd;
      if (hdp.tx_data) {
        tx_data = hdp.tx_data;
        tx_len = tx_data.length;
        if (tx_data.length < 6) {
          tx_data += "      ";  // See above.
        }
      }
      dummy_len = hdp.dummy_len || 0;
      rx_len = hdp.rx_len || 0;
      if (rx_len > 0) {
        rx_buf = Sys._sbuf(rx_len);
      }
    }

    if (is_fd) {
      this._sfd(txn, rx_len, tx_data, rx_buf);
    } else {
      this._shd(txn, tx_len, tx_data, dummy_len, rx_len, rx_buf);
    }

    // Run that transaction
    let res = this._run(spi, is_fd, txn);

    // Free transaction struct
    Sys.free(txn);

    if (!res) {
      return false;
    } else if (rx_buf) {
      return rx_buf.slice(0, rx_len);
    } else {
      return true;
    }
  },
};
