// Arduino Adafruit DHT library API. Source C API is defined at:
// [mgos_arduino_dht.h](https://github.com/cesanta/mongoose-os/blob/master/arduino_drivers/dht/mgos_arduino_dht.h)

load("api_math.js");

let DHT = {
  _create: ffi('void *mgos_dht_create(int, int)'),
  _cls: ffi('void mgos_dht_close(void *)'),
  _bgn: ffi('void mgos_dht_begin(void *)'),
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

  _proto: {
    // Close DHT handle. Return value: none.
    close: function() {
      return DHT._cls(this.dht);
    },

    // Initialize a device.
    begin: function() {
      return DHT._bgn(this.dht);
    },

    // Returns temperature in DegC or DegF depending on param #2
    // or -127.0 if operation failed
    // Param #2: 0 - Celcius; 1 - Fahrenheit).
    // Param #3: 0 - normal read mode, 1 - forced read.
    readTemperature: function(s, f) {
      // C-functions output value of “1234” equals 12.34 Deg.
      return DHT._rt(this.dht, s, f) / 100.0;
    },

    // Convert DegC to DegF or -127.0 if operation failed.
    convertCtoF: function(tc) {
      // C-functions input and output values of “1234” equals 12.34 Deg.
      return DHT._cctof(this.dht, Math.round(tc * 100.0)) / 100.0;
    },

    // Convert DegF to DegC or -127.0 if operation failed.
    convertFtoC: function(tf) {
      // C-functions input and output values of “1234” equals 12.34 Deg.
      return DHT._cftoc(this.dht, Math.round(tf * 100.0)) / 100.0;
    },

    // Compute a heat index or -127.0 if operation failed.
    // Please see Arduino Adafrut DHTxx library for more details.
    computeHeatIndex: function(t, h, f) {
      // C-functions input and output values of “1234” equals 12.34 Deg.
      return DHT._chi(this.dht, Math.round(t * 100), Math.round(h * 100), f) / 100.0;
    },
    
    // Returns temperature in RH% or -127.0 if operation failed.
    // Param #2: 0 - normal read mode, 1 - forced read.
    readHumidity: function(f) {
      // c-functions output value of “4321” equals 43.21 %.
      return DHT._rh(this.dht, f) / 100.0;
    },
  },

  // Create a DHT object.
  create: function(pin, type) {
    let obj = Object.create(DHT._proto);
    // Initialize DHT library.
    // Return value: DHT handle opaque pointer.
    obj.dht = DHT._create(pin, type);
    return obj;
  },
};
