// Arduino Adafruit_BME280 library API. Source C API is defined at:
// [mgos_arduino_bme280.h](https://github.com/cesanta/mongoose-os/blob/master/arduino_drivers/Arduino/mgos_arduino_bme280.h)

load('api_math.js');

let Adafruit_BME280 = {
  // Error codes
  RES_FAIL: -100.0,

  _create: ffi('void *mgos_bme280_create(void)'),
  _close: ffi('void mgos_bme280_close(void *)'),
  _begin: ffi('int mgos_bme280_begin(void *, int)'),
  _tfm: ffi('void mgos_bme280_take_forced_measurement(void *)'),
  _rt: ffi('int mgos_bme280_read_temperature(void *)'),
  _rp: ffi('int mgos_bme280_read_pressure(void *)'),
  _rh: ffi('int mgos_bme280_read_humidity(void *)'),
  _ra: ffi('int mgos_bme280_read_altitude(void *, int)'),
  _alfa: ffi('int mgos_bme280_sea_level_for_altitude(void *, int, int)'),

  _proto: {
    // Close Adafruit_BME280 handle. Return value: none.
    close: function() {
      return Adafruit_BME280._close(this.bme);
    },

    // Initialize the sensor with given parameters/settings.
    // Returns 0 if the sensor not plugged or a checking failed,
    // i.e. the chip ID is incorrect.
    // Returns 1 otherwise.
    begin: function(addr) {
      return Adafruit_BME280._begin(this.bme, addr);
    },

    // Take a new measurement (only possible in forced mode).
    takeForcedMeasurement: function() {
      return Adafruit_BME280._tfm(this.bme);
    },

    // Returns the temperature from the sensor in degrees C
    // or RES_FAIL if an operation failed.
    readTemperature: function() {
      // C-functions output value of “1234” equals 12.34 Deg.
      return Adafruit_BME280._rt(this.bme) / 100.0;
    },

    // Returns the pressure from the sensor in hPa
    // or RES_FAIL if an operation failed.
    readPressure: function() {
      // C-functions output value of “1234” equals 12.34 hPa.
      return Adafruit_BME280._rp(this.bme) / 10000.0;
    },

    // Returns the humidity from the sensor in %RH
    // or RES_FAIL if an operation failed.
    readHumidity: function() {
      // C-functions output value of “1234” equals 12.34 %RH.
      return Adafruit_BME280._rh(this.bme) / 100.0;
    },

    // Returns the altitude in meters calculated from the specified
    // atmospheric pressure (in hPa), and sea-level pressure (in hPa)
    // or RES_FAIL if an operation failed.
    // http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf, P.16
    readAltitude: function(lvl) {
      // C-functions input and output values of “1234” equals 12.34.
      return Adafruit_BME280._ra(this.bme, Math.round(lvl * 100.0)) / 100.0;
    },

    // Returns the pressure at sea level in hPa
    // calculated from the specified altitude (in meters),
    // and atmospheric pressure (in hPa)
    // or RES_FAIL if an operation failed.
    // http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf, P.17
    seaLevelForAltitude: function(alt, pres) {
      // C-functions input and output values of “1234” equals 12.34.
      return Adafruit_BME280._alfa(this.bme, Math.round(alt * 100.0), Math.round(pres * 100.0)) / 100.0;
    },
  },

  create: function() {
    let obj = Object.create(Adafruit_BME280._proto);
    // Initialize Adafruit_BME280 library.
    // Return value: handle opaque pointer.
    obj.bme = Adafruit_BME280._create();
    return obj;
  },
};
