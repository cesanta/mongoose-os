// Driver code for the AdaFruit NeoPixel RGB LED strips.
//   https://www.adafruit.com/category/168
//
// Usage example:
//
// ```javascript
// let pin = 5, numPixels = 16, colorOrder = NeoPixel.GRB;
// let s = NeoPixelStrip(pin, numPixels, colorOrder);
// NeoPixel.setPixel(s, 0 /* pixel */, 12, 34, 56);
// NeoPixel.show(s);
//
// NeoPixel.clear(s);
// NeoPixel.setPixel(s, 1 /* pixel */, 12, 34, 56);
// NeoPixel.show(s);
// ```


load("api_bitbang.js");
load("api_gpio.js");
load("api_sys.js");

function NeoPixelStrip(pin, numPixels, order) {
  GPIO.set_mode(pin, GPIO.MODE_OUTPUT);
  GPIO.write(pin, 0);  // Keep in reset.
  let s = {
    pin: pin,
    len: numPixels * 3,
    // Note: memory allocated here is currently not released.
    // This should be ok for now, we don't expect strips to be re-created.
    data: Sys.malloc(numPixels * 3),
    order: order,
  };
  NeoPixel.clear(s);
  return s;
}

let NeoPixel = {
  RGB: 0,
  GRB: 1,
  BGR: 2,

  // ## **`NeoPixel.setPixel(s, i, r, g, b)`**
  // Set i-th's pixel's RGB value.
  // Note that this only affects in-memory value of the pixel.
  setPixel: function(s, i, r, g, b) {
    let v0, v1, v2;
    if (s.order === NeoPixel.RGB) {
      v0 = r; v1 = g; v2 = b;
    } else if (s.order === NeoPixel.GRB) {
      v0 = g; v1 = r; v2 = b;
    } else if (s.order === NeoPixel.BGR) {
      v0 = b; v1 = g; v2 = r;
    } else {
      return;
    }
    s.data[i * 3] = v0;
    s.data[i * 3 + 1] = v1;
    s.data[i * 3 + 2] = v2;
  },

  // **`NeoPixel.clear(s)`** - clear in-memory values of the pixels.
  clear: function(s) {
    for (let i = 0; i < s.len; i++) {
      s.data[i] = 0;
    }
  },

  // **`NeoPixel.show(s)`** - output values of the pixels.
  show: function(s) {
    GPIO.write(s.pin, 0);
    Sys.usleep(60);
    BitBang.write(s.pin, BitBang.DELAY_100NSEC, 3, 8, 7, 6, s.data, s.len);
    GPIO.write(s.pin, 0);
    Sys.usleep(60);
    GPIO.write(s.pin, 1);
  },
};
