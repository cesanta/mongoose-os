/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 * This example demonstrates how to use mJS Arduino Adafruit_BME280
 * library API to get data from BME280 combined humidity and pressure sensor.
 */

// Load Mongoose OS API
load('api_timer.js');
load('api_arduino_bme280.js');

// Sensors address
let sens_addr = 0x76;
// Initialize Adafruit_BME280 library
let bme = Adafruit_BME280.init();
// Initialize the sensor
if (Adafruit_BME280.begin(bme, sens_addr) === 0) {
  print('Cant find a sensor');
} else {
  // This function reads data from the BME280 sensor every 2 seconds
  Timer.set(2000 /* milliseconds */, true /* repeat */, function() {
    print('Temperature: ');
    print(Adafruit_BME280.readTemperature(bme));
    print('Humidity: ');
    print(Adafruit_BME280.readHumidity(bme));
    print('Pressure: ');
    print(Adafruit_BME280.readPressure(bme));
    print('Altitude: ');
    print(Adafruit_BME280.readAltitude(bme, 1013.25/*hPa*/));
  }, null);
}
