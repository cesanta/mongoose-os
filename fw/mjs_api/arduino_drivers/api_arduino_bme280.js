// Arduino Adafruit_BME280 library API. Source C API is defined at:
// [mgos_arduino_bme280.h](https://github.com/cesanta/mongoose-os/blob/master/arduino_drivers/Arduino/mgos_arduino_bme280.h)

load('api_math.js');

let Adafruit_BME280 = {
  // Error codes
  RES_FAIL: -100.0,

  _rt: ffi('int mgos_bme280_read_temperature(void *)'),
  _rp: ffi('int mgos_bme280_read_pressure(void *)'),
  _rh: ffi('int mgos_bme280_read_humidity(void *)'),
  _ra: ffi('int mgos_bme280_read_altitude(void *, int)'),
  _alfa: ffi('int mgos_bme280_sea_level_for_altitude(void *, int, int)'),

  // Initialize Adafruit_BME280 library.
  // Return value: OneWire handle opaque pointer.
  init: ffi('void *mgos_bme280_init(void)'),

  // Close Adafruit_BME280 handle. Return value: none.
  close: ffi('void mgos_bme280_close(void *)'),

  // Initialize the sensor with given parameters/settings.
  // Returns 0 if the sensor not plugged or a checking failed,
  // i.e. the chip ID is incorrect.
  // Returns 1 otherwise.
  begin: ffi('int mgos_bme280_begin(void *, int)'),

  // Take a new measurement (only possible in forced mode).
  takeForcedMeasurement: ffi('void mgos_bme280_take_forced_measurement(void *)'),

  // Returns the temperature from the sensor in degrees C
  // or RES_FAIL if an operation failed.
  readTemperature: function(bme) {
    // C-functions output value of “1234” equals 12.34 Deg.
    return this._rt(bme) / 100.0;
  },

  // Returns the pressure from the sensor in hPa
  // or RES_FAIL if an operation failed.
  readPressure: function(bme) {
    // C-functions output value of “1234” equals 12.34 hPa.
    return this._rp(bme) / 10000.0;
  },

  // Returns the humidity from the sensor in %RH
  // or RES_FAIL if an operation failed.
  readHumidity: function(bme) {
    // C-functions output value of “1234” equals 12.34 %RH.
    return this._rh(bme) / 100.0;
  },

  // Returns the altitude in meters calculated from the specified
  // atmospheric pressure (in hPa), and sea-level pressure (in hPa)
  // or RES_FAIL if an operation failed.
  // http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf, P.16
  readAltitude: function(bme, seaLevel) {
    // C-functions input and output values of “1234” equals 12.34.
    return this._ra(bme, Math.round(seaLevel * 100.0)) / 100.0;
  },

  // Returns the pressure at sea level in hPa
  // calculated from the specified altitude (in meters),
  // and atmospheric pressure (in hPa)
  // or RES_FAIL if an operation failed.
  // http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf, P.17
  seaLevelForAltitude: function(bme, altitude, pressure) {
    // C-functions input and output values of “1234” equals 12.34.
    return this._alfa(bme, Math.round(altitude * 100.0), Math.round(pressure * 100.0)) / 100.0;
  },
};
