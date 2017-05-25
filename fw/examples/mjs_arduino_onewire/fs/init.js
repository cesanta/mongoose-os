/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 * This example demonstrates how to use mJS Arduino OneWire
 * library API to get data from DS18B20 temperature sensors.
 * Datasheet: http://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
 */

// Load Mongoose OS API
load('api_timer.js');
// Load example DS18B20
load('ds18b20.js');

// Number of sensors found on the 1-Wire bus
let n = 0;
// Sensors addresses
let rom = ['01234567'];

// Search for sensors
let searchSens = function() {
  let i = 0;
  // Setup the search to find the device type on the next call
  // to search() if it is present.
  ow.target_search(DEVICE_FAMILY.DS18B20);

  while (ow.search(rom[i], 0/* Normal search mode */) === 1) {
    // If no devices of the desired family are currently on the bus, 
    // then another type will be found. We should check it.
    if (rom[i][0].charCodeAt(0) !== DEVICE_FAMILY.DS18B20) {
      break;
    }
    // Sensor found
    print('Sensor#', i, 'address:', toHexStr(rom[i]));
    rom[++i] = '01234567';
  }
  return i;
};

// This function prints temperature every second
Timer.set(1000 /* milliseconds */, true /* repeat */, function() {
  if (n === 0) {
    if ((n = searchSens()) === 0) {
      print('No device found');
    }
  }

  for (let i = 0; i < n; i++) {
    print('Sensor#', i, 'Temperature:', getTemp(rom[i]), '*C');
  }
}, null);
