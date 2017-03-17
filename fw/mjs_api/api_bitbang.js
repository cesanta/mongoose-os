let BitBang = {
  DELAY_MSEC: 0,
  DELAY_USEC: 1,
  DELAY_100NSEC: 2,

  writeBits: function(pin, delay_unit, t0h, t0l, t1h, t1l, ptr, len) {
    let t = (t0h << 24) | (t0l << 16) | (t1h << 8) | t1l;
    BitBang._wb(pin, delay_unit, t, ptr, len);
  },

  _wb: ffi('void mgos_bitbang_write_bits_js(int, int, int, void *, int)'),
};
