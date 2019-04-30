// Mongoose OS DHT library API. Source C API is defined at:
// [mgos_dht.h](https://github.com/cesanta/mongoose-os/libs/dht/tree/master/src/mgos_dht.h)

let DHT = {
  _crt: ffi('void *mgos_dht_create(int, int)'),
  _cls: ffi('void mgos_dht_close(void *)'),
  _gt: ffi('float mgos_dht_get_temp(void *)'),
  _gh: ffi('float mgos_dht_get_humidity(void *)'),

  // Define type of sensors.
  DHT11: 11,
  DHT21: 21,
  AM2301: 21,
  DHT22: 22,
  AM2302: 22,

  // **`DHT.create(pin, type)`**
  // Create a DHT object. `type` could be `DHT.DHT11`, `DHT.DHT21`,
  // `DHT.DHT22`. Return value: an object with the methods described below, or
  // 'null' in case of a failure.
  // Example:
  // ```javascript
  // let mydht = DHT.create(5, DHT.DHT11);
  // print('Temperature:', mydht.getTemp());
  // ```
  create: function(pin, type) {
    let obj = Object.create(DHT._proto);
    obj.dht = DHT._crt(pin, type);
    return obj;
  },

  _proto: {
    // **`mydht.close()`**
    // Close DHT handle. Return value: none.
    close: function() {
      return DHT._cls(this.dht);
    },

    // **`mydht.getTemp()`**
    // Return temperature in degrees C or 'NaN' in case of a failure.
    getTemp: function() {
      return DHT._gt(this.dht);
    },

    // **`mydht.getHumidity()`**
    // Return humidity in RH% or 'NaN' in case of a failure.
    getHumidity: function() {
      return DHT._gh(this.dht);
    },
  },
};
