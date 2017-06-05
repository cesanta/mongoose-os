// Arduino Adafruit_SSD1306 library API. Source C API is defined at:
// [mgos_arduino_ssd1306.h](https://github.com/cesanta/mongoose-os/blob/master/arduino_drivers/Arduino/mgos_arduino_ssd1306.h)

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
  _dch: ffi('void mgos_ssd1306_draw_circle_helper(void *, int, int, int, int)'),
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
  _gcx: ffi('int mgos_ssd1306_get_cursor_x(void *)'),
  _gcy: ffi('int mgos_ssd1306_get_cursor_y(void *)'),

  RES_96_16: 0,
  RES_128_32: 1,
  RES_128_64: 2,

  EXTERNALVCC: 1,
  SWITCHCAPVCC: 2,

  BLACK: 0,
  WHITE: 1,
  INVERSE: 2,

  _proto: {
    // Close Adafruit_SSD1306 handle. Return value: none.
    close: function() {
      return Adafruit_SSD1306._close(this.ssd);
    },

    // Initialize the display.
    begin: function(vccst, addr, reset) {
      return Adafruit_SSD1306._begin(this.ssd, vccst, addr, reset);
    },

    ssd1306_command: function(cmd) {
      return Adafruit_SSD1306._cmd(this.ssd, cmd);
    },

    clearDisplay: function() {
      return Adafruit_SSD1306._cd(this.ssd);
    },

    invertDisplay: function(i) {
      return Adafruit_SSD1306._id(this.ssd, i);
    },

    display: function() {
      return Adafruit_SSD1306._d(this.ssd);
    },

    // Activates a right handed scroll for rows start through stop.
    startScrollRight: function(start, stop) {
      return Adafruit_SSD1306._ssr(this.ssd, start, stop);
    },

    // Activates a right handed scroll for rows start through stop.
    startScrollLeft: function(start, stop) {
      return Adafruit_SSD1306._ssl(this.ssd, start, stop);
    },

    // Activates a diagonal scroll for rows start through stop.
    startScrollDiagRight: function(start, stop) {
      return Adafruit_SSD1306._ssdr(this.ssd, start, stop);
    },

    // Activates a diagonal scroll for rows start through stop.
    startScrollDiagLeft: function(start, stop) {
      return Adafruit_SSD1306._ssdl(this.ssd, start, stop);
    },

    // Stops scroll.
    stopScroll: function() {
      return Adafruit_SSD1306._ss(this.ssd);
    },

    // Dims the display.
    // dim = 1: display is dimmed;
    // dim = 0: display is normal.
    dim: function(dim) {
      return Adafruit_SSD1306._dim(this.ssd, dim);
    },

    // Sets a single pixel.
    drawPixel: function(x, y, color) {
      return Adafruit_SSD1306._dp(this.ssd, x, y, color);
    },

    drawFastVLine: function(x, y, h, color) {
      return Adafruit_SSD1306._dfvl(this.ssd, x, y, h, color);
    },

    drawFastHLine: function(x, y, w, color) {
      return Adafruit_SSD1306._dfhl(this.ssd, x, y, w, color);
    },

    drawCircle: function(x0, y0, r, color) {
      return Adafruit_SSD1306._dc(this.ssd, _pair(x0, y0), r, color);
    },

    drawCircleHelper: function(x0, y0, r, cornername, color) {
      return Adafruit_SSD1306._dch(this.ssd, _pair(x0, y0), r, cornername, color);
    },

    fillCircle: function(x0, y0, r, color) {
      return Adafruit_SSD1306._fc(this.ssd, _pair(x0, y0), r, color);
    },

    fillCircleHelper: function(x0, y0, r, cornername, delta, color) {
      return Adafruit_SSD1306._fch(this.ssd, _pair(x0, y0), r, cornername, delta, color);
    },

    drawTriangle: function(x0, y0, x1, y1, x2, y2, color) {
      return Adafruit_SSD1306._dt(this.ssd, _pair(x0, y0), _pair(x1, y1), _pair(x2, y2), color);
    },

    fillTriangle: function(x0, y0, x1, y1, x2, y2, color) {
      return Adafruit_SSD1306._ft(this.ssd, _pair(x0, y0), _pair(x1, y1), _pair(x2, y2), color);
    },

    drawRoundRect: function(x0, y0, w, h, radius, color) {
      return Adafruit_SSD1306._drr(this.ssd, _pair(x0, y0), w, h, radius, color);
    },

    fillRoundRect: function(x0, y0, w, h, radius, color) {
      return Adafruit_SSD1306._frr(this.ssd, _pair(x0, y0), w, h, radius, color);
    },

    drawChar: function(x, y, c, color, bg, size) {
      return Adafruit_SSD1306._dch(this.ssd, _pair(x, y), c, color, bg, size);
    },

    setCursor: function(x, y) {
      return Adafruit_SSD1306._sc(this.ssd, x, y);
    },

    setTextColor: function(color) {
      return Adafruit_SSD1306._stc(this.ssd, color);
    },

    setTextColorBg: function(color, bg) {
      return Adafruit_SSD1306._stcb(this.ssd, color, bg);
    },

    setTextSize: function(size) {
      return Adafruit_SSD1306._sts(this.ssd, size);
    },

    setTextWrap: function(w) {
      return Adafruit_SSD1306._stw(this.ssd, w);
    },

    write: function(str) {
      return Adafruit_SSD1306._w(this.ssd, str, str.length);
    },

    height: function() {
      return Adafruit_SSD1306._ht(this.ssd);
    },

    width: function() {
      return Adafruit_SSD1306._wh(this.ssd);
    },

    getRotation: function() {
      return Adafruit_SSD1306._gr(this.ssd);
    },

    // get current cursor position (get rotation safe maximum values, using: width() for x, height() for y)
    getCursorX: function() {
      return Adafruit_SSD1306._gcx(this.ssd);
    },

    getCursorY: function() {
      return Adafruit_SSD1306._gcy(this.ssd);
    },
  },

  // Create a SSD1306 object for I2C.
  create_i2c: function(rst, res) {
    let obj = Object.create(Adafruit_SSD1306._proto);
    // Initialize Adafruit_SSD1306 library for I2C.
    // Return value: handle opaque pointer.
    // We set the reset pin and
    // Resolution: 0 - RES_96_16, 1 - RES_128_32, 2 - RES_128_64.
    obj.ssd = Adafruit_SSD1306._ci2c(rst, res);
    return obj;
  },

  // Create a SSD1306 object for SPI.
  create_spi: function(dc, rst, cs, res) {
    let obj = Object.create(Adafruit_SSD1306._proto);
    // Initialize Adafruit_SSD1306 library for SPI.
    // Return value: handle opaque pointer.
    // We set DataCommand, ChipSelect, Reset and
    // Resolution: 0 - RES_96_16, 1 - RES_128_32, 2 - RES_128_64.
    obj.ssd = Adafruit_SSD1306._cspi(dc, rst, cs, res);
    return obj;
  },
}
