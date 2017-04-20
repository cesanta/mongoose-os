---
title: "NeoPixel"
items:
---

Driver code for the AdaFruit NeoPixel RGB LED strips.
  https://www.adafruit.com/category/168

Usage example:

```javascript
let pin = 5, numPixels = 16, colorOrder = NeoPixel.GRB;
let s = NeoPixelStrip(pin, numPixels, colorOrder);
NeoPixel.setPixel(s, 0 /* pixel */, 12, 34, 56);
NeoPixel.show(s);

NeoPixel.clear(s);
NeoPixel.setPixel(s, 1 /* pixel */, 12, 34, 56);
NeoPixel.show(s);
```

load("api_bitbang.js");
load("api_gpio.js");
load("api_sys.js");

function NeoPixelStrip(pin, numPixels, order) {
  GPIO.set_mode(pin, GPIO.MODE_OUTPUT);
  GPIO.write(pin, 0);  // Keep in reset.
  let s = {
    pin: pin,
    len: numPixels * 3,
Note: memory allocated here is currently not released.
This should be ok for now, we don't expect strips to be re-created.
    data: Sys.malloc(numPixels * 3),
    order: order,
  };
  NeoPixel.clear(s);
  return s;
}



Note: memory allocated here is currently not released.
This should be ok for now, we don't expect strips to be re-created.



## **`NeoPixel.setPixel(s, i, r, g, b)`**
Set i-th's pixel's RGB value.
Note that this only affects in-memory value of the pixel.



## **`NeoPixel.clear(s)`**
Clear in-memory values of the pixels.



## **`NeoPixel.show(s)`**
Output values of the pixels.

