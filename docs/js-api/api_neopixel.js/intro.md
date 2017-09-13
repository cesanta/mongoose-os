---
title: "NeoPixel"
items:
---

Driver code for the AdaFruit NeoPixel RGB LED strips.
  https://www.adafruit.com/category/168

load("api_bitbang.js");
load("api_gpio.js");
load("api_sys.js");



## **`NeoPixel.create(pin, numPixels, order)`**
Create a NeoPixel strip object. Example:
```javascript
let pin = 5, numPixels = 16, colorOrder = NeoPixel.GRB;
let strip = NeoPixel.create(pin, numPixels, colorOrder);
strip.setPixel(0 /* pixel */, 12, 34, 56);
strip.show();

strip.clear();
strip.setPixel(1 /* pixel */, 12, 34, 56);
strip.show();
```



Note: memory allocated here is currently not released.
This should be ok for now, we don't expect strips to be re-created.



## **`strip.setPixel(i, r, g, b)`**
Set i-th's pixel's RGB value.
Note that this only affects in-memory value of the pixel.



## **`strip.clear()`**
Clear in-memory values of the pixels.



## **`strip.show()`**
Output values of the pixels.

