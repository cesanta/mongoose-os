// See on GitHub:
// [api_neopixel.js](https://github.com/cesanta/mongoose-os/blob/master/fw/mjs_api/api_neopixel.js)

load("api_bitbang.js");
load("api_gpio.js");
load("api_sys.js");

let NeoPixel = {
  RGB: 0,
  GRB: 1,
  BGR: 2,

  // ## **`NeoPixel.create(pin, numPixels, order)`**
  // Create and return a NeoPixel strip object. Example:
  // ```javascript
  // let pin = 5, numPixels = 16, colorOrder = NeoPixel.GRB;
  // let strip = NeoPixel.create(pin, numPixels, colorOrder);
  // strip.setPixel(0 /* pixel */, 12, 34, 56);
  // strip.show();
  //
  // strip.clear();
  // strip.setPixel(1 /* pixel */, 12, 34, 56);
  // strip.show();
  // ```
  create: function(pin, numPixels, order) {
    GPIO.set_mode(pin, GPIO.MODE_OUTPUT);
    GPIO.write(pin, 0);  // Keep in reset.
    let s = Object.create({
      pin: pin,
      len: numPixels * 3,
      // Note: memory allocated here is currently not released.
      // This should be ok for now, we don't expect strips to be re-created.
      data: Sys.malloc(numPixels * 3),
      order: order,
      setPixel: NeoPixel.set,
      clear: NeoPixel.clear,
      show: NeoPixel.show,
    });
    s.clear();
    return s;
  },

  // ## **`strip.setPixel(i, r, g, b)`**
  // Set i-th's pixel's RGB value.
  // Note that this only affects in-memory value of the pixel.
  set: function(i, r, g, b) {
    let v0, v1, v2;
    if (this.order === NeoPixel.RGB) {
      v0 = r; v1 = g; v2 = b;
    } else if (this.order === NeoPixel.GRB) {
      v0 = g; v1 = r; v2 = b;
    } else if (this.order === NeoPixel.BGR) {
      v0 = b; v1 = g; v2 = r;
    } else {
      return;
    }
    this.data[i * 3] = v0;
    this.data[i * 3 + 1] = v1;
    this.data[i * 3 + 2] = v2;
  },

  // ## **`strip.clear()`**
  // Clear in-memory values of the pixels.
  clear: function() {
    for (let i = 0; i < this.len; i++) {
      this.data[i] = 0;
    }
  },

  // ## **`strip.show()`**
  // Output values of the pixels.
  show: function() {
    GPIO.write(this.pin, 0);
    Sys.usleep(60);
    BitBang.write(this.pin, BitBang.DELAY_100NSEC, 3, 8, 7, 6, this.data, this.len);
    GPIO.write(this.pin, 0);
    Sys.usleep(60);
    GPIO.write(this.pin, 1);
  },
};
