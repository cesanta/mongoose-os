# JS bindings for ESP32 touch pad sensor

## Overview

This library provides JavaScript bindings for the ESP32 touch pad sensor.
The JS API largely mirrors the [C API](https://github.com/espressif/esp-idf/blob/master/components/driver/include/driver/touch_pad.h).

## Examples

### Polling the sensor manually

```js
load('api_esp32_touchpad.js');

// Touch sensors are numbered from 0 to 9.
// For convenience, TouchPad.GPIO map translates from GPIO number to sensor number.
let ts = TouchPad.GPIO[15];

TouchPad.init();
TouchPad.setVoltage(TouchPad.HVOLT_2V4, TouchPad.LVOLT_0V8, TouchPad.HVOLT_ATTEN_1V5);
TouchPad.config(ts, 0);
Timer.set(1000 /* 1 sec */, Timer.REPEAT, function() {
  let tv = TouchPad.read(ts);
  print('Sensor', ts, 'value', tv);
}, null);

```

### Using interrupts

```js
load('api_esp32_touchpad.js');

// Touch sensors are numbered from 0 to 9.
// For convenience, TouchPad.GPIO map translates from GPIO number to sensor number.
let ts = TouchPad.GPIO[15];

TouchPad.init();
TouchPad.filterStart(10);
TouchPad.setMeasTime(0x1000, 0xffff);
TouchPad.setVoltage(TouchPad.HVOLT_2V4, TouchPad.LVOLT_0V8, TouchPad.HVOLT_ATTEN_1V5);
TouchPad.config(ts, 0);
Sys.usleep(100000); // wait a bit for initial filtering.
let noTouchVal = TouchPad.readFiltered(ts);
let touchThresh = noTouchVal * 2 / 3;
print('Sensor', ts, 'noTouchVal', noTouchVal, 'touchThresh', touchThresh);
TouchPad.setThresh(ts, touchThresh);
TouchPad.isrRegister(function(st) {
  // st is a bitmap with 1 bit per sensor.
  let val = TouchPad.readFiltered(ts);
  print('Status:', st, 'Value:', val);
}, null);
TouchPad.intrEnable();
```
