// Arduino Adafruit_SSD1306 library API. Source C API is defined at:
// [mgos_arduino_ssd1306.h](https://github.com/cesanta/mongoose-os/libs/arduino-adafruit-ssd1306/blob/master/src/mgos_arduino_ssd1306.h)

let Adafruit_SSD1306 = {
  _ci2c: ffi('void *mgos_ssd1306_create_i2c(int, int)'),
  _cspi: ffi('void *mgos_ssd1306_create_spi(int, int, int, int)'),
  _close: ffi('void mgos_ssd1306_close(void *)'),
  _begin: ffi('void mgos_ssd1306_begin(void *, int, int, int)'),
  _cmd: ffi('void mgos_ssd1306_command(void *, int)'),
  _cd: ffi('void mgos_ssd1306_clear_display(void *)'),
  _id: ffi('void mgos_ssd1306_invert_display(void *, int)'),
  _d: ffi('void mgos_ssd1306_display(void *)'),
  _ssr: ffi('void mgos_ssd1306_start_scroll_right(void *, int, int)'),
  _ssl: ffi('void mgos_ssd1306_start_scroll_left(void *, int, int)'),
  _ssdr: ffi('void mgos_ssd1306_start_scroll_diag_right(void *, int, int)'),
  _ssdl: ffi('void mgos_ssd1306_start_scroll_diag_left(void *, int, int)'),
  _ss: ffi('void mgos_ssd1306_stop_scroll(void *)'),
  _dim: ffi('void mgos_ssd1306_dim(void *, int)'),
  _dp: ffi('void mgos_ssd1306_draw_pixel(void *, int, int, int)'),
  _dfvl: ffi('void mgos_ssd1306_draw_fast_vline(void *, int, int, int, int)'),
  _dfhl: ffi('void mgos_ssd1306_draw_fast_hline(void *, int, int, int, int)'),
  _pair: ffi('int mgos_ssd1306_make_xy_pair(int, int)'),
  _dc: ffi('void mgos_ssd1306_draw_circle(void *, int, int, int)'),
  _dchl: ffi('void mgos_ssd1306_draw_circle_helper(void *, int, int, int, int)'),
  _fc: ffi('void mgos_ssd1306_fill_circle(void *, int, int, int)'),
  _fch: ffi('void mgos_ssd1306_fill_circle_helper(void *, int, int, int, int, int)'),
  _dt: ffi('void mgos_ssd1306_draw_triangle(void *, int, int, int, int)'),
  _ft: ffi('void mgos_ssd1306_fill_triangle(void *, int, int, int, int)'),
  _drr: ffi('void mgos_ssd1306_draw_round_rect(void *, int, int, int, int, int)'),
  _frr: ffi('void mgos_ssd1306_fill_round_rect(void *, int, int, int, int, int)'),
  _dch: ffi('void mgos_ssd1306_draw_char(void *, int, int, int, int, int)'),
  _sc: ffi('void mgos_ssd1306_set_cursor(void *, int, int)'),
  _stc: ffi('void mgos_ssd1306_set_text_color(void *, int)'),
  _stcb: ffi('void mgos_ssd1306_set_text_color_bg(void *, int, int)'),
  _sts: ffi('void mgos_ssd1306_set_text_size(void *, int)'),
  _stw: ffi('void mgos_ssd1306_set_text_wrap(void *, int)'),
  _w: ffi('int mgos_ssd1306_write(void *, char *, int)'),
  _ht: ffi('int mgos_ssd1306_height(void *)'),
  _wh: ffi('int mgos_ssd1306_width(void *)'),
  _gr: ffi('int mgos_ssd1306_get_rotation(void *)'),
  _sr: ffi('void mgos_ssd1306_set_rotation(void *, int)'),
  _gcx: ffi('int mgos_ssd1306_get_cursor_x(void *)'),
  _gcy: ffi('int mgos_ssd1306_get_cursor_y(void *)'),

  RES_96_16: 0,
  RES_128_32: 1,
  RES_128_64: 2,

  EXTERNALVCC: 1,
  SWITCHCAPVCC: 2,

  // ## **`Colors`**
  // - `Adafruit_SSD1306.BLACK`
  // - `Adafruit_SSD1306.WHITE`
  // - `Adafruit_SSD1306.INVERSE`
  BLACK: 0,
  WHITE: 1,
  INVERSE: 2,

  // ## **`Adafruit_SSD1306.create_i2c(rst, res)`**
  // Create an SSD1306 object for I2C. `rst` is a number of reset pin,
  // `res` is the resolution, one of the:
  // - `Adafruit_SSD1306.RES_96_16`
  // - `Adafruit_SSD1306.RES_128_32`
  // - `Adafruit_SSD1306.RES_128_64`
  //
  // Return value: an object with methods described below.
  // Example:
  // ```javascript
  // Adafruit_SSD1306.create_i2c(12, Adafruit_SSD1306.RES_128_32);
  // ```
  create_i2c: function(rst, res) {
    let obj = Object.create(Adafruit_SSD1306._proto);
    // Initialize Adafruit_SSD1306 library for I2C.
    // Return value: handle opaque pointer.
    // We set the reset pin and
    // Resolution: 0 - RES_96_16, 1 - RES_128_32, 2 - RES_128_64.
    obj.ssd = Adafruit_SSD1306._ci2c(rst, res);
    return obj;
  },

  // ## **`Adafruit_SSD1306.create_spi(dc, rst, cs, res)`**
  // Create an SSD1306 object for SPI.
  // `dc` is a number of data command pin, `rst` is a number of reset pin,
  // `cs` is a number of chip select pin, `res` is the resolution, one of the:
  // - `Adafruit_SSD1306.RES_96_16`
  // - `Adafruit_SSD1306.RES_128_32`
  // - `Adafruit_SSD1306.RES_128_64`
  //
  // Return value: an object with methods described below.
  // Example:
  // ```javascript
  // Adafruit_SSD1306.create_spi(10, 12, 11, Adafruit_SSD1306.RES_128_32);
  // ```
  create_spi: function(dc, rst, cs, res) {
    let obj = Object.create(Adafruit_SSD1306._proto);
    // Initialize Adafruit_SSD1306 library for SPI.
    // Return value: handle opaque pointer.
    // We set DataCommand, ChipSelect, Reset and
    // Resolution: 0 - RES_96_16, 1 - RES_128_32, 2 - RES_128_64.
    obj.ssd = Adafruit_SSD1306._cspi(dc, rst, cs, res);
    return obj;
  },

  _proto: {
    // ## **`mySSD1306.close()`**
    // Close Adafruit_SSD1306 instance. Return value: none.
    close: function() {
      return Adafruit_SSD1306._close(this.ssd);
    },

    // ## **`mySSD1306.begin(vccst, i2caddr, reset)`**
    // Initialize the display. `vccst` is a VCC state, one of those:
    // - `Adafruit_SSD1306.EXTERNALVCC`
    // - `Adafruit_SSD1306.SWITCHCAPVCC`
    // `i2caddr` is an I2C address (ignored if `create_spi` was used). `reset`
    // is a boolean; if true, then the display controller will be reset.
    // Return value: none.
    // Example:
    // ```javascript
    // mySSD1306.begin(Adafruit_SSD1306.EXTERNALVCC, 0x42, true);
    // ```
    begin: function(vccst, addr, reset) {
      return Adafruit_SSD1306._begin(this.ssd, vccst, addr, reset);
    },

    // ## **`mySSD1306.ssd1306_command(cmd)`**
    // Send an arbitrary command `cmd`, which must be a number from 0 to 255.
    // Return value: none.
    ssd1306_command: function(cmd) {
      return Adafruit_SSD1306._cmd(this.ssd, cmd);
    },

    // ## **`mySSD1306.clearDisplay()`**
    // Clear display. Return value: none.
    clearDisplay: function() {
      return Adafruit_SSD1306._cd(this.ssd);
    },

    // ## **`mySSD1306.invertDisplay(i)`**
    // Set invert mode: 0 - don't invert; 1 - invert. Return value: none.
    invertDisplay: function(i) {
      return Adafruit_SSD1306._id(this.ssd, i);
    },

    // ## **`mySSD1306.display()`**
    // Put image data to the display. Return value: none.
    display: function() {
      return Adafruit_SSD1306._d(this.ssd);
    },

    // ## **`mySSD1306.startScrollRight()`**
    // Activate a right handed scroll for rows from `start` to `stop`.
    // Return value: none.
    startScrollRight: function(start, stop) {
      return Adafruit_SSD1306._ssr(this.ssd, start, stop);
    },

    // ## **`mySSD1306.startScrollLeft()`**
    // Activate a left handed scroll for rows from `start` to `stop`.
    // Return value: none.
    startScrollLeft: function(start, stop) {
      return Adafruit_SSD1306._ssl(this.ssd, start, stop);
    },

    // ## **`mySSD1306.startScrollDiagRight()`**
    // Activate a diagonal scroll for rows from `start` to `stop`.
    // Return value: none.
    startScrollDiagRight: function(start, stop) {
      return Adafruit_SSD1306._ssdr(this.ssd, start, stop);
    },

    // ## **`mySSD1306.startScrollDiagLeft()`**
    // Activate a diagonal scroll for rows from `start` to `stop`.
    // Return value: none.
    startScrollDiagLeft: function(start, stop) {
      return Adafruit_SSD1306._ssdl(this.ssd, start, stop);
    },

    // ## **`mySSD1306.stopScroll()`**
    // Stop scrolling. Return value: none.
    stopScroll: function() {
      return Adafruit_SSD1306._ss(this.ssd);
    },

    // ## **`mySSD1306.dim(dim)`**
    // Set dim mode:
    // `dim` is 1: display is dimmed;
    // `dim` is 0: display is normal.
    // Return value: none.
    dim: function(dim) {
      return Adafruit_SSD1306._dim(this.ssd, dim);
    },

    // ## **`mySSD1306.drawPixel(x, y, color)`**
    // Set a single pixel with coords `x`, `y` to have the given `color`. See
    // available colors above.
    // Return value: none.
    drawPixel: function(x, y, color) {
      return Adafruit_SSD1306._dp(this.ssd, x, y, color);
    },

    // ## **`mySSD1306.drawFastVLine(x, y, h, color)`**
    // Draw a vertical line with height `h` starting from `x`, `y`, with color
    // `color`. See available colors above.
    // Return value: none.
    // Example:
    // ```javascript
    // mySSD1306.drawFastVLine(10, 5, 15, Adafruit_SSD1306.WHITE);
    // ```
    drawFastVLine: function(x, y, h, color) {
      return Adafruit_SSD1306._dfvl(this.ssd, x, y, h, color);
    },

    // ## **`mySSD1306.drawFastHLine(x, y, w, color)`**
    // Draw a horizontal line of width `w` starting from `x`, `y`, with color
    // `color`. See available colors above.
    // Return value: none.
    // Example:
    // ```javascript
    // mySSD1306.drawFastHLine(10, 10, 20, Adafruit_SSD1306.WHITE);
    // ```
    drawFastHLine: function(x, y, w, color) {
      return Adafruit_SSD1306._dfhl(this.ssd, x, y, w, color);
    },

    // ## **`mySSD1306.drawCircle(x, y, r, color)`**
    // Draw a circle with the radius `r`, centered at from `x`, `y`, with color
    // `color`. See available colors above.
    // Return value: none.
    // Example:
    // ```javascript
    // mySSD1306.drawCircle(10, 10, 20, 10, 3, Adafruit_SSD1306.WHITE);
    // ```
    drawCircle: function(x0, y0, r, color) {
      return Adafruit_SSD1306._dc(this.ssd, Adafruit_SSD1306._pair(x0, y0), r, color);
    },

    drawCircleHelper: function(x0, y0, r, cornername, color) {
      return Adafruit_SSD1306._dchl(this.ssd, Adafruit_SSD1306._pair(x0, y0), r, cornername, color);
    },

    // ## **`mySSD1306.fillCircle(x, y, r, color)`**
    // Draw a filled circle with the radius `r`, centered at from `x`, `y`,
    // with color `color`. See available colors above.
    // Return value: none.
    // Example:
    // ```javascript
    // mySSD1306.fillCircle(10, 10, 5, Adafruit_SSD1306.WHITE);
    // ```
    fillCircle: function(x0, y0, r, color) {
      return Adafruit_SSD1306._fc(this.ssd, Adafruit_SSD1306._pair(x0, y0), r, color);
    },

    fillCircleHelper: function(x0, y0, r, cornername, delta, color) {
      return Adafruit_SSD1306._fch(this.ssd, Adafruit_SSD1306._pair(x0, y0), r, cornername, delta, color);
    },

    // ## **`mySSD1306.drawTriangle(x0, y0, x1, y1, x2, y2, color)`**
    // Draw a triangle at the given coordinates, with color `color`. See
    // available colors above.
    // Return value: none.
    // Example:
    // ```javascript
    // mySSD1306.drawTriangle(10, 0, 20, 20, 0, 20, Adafruit_SSD1306.WHITE);
    // ```
    drawTriangle: function(x0, y0, x1, y1, x2, y2, color) {
      return Adafruit_SSD1306._dt(this.ssd, Adafruit_SSD1306._pair(x0, y0), Adafruit_SSD1306._pair(x1, y1), Adafruit_SSD1306._pair(x2, y2), color);
    },

    fillTriangle: function(x0, y0, x1, y1, x2, y2, color) {
      return Adafruit_SSD1306._ft(this.ssd, Adafruit_SSD1306._pair(x0, y0), Adafruit_SSD1306._pair(x1, y1), Adafruit_SSD1306._pair(x2, y2), color);
    },

    // ## **`mySSD1306.drawRoundRect(x0, y0, w, h, radius, color)`**
    // Draw a rectangle with round corners; `x0`, `y0` are the coords of the
    // left-top corner, `w` is width, `h` is height, `radius` is the corners
    // radius, with color `color`. See available colors above.
    // Return value: none.
    // Example:
    // ```javascript
    // mySSD1306.drawRoundRect(10, 10, 20, 10, 3, Adafruit_SSD1306.WHITE);
    // ```
    drawRoundRect: function(x0, y0, w, h, radius, color) {
      return Adafruit_SSD1306._drr(this.ssd, Adafruit_SSD1306._pair(x0, y0), w, h, radius, color);
    },

    // ## **`mySSD1306.drawRoundRect(x0, y0, w, h, radius, color)`**
    // Draw a filled rectangle with round corners; `x0`, `y0` are the coords of
    // the left-top corner, `w` is width, `h` is height, `radius` is the
    // corners radius, with color `color`. See available colors above.
    // Return value: none.
    // Example:
    // ```javascript
    // mySSD1306.fillRoundRect(10, 10, 20, 10, 3, Adafruit_SSD1306.WHITE);
    // ```
    fillRoundRect: function(x0, y0, w, h, radius, color) {
      return Adafruit_SSD1306._frr(this.ssd, Adafruit_SSD1306._pair(x0, y0), w, h, radius, color);
    },

    // ## **`mySSD1306.drawChar(x, y, c, color, bg, size)`**
    // Draw a character `c` starting at the point `x`, `y`, with the color
    // `color` (see available colors above). If `bg` is different from `color`,
    // then the background is filled with `bg`; otherwise bacground is left
    // intact.
    //
    // There is only one font (to save space) and it's meant to be 5x8 pixels,
    // but an optional `size` parameter which scales the font by this factor (e.g.
    // size=2 will render the text at 10x16 pixels per character).
    // Return value: none.
    // Example:
    // ```javascript
    // mySSD1306.drawChar(10, 10, 'a',
    //                    Adafruit_SSD1306.WHITE, Adafruit_SSD1306.WHITE, 1);
    // ```
    drawChar: function(x, y, c, color, bg, size) {
      size = size || 1;
      return Adafruit_SSD1306._dch(this.ssd, Adafruit_SSD1306._pair(x, y), c.at(0), color, bg, size);
    },

    // ## **`mySSD1306.setCursor(x, y)`**
    // Set text cursor for the following calls to `mySSD1306.write()`.
    // See example for `write()` below.
    // Return value: none.
    setCursor: function(x, y) {
      return Adafruit_SSD1306._sc(this.ssd, x, y);
    },

    // ## **`mySSD1306.setTextColor(color)`**
    // Set text color for the following calls to `mySSD1306.write()`. See
    // available colors above.
    // See example for `write()` below.
    // Return value: none.
    setTextColor: function(color) {
      return Adafruit_SSD1306._stc(this.ssd, color);
    },

    // ## **`mySSD1306.setTextColorBg(color, bg)`**
    // Set text color and background color for the following calls to
    // `mySSD1306.write()`. If `bg` is equal to the `color`, then the
    // background will be left intact while drawing characters.
    // See example for `write()` below.
    // Return value: none.
    setTextColorBg: function(color, bg) {
      return Adafruit_SSD1306._stcb(this.ssd, color, bg);
    },

    // ## **`mySSD1306.setTextSize(size)`**
    // Set text color for the following calls to `mySSD1306.write()`. There is
    // only one font (to save space) and it's meant to be 5x8 pixels, but an
    // optional `size` parameter which scales the font by this factor (e.g.
    // size=2 will render the text at 10x16 pixels per character).
    // See example for `write()` below.
    // Return value: none.
    setTextSize: function(size) {
      return Adafruit_SSD1306._sts(this.ssd, size);
    },

    // ## **`mySSD1306.setTextWrap(wrap)`**
    // Set text wrap mode (true or false) for the following calls to
    // `mySSD1306.write()`.
    // See example for `write()` below.
    // Return value: none.
    setTextWrap: function(w) {
      return Adafruit_SSD1306._stw(this.ssd, w);
    },

    // ## **`mySSD1306.write(str)`**
    // Write given string `str` using the parameters set before (`setCursor()`,
    // `setTextColor()`, `setTextColorBg()`, `setTextSize()`, `setTextWrap()`)
    // Return value: 1.
    // Example:
    // ```javascript
    // mySSD1306.setCursor(10, 10);
    // mySSD1306.setTextColor(Adafruit_SSD1306.WHITE);
    // mySSD1306.setTextSize(2);
    // mySSD1306.setTextWrap(true);
    // mySSD1306.write("Hello world!");
    // ```
    write: function(str) {
      return Adafruit_SSD1306._w(this.ssd, str, str.length);
    },

    // ## **`mySSD1306.height()`**
    // Return display height in pixels.
    height: function() {
      return Adafruit_SSD1306._ht(this.ssd);
    },

    // ## **`mySSD1306.width()`**
    // Return display width in pixels.
    width: function() {
      return Adafruit_SSD1306._wh(this.ssd);
    },

    // ## **`mySSD1306.setRotation(rot)`**
    // Set display rotation:
    // - 0: no rotation
    // - 1: rotated at 90 degrees
    // - 2: rotated at 180 degrees
    // - 3: rotated at 270 degrees
    setRotation: function(rot) {
      return Adafruit_SSD1306._sr(this.ssd, rot);
    },

    // ## **`mySSD1306.getRotation()`**
    // Return rotation previously set with `setRotation()`
    getRotation: function() {
      return Adafruit_SSD1306._gr(this.ssd);
    },

    // ## **`mySSD1306.getCursorX()`**
    // Return cursor X coordinate, previously set with `setCursor()`.
    getCursorX: function() {
      return Adafruit_SSD1306._gcx(this.ssd);
    },

    // ## **`mySSD1306.getCursorY()`**
    // Return cursor Y coordinate, previously set with `setCursor()`.
    getCursorY: function() {
      return Adafruit_SSD1306._gcy(this.ssd);
    },
  },

};
