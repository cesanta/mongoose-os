// Arduino OneWire library API. Source C API is defined at:
// [mgos_arduino_onewire.h](https://github.com/cesanta/mongoose-os/libs/arduino-onewire/blob/master/src/mgos_arduino_onewire.h)

let OneWire = {
  _init: ffi('void *mgos_arduino_onewire_create(int)'),
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

  // ## **`OneWire.create()`**
  // Create a OneWire object instance. Return value: an object with the methods
  // described below.
  create: function(pin) {
    let obj = Object.create(OneWire._proto);
    // Initialize OneWire library.
    // Return value: OneWire handle opaque pointer.
    obj.ow = OneWire._init(pin);
    return obj;
  },

  _proto: {
    // ## **`myOW.close()`**
    // Close OneWire handle. Return value: none.
    close: function() {
      return OneWire._cls(this.ow);
    },

    // ## **`myOW.reset()`**
    // Reset the 1-wire bus. Usually this is needed before communicating with
    // any device.
    reset: function() {
      return OneWire._rst(this.ow);
    },

    // ## **`myOW.select(addr)`**
    // Select a device based on its address `addr`, which is a 8-byte string.
    // After a reset, this is needed to choose which device you will use, and
    // then all communication will be with that device, until another reset.
    // Example:
    // ```javascript
    // myOW.select("\x28\xff\x2b\x45\x4c\x04\x00\x10");
    // ```
    select: function(addr) {
      return OneWire._sel(this.ow, addr);
    },

    // ## **`myOW.skip()`**
    // Skip the device selection. This only works if you have a single device,
    // but you can avoid searching and use this to immediately access your
    // device.
    skip: function() {
      return OneWire._sp(this.ow);
    },

    // ## **`myOW.write(v)`**
    // Write a byte to the onewire bus. Example:
    // ```javascript
    // // Write 0x12 to the onewire bus
    // myOW.write(0x12);
    // ```
    write: function(v) {
      return OneWire._w(this.ow, v);
    },

    // ## **`myOW.write_bytes(buf, len)`**
    // Write first `len` bytes of the string `buf`. Example:
    // ```javascript
    // // Write [0x55, 0x66, 0x77] to the onewire bus
    // myOW.write_bytes("\x55\x66\x77", 3);
    // ```
    write_bytes: function(buf, len) {
      return OneWire._wbs(this.ow, buf, len);
    },

    // ## **`myOW.read()`**
    // Read a byte from the onewire bus. Example:
    // ```javascript
    // let b = myOW.read();
    // print('read:', b);
    // ```
    read: function() {
      return OneWire._r(this.ow);
    },

    // ## **`myOW.read_bytes(buf, len)`**
    // Read multiple bytes from the onewire bus. The given buffer should
    // be large enough to fit the data; otherwise it will result in memory
    // corruption and thus undefined behavior.
    // Return value: none.
    // Example:
    // ```javascript
    // let buf = "          ";
    // myOW.read_bytes(buf, 10);
    // print('read:', buf);
    // ```
    read_bytes: function(buf, len) {
      return OneWire._rbs(this.ow, buf, len);
    },

    // ## **`myOW.write_bit(v)`**
    // Write a single bit to the onewire bus. Given `v` should be a number:
    // either 0 or 1.
    write_bit: function(v) {
      return OneWire._wb(this.ow, v);
    },

    // ## **`myOW.read_bit()`**
    // Read a single bit from the onewire bus. Returned value is either 0 or 1.
    read_bit: function() {
      return OneWire._rb(this.ow);
    },

    // ## **`myOW.depower()`**
    // Not implemented yet.
    depower: function() {
      return OneWire._dp(this.ow);
    },

    // ## **`myOW.search(addr, mode)`**
    // Search for the next device. The given `addr` should be an string
    // containing least 8 bytes (any bytes). If a device is found, `addr` is
    // filled with the device's address and 1 is returned. If no more
    // devices are found, 0 is returned.
    // `mode` is an integer: 0 means normal search, 1 means conditional search.
    // Example:
    // ```javascript
    // let addr = "        ";
    // let res = myOW.search(addr, 0);
    // if (res === 1) {
    //   print("Found:", addr);
    // } else {
    //   print("Not found");
    // }
    // ```
    search: function(addr, mode) {
      return OneWire._sch(this.ow, addr, mode);
    },

    // ## **`myOW.reset_search()`**
    // Reset a search. Next use of `myOW.search(....)` will begin at the first
    // device.
    reset_search: function() {
      return OneWire._rsch(this.ow);
    },

    // ## **`myOW.target_search(fc)`**
    // Setup the search to find the device type 'fc' (family code) on the next
    // call to `myOW.search()` if it is present.
    //
    // If no devices of the desired family are currently on the bus, then
    // device of some another type will be found by `search()`.
    target_search: function(fc) {
      return OneWire._tsch(this.ow, fc);
    },

    // ## **`myOW.crc8(buf, len)`**
    // Calculate crc8 for the first `len` bytes of a string `buf`.
    // Example:
    // ```javascript
    // // Calculate crc8 of the buffer
    // let s = "foobar";
    // let crc = myOW.crc8(s, 6);
    // print("crc:", crc);
    // ```
    crc8: function(buf, len) {
      return OneWire._crc8(this.ow, buf, len);
    },

    // ## **`myOW.delay(t)`**
    // Delay `t` milliseconds.
    delay: function(t) {
      return OneWire._dly(t);
    },
  },
};
