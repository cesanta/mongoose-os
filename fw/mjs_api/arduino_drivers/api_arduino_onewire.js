// Arduino OneWire library API. Source C API is defined at:
// [mgos_arduino_onewire.h](https://github.com/cesanta/mongoose-os/blob/master/arduino_drivers/Arduino/mgos_arduino_onewire.h)

let OneWire = {
  _init: ffi('void *mgos_arduino_onewire_init(int)'),
  _cls: ffi('void mgos_arduino_onewire_close(void *)'),
  _rst: ffi('int mgos_arduino_onewire_reset(void *)'),
  _sel: ffi('void mgos_arduino_onewire_select(void *, char *)'),
  _sp: ffi('void mgos_arduino_onewire_skip(void *)'),
  _w: ffi('void mgos_arduino_onewire_write(void *, int)'),
  _wbs: ffi('void mgos_arduino_onewire_write_bytes(void *, char *, int)'),
  _r: ffi('int mgos_arduino_onewire_read(void *)'),
  _rbs: ffi('void mgos_arduino_onewire_read_bytes(void *, char *, int)'),
  _wb: ffi('void mgos_arduino_onewire_write_bit(void *, int)'),
  _rb: ffi('int mgos_arduino_onewire_read_bit(void *)'),
  _dp: ffi('void mgos_arduino_onewire_depower(void *)'),
  _rsch: ffi('void mgos_arduino_onewire_reset_search(void *)'),
  _tsch: ffi('void mgos_arduino_onewire_target_search(void *, int)'),
  _sch: ffi('int mgos_arduino_onewire_search(void *, char *, int)'),
  _crc8: ffi('int mgos_arduino_onewire_crc8(void *, char *, int)'),
  _dly: ffi('void mgos_msleep(int)'),

  _proto: {
    // Close OneWire handle. Return value: none.
    close: function() {
      return OneWire._cls(this.ow);
    },

    reset: function() {
      return OneWire._rst(this.ow);
    },

    select: function(addr) {
      return OneWire._sel(this.ow, addr);
    },

    skip: function() {
      return OneWire._sp(this.ow);
    },

    write: function(v) {
      return OneWire._w(this.ow, v);
    },

    write_bytes: function(buf, len) {
      return OneWire._wbs(this.ow, buf, len);
    },

    read: function() {
      return OneWire._r(this.ow);
    },

    read_bytes: function(buf, len) {
      return OneWire._rbs(this.ow, buf, len);
    },

    write_bit: function(v) {
      return OneWire._wb(this.ow, v);
    },

    read_bit: function() {
      return OneWire._rb(this.ow);
    },

    depower: function() {
      return OneWire._dp(this.ow);
    },

    reset_search: function() {
      return OneWire._rsch(this.ow);
    },

    target_search: function(fc) {
      return OneWire._tsch(this.ow, fc);
    },

    search: function(addr, mde) {
      return OneWire._sch(this.ow, addr, mde);
    },

    crc8: function(addr, len) {
      return OneWire._crc8(this.ow, addr, len);
    },

    delay: function(t) {
      return OneWire._dly(t);
    },
  },

  // Create a OneWire object.
  create: function(pin) {
    let obj = Object.create(OneWire._proto);
    // Initialize OneWire library.
    // Return value: OneWire handle opaque pointer.
    obj.ow = OneWire._init(pin);
    return obj;
  },
};
