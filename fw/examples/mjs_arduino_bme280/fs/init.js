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
let bme = Adafruit_BME280.create();
// Initialize the sensor
if (bme.begin(sens_addr) === 0) {
  print('Cant find a sensor');
} else {
  // This function reads data from the BME280 sensor every 2 seconds
  Timer.set(2000 /* milliseconds */, true /* repeat */, function() {
    print('Temperature:', bme.readTemperature(), '*C');
    print('Humidity:', bme.readHumidity(), '%RH');
    print('Pressure:', bme.readPressure(), 'hPa');
  }, null);
}
