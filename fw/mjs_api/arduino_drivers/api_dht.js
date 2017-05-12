// Arduino Adafruit DHT library API. Source C API is defined at:
// [mgos_dht.h](https://github.com/cesanta/mongoose-os/blob/master/arduino_drivers/dht/mgos_dht.h)

load("api_math.js");

let DHT = {
  _rt: ffi('int mgos_dht_read_temperature(void *, int, int)'),
  _cctof: ffi('int mgos_dht_convert_ctof(void *, int)'),
  _cftoc: ffi('int mgos_dht_convert_ftocC(void *, int)'),
  _chi: ffi('int mgos_dht_compute_heat_index(void *, int, int, int)'),
  _rh: ffi('int mgos_dht_read_humidity(void *, int)'),

  // Define type of sensors.
  DHT11: 11,
  DHT21: 21,
  AM2301: 21,
  DHT22: 22,
  AM2302: 22,

  // Initialize DHT library. Return value: DHT handle opaque pointer.
  init: ffi('void *mgos_dht_init(int, int)'),

  // Close DHT handle. Return value: none.
  close: ffi('void mgos_dht_close(void *)'),

  // Initialize a device.
  begin: ffi('void mgos_dht_begin(void *)'),

  // Returns temperature in DegC or DegF depending on param #2
  // or -127.0 if operation failed
  // Param #2: 0 - Celcius; 1 - Fahrenheit).
  // Param #3: 0 - normal read mode, 1 - forced read.
  readTemperature: function(dht, s, f) {
    // C-functions output value of “1234” equals 12.34 Deg.
    return this._rt(dht, s, f) / 100.0;
  },

  // Convert DegC to DegF or -127.0 if operation failed.
  convertCtoF: function(dht, tc) {
    // C-functions input and output values of “1234” equals 12.34 Deg.
    return this._cctof(dht, Math.round(tc * 100.0)) / 100.0;
  },

  // Convert DegF to DegC or -127.0 if operation failed.
  convertFtoC: function(dht, tf) {
    // C-functions input and output values of “1234” equals 12.34 Deg.
    return this._cftoc(dht, Math.round(tf * 100.0)) / 100.0;
  },

  // Compute a heat index or -127.0 if operation failed.
  // Please see Arduino Adafrut DHTxx library for more details.
  computeHeatIndex: function(dht, t, h, f) {
    // C-functions input and output values of “1234” equals 12.34 Deg.
    return this._chi(dht, Math.round(t * 100), Math.round(h * 100), f) / 100.0;
  },
  
  // Returns temperature in RH% or -127.0 if operation failed.
  // Param #2: 0 - normal read mode, 1 - forced read.
  readHumidity: function(dht, f) {
    // c-functions output value of “4321” equals 43.21 %.
    return this._rh(dht, f) / 100.0;
  },
};
