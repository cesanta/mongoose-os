// Arduino Adafruit_BME280 library API. Source C API is defined at:
// [mgos_arduino_bme280.h](https://github.com/cesanta/mongoose-os/libs/arduino-adafruit-bme280/blob/master/src/mgos_arduino_bme280.h)

load('api_math.js');

let Adafruit_BME280 = {
  // Error codes
  RES_FAIL: -100.0,

  _c_i2c: ffi('void *mgos_bme280_create_i2c(void)'),
  _c_spi: ffi('void *mgos_bme280_create_spi(int)'),
  _c_spi_full: ffi('void *mgos_bme280_create_spi_full(int, int, int, int)'),
  _close: ffi('void mgos_bme280_close(void *)'),
  _begin: ffi('int mgos_bme280_begin(void *, int)'),
  _tfm: ffi('void mgos_bme280_take_forced_measurement(void *)'),
  _rt: ffi('int mgos_bme280_read_temperature(void *)'),
  _rp: ffi('int mgos_bme280_read_pressure(void *)'),
  _rh: ffi('int mgos_bme280_read_humidity(void *)'),
  _ra: ffi('int mgos_bme280_read_altitude(void *, int)'),
  _alfa: ffi('int mgos_bme280_sea_level_for_altitude(void *, int, int)'),

  // ## **`Adafruit_BME280.createI2C(i2caddr)`**
  // Create a BME280 instance on I2C bus with the given address `i2caddr`.
  // Return value: an object with the methods described below.
  createI2C: function(i2caddr) {
    let obj = Object.create(Adafruit_BME280._proto);
    // Initialize Adafruit_BME280 library.
    // Return value: handle opaque pointer.
    obj.bme = Adafruit_BME280._c_i2c();
    let b = Adafruit_BME280._begin(obj.bme, i2caddr);
    if (b === 0) {
      // Can't find a sensor
      return undefined;
    }
    return obj;
  },

  // ## **`Adafruit_BME280.createSPI(cspin)`**
  // Create a BME280 instance on SPI bus with the given Chip Select pin `cspin`.
  // Return value: an object with the methods described below.
  createSPI: function(cspin) {
    let obj = Object.create(Adafruit_BME280._proto);
    // Initialize Adafruit_BME280 library.
    // Return value: handle opaque pointer.
    obj.bme = Adafruit_BME280._c_spi(cspin);
    let b = Adafruit_BME280._begin(obj.bme, undefined);
    if (b === 0) {
      // Can't find a sensor
      return undefined;
    }
    return obj;
  },

  // ## **`Adafruit_BME280.createSPIFull(cspin, mosipin, misopin, sckpin)`**
  // Create a BME280 instance on SPI bus with the given pins `cspin`,
  // `mosipin`, `misopin`, `sckpin`.
  // Return value: an object with the methods described below.
  createSPIFull: function(cspin, mosipin, misopin, sckpin) {
    let obj = Object.create(Adafruit_BME280._proto);
    // Initialize Adafruit_BME280 library.
    // Return value: handle opaque pointer.
    obj.bme = Adafruit_BME280._c_spi_full(cspin, mosipin, misopin, sckpin);
    let b = Adafruit_BME280._begin(obj.bme, undefined);
    if (b === 0) {
      // Can't find a sensor
      return undefined;
    }
    return obj;
  },

  _proto: {
    // ## **`myBME.close()`**
    // Close Adafruit_BME280 instance; no methods can be called on this instance
    // after that.
    // Return value: none.
    close: function() {
      return Adafruit_BME280._close(this.bme);
    },

    // ## **`myBME.takeForcedMeasurement()`**
    // Take a new measurement (only possible in forced mode).
    takeForcedMeasurement: function() {
      return Adafruit_BME280._tfm(this.bme);
    },

    // ## **`myBME.readTemperature()`**
    // Return the temperature from the sensor in degrees C or
    // `Adafruit_BME280.RES_FAIL` in case of a failure.
    readTemperature: function() {
      // C-functions output value of “1234” equals 12.34 Deg.
      return Adafruit_BME280._rt(this.bme) / 100.0;
    },

    // ## **`myBME.readPressure()`**
    // Returns the pressure from the sensor in hPa
    // or `Adafruit_BME280.RES_FAIL` in case of a failure.
    readPressure: function() {
      // C-functions output value of “1234” equals 12.34 hPa.
      return Adafruit_BME280._rp(this.bme) / 10000.0;
    },

    // ## **`myBME.readHumidity()`**
    // Returns the humidity from the sensor in %RH
    // or `Adafruit_BME280.RES_FAIL` in case of a failure.
    readHumidity: function() {
      // C-functions output value of “1234” equals 12.34 %RH.
      return Adafruit_BME280._rh(this.bme) / 100.0;
    },

    // ## **`myBME.readAltitude(seaLevel)`**
    // Returns the altitude in meters calculated from the specified
    // sea-level pressure `seaLevel` (in hPa)
    // or `Adafruit_BME280.RES_FAIL` in case of a failure.
    // http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf, P.16
    readAltitude: function(lvl) {
      // C-functions input and output values of “1234” equals 12.34.
      return Adafruit_BME280._ra(this.bme, Math.round(lvl * 100.0)) / 100.0;
    },

    // ## **`myBME.seaLevelForAltitude(alt, pres)`**
    // Returns the pressure at sea level in hPa
    // calculated from the specified altitude `alt` (in meters),
    // and atmospheric pressure `pres` (in hPa)
    // or `Adafruit_BME280.RES_FAIL` in case of a failure.
    // http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf, P.17
    seaLevelForAltitude: function(alt, pres) {
      // C-functions input and output values of “1234” equals 12.34.
      return Adafruit_BME280._alfa(this.bme, Math.round(alt * 100.0), Math.round(pres * 100.0)) / 100.0;
    },
  },

};
