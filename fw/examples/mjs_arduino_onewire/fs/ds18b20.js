/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 * This example demonstrates how to use mJS Arduino OneWire
 * library API to get data from DS18B20 temperature sensors.
 * Datasheet: http://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
 */

load('api_arduino_onewire.js');

let GPIO = {
  PIN: 13 // GPIO pin which has sensors data wire connected
};

let DEVICE_FAMILY = {
  DS18B20: 0x28
};

// Initialize OneWire library
let ow = OneWire.create(GPIO.PIN);

// Get device address in hex format
let toHexStr = function(addr) {
  let byte2hex = function(byte) {
    let hex_char = '0123456789abcdef';
    return hex_char[(byte >> 4) & 0x0F] + hex_char[byte & 0x0F];
  };
  let res = '';
  for (let i = 0; i < addr.length; i++) {
    res += byte2hex(addr.charCodeAt(i));
  }
  return res;
};

// This function reads data from the DS18B20 temperature sensor
let getTemp = function(rom) {
  let DATA = {
    TEMP_LSB: 0,
    TEMP_MSB: 1,
    REG_CONF: 4,
    SCRATCHPAD_SIZE: 9
  };

  let REG_CONF = {
    RESOLUTION_9BIT: 0x00,
    RESOLUTION_10BIT: 0x20,
    RESOLUTION_11BIT: 0x40,
    RESOLUTION_MASK: 0x60
  };

  let CMD = {
    CONVERT_T: 0x44,
    READ_SCRATCHPAD: 0xBE
  };

  let data = [];
  let raw;
  let cfg;

  ow.reset();
  ow.select(rom);
  ow.write(CMD.CONVERT_T);

  ow.delay(750);

  ow.reset();
  ow.select(rom);    
  ow.write(CMD.READ_SCRATCHPAD);

  for (let i = 0; i < DATA.SCRATCHPAD_SIZE; i++) {
    data[i] = ow.read();
  }

  raw = (data[DATA.TEMP_MSB] << 8) | data[DATA.TEMP_LSB];
  cfg = (data[DATA.REG_CONF] & REG_CONF.RESOLUTION_MASK);
  
  if (cfg === REG_CONF.RESOLUTION_9BIT) {
    raw = raw & ~7;
  } else if (cfg === REG_CONF.RESOLUTION_10BIT) {
    raw = raw & ~3;
  } else if (cfg === REG_CONF.RESOLUTION_11BIT) {
    raw = raw & ~1;
  }
  // Default resolution is 12 bit

  return raw / 16.0;
};
