// A Bosch BM222 accelerometer library API. Source C API is defined at:
// [mgos_bm222.h](https://github.com/cesanta/dev/blob/master/mos_libs/bm222/include/mgos_bm222.h)

let BM222 = {
  // ## **`BM222.init(bus, addr)`**
  // Initialize BM222 sensor on a given I2C `bus` and `addr`.
  // Returns true on success, false otherwise.
  init: ffi('bool bm222_init(void *, int)'),

  // ## **`BM222.read()`**
  // Read sensor values given I2C `bus` and `addr`.
  // In case of success, returns array of 3 integers: x, y, z.
  // Otherwise returns `null`.
  read: function(bus, addr) {
    let b1 = '      ', b2 = '      ', b3 = '      ';
    let res = this._read(bus, addr, b1, b2, b3);
    if (!res) return null;
    let f = function(s) { return s.at(0) | s.at(1) << 8; };
    return [ f(b1), f(b2), f(b3) ];
  },

  _read: ffi('bool bm222_read(void *, int, void *, void *, void *)'),
};
