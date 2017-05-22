/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 * This example demonstrates how to use mJS Arduino Adafruit_SSD1306
 * library API.
 */

// Load Mongoose OS API
load('api_timer.js');
load('api_arduino_ssd1306.js');

// Initialize Adafruit_SSD1306 library (I2C)
let d = Adafruit_SSD1306.create_i2c(4 /* RST GPIO */, Adafruit_SSD1306.RES_128_64);
// Initialize the display.
d.begin(Adafruit_SSD1306.SWITCHCAPVCC, 0x3D, true /* reset */);
d.display();
let i = 0;

let showStr = function(d, str) {
  d.clearDisplay();
  d.setTextSize(2);
  d.setTextColor(Adafruit_SSD1306.WHITE);
  d.setCursor(d.width() / 4, d.height() / 4);
  d.write(str);
  d.display();
};

Timer.set(1000 /* milliseconds */, true /* repeat */, function() {
  showStr(d, "i = " + JSON.stringify(i));
  print("i = ", i);
  i++;
}, null);
