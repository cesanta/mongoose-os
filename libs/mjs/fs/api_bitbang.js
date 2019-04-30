let BitBang = {
  DELAY_MSEC: 0,
  DELAY_USEC: 1,
  DELAY_100NSEC: 2,

  // ## **`BitBang.write(pin, delay_unit, t0h, t0l, t1h, t1l, ptr, len)`**
  // Write bits to a given `pin`. `delay_unit` is one of the:
  // `BitBang.DELAY_MSEC`, `BitBang.DELAY_USEC`, `BitBang.DELAY_100NSEC`.
  // `ptr, len` is a bit pattern to write. `t0h, t0l` is the time pattern
  // for zero bit, `t1h, t1l` is the time pattern for 1. The time pattern
  // specifies the number of time units to hold the pin high, and the number
  // of units to hold the pin low. Return value: none.
  write: function(pin, delay_unit, t0h, t0l, t1h, t1l, ptr, len) {
    let t = (t0h << 24) | (t0l << 16) | (t1h << 8) | t1l;
    this._wb(pin, delay_unit, t, ptr, len);
  },

  _wb: ffi('void mgos_bitbang_write_bits_js(int, int, int, void *, int)'),
};
