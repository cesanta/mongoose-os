// esp32 architecture-dependent UART wrappers
UART._arch = {
  _pins: ffi('void esp32_uart_config_set_pins(int, void *, int, int, int, int)'),
  _fifo: ffi('void esp32_uart_config_set_fifo(int, void *, int, int, int, int)'),

  // Set arch-dependent UART config
  scfg: function(uartNo, cfg, param) {
    if (param.esp32 === undefined) return;

    // Set GPIO params
    if (param.esp32.gpio !== undefined) {
      let dgpio = param.esp32.gpio;

      let rx = (dgpio.rx !== undefined ? dgpio.rx : -1);
      let tx = (dgpio.tx !== undefined ? dgpio.tx : -1);
      let cts = (dgpio.cts !== undefined ? dgpio.cts : -1);
      let rts = (dgpio.rts !== undefined ? dgpio.rts : -1);

      this._pins(uartNo, cfg, rx, tx, cts, rts);
    }

    // Set FIFO params
    if (param.esp32.fifo !== undefined) {
      let dfifo = param.esp32.fifo;

      let ft = (dfifo.rxFullThresh !== undefined ? dfifo.rxFullThresh : -1);
      let fct = (dfifo.rxFcThresh !== undefined ? dfifo.rxFcThresh : -1);
      let alarm = (dfifo.rxAlarm !== undefined ? dfifo.rxAlarm : -1);
      let et = (dfifo.txEmptyThresh !== undefined ? dfifo.txEmptyThresh : -1);

      this._fifo(uartNo, cfg, ft, fct, alarm, et);
    }
  },
};
