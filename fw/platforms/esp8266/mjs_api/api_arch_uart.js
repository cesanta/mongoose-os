UART._arch = {
  _usp: ffi('void esp_uart_config_set_params(void *, int, int, int, int, bool)'),

  // Set arch-dependent UART config
  scfg: function(uartNo, cfg, param) {
    if (param.esp8266 === undefined) return;

    let ep = param.esp8266;

    let ft = (ep.rxFullThresh !== undefined ? ep.rxFullThresh : -1);
    let fct = (ep.rxFcThresh !== undefined ? ep.rxFcThresh : -1);
    let alarm = (ep.rxAlarm !== undefined ? ep.rxAlarm : -1);
    let et = (ep.txEmptyThresh !== undefined ? ep.txEmptyThresh : -1);
    let swap = (ep.swapRxCtsTxRts !== undefined ? !!ep.swapRxCtsTxRts : false);

    this._usp(cfg, ft, fct, alarm, et, swap);
  },
};
