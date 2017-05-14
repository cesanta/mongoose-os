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
let dht = DHT.init(pin, DHT.DHT22);
// Initialize a device
DHT.begin(dht);

// This function reads data from the DHT sensor every 2 second
Timer.set(2000 /* milliseconds */, true /* repeat */, function() {
  let t = DHT.readTemperature(dht, 0, 0);
  let h = DHT.readHumidity(dht, 0);

  print('Temperature, *C: ');
  print(t);
  print('Humidity, %: ');
  print(h);
  print('Heat index, *C: ');
  print(DHT.computeHeatIndex(dht, t, h, 0));
}, null);
