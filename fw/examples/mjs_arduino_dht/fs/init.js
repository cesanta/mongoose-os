/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 * This example demonstrates how to use mJS Arduino DHT library API
 * to get data from DHTxx temperature and humidity sensors.
 * Datasheet: https://cdn-shop.adafruit.com/datasheets/
 *            Digital+humidity+and+temperature+sensor+AM2302.pdf
 */

// Load Mongoose OS API
load('api_timer.js');
load('api_arduino_dht.js');

// GPIO pin which has a DHT sensor data wire connected
let pin = 13;

// Initialize Adafruit DHT library
let dht = DHT.create(pin, DHT.DHT22);
// Initialize a device
dht.begin();

// This function reads data from the DHT sensor every 2 second
Timer.set(2000 /* milliseconds */, true /* repeat */, function() {
  let t = dht.readTemperature(0, 0);
  let h = dht.readHumidity(0);

  print('Temperature:', t, '*C');
  print('Humidity:', h, '%');
  print('Heat index:', dht.computeHeatIndex(t, h, 0), '*C');
}, null);
