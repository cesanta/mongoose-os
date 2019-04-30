/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 * Arduino Adafruit_SSD1306 library API wrapper
 */

#include "mgos_arduino_ssd1306.h"

static Adafruit_SSD1306::Resolution mgos_ssd1306_get_res(int res) {
  Adafruit_SSD1306::Resolution r;
  switch (res) {
    case MGOS_SSD1306_RES_96_16:
      r = Adafruit_SSD1306::RES_96_16;
      break;
    case MGOS_SSD1306_RES_128_32:
      r = Adafruit_SSD1306::RES_128_32;
      break;
    case MGOS_SSD1306_RES_128_64:
      r = Adafruit_SSD1306::RES_128_64;
      break;
    default:
      r = SSD1306_DEFAULT_RES;
  }
  return r;
}

Adafruit_SSD1306 *mgos_ssd1306_create_i2c(int rst, int res) {
  return new Adafruit_SSD1306(rst, mgos_ssd1306_get_res(res));
}

Adafruit_SSD1306 *mgos_ssd1306_create_spi(int dc, int rst, int cs, int res) {
  return new Adafruit_SSD1306(dc, rst, cs, mgos_ssd1306_get_res(res));
}

void mgos_ssd1306_close(Adafruit_SSD1306 *ssd) {
  if (ssd != nullptr) {
    delete ssd;
    ssd = nullptr;
  }
}

void mgos_ssd1306_begin(Adafruit_SSD1306 *ssd, int vccstate, int i2caddr,
                        bool reset) {
  if (ssd == nullptr) return;
  ssd->begin(vccstate, i2caddr, reset);
}

void mgos_ssd1306_command(Adafruit_SSD1306 *ssd, int c) {
  if (ssd == nullptr) return;
  ssd->ssd1306_command(c);
}

void mgos_ssd1306_clear_display(Adafruit_SSD1306 *ssd) {
  if (ssd == nullptr) return;
  ssd->clearDisplay();
}

void mgos_ssd1306_invert_display(Adafruit_SSD1306 *ssd, int i) {
  if (ssd == nullptr) return;
  ssd->invertDisplay(i);
}

void mgos_ssd1306_display(Adafruit_SSD1306 *ssd) {
  if (ssd == nullptr) return;
  ssd->display();
}

void mgos_ssd1306_start_scroll_right(Adafruit_SSD1306 *ssd, int start,
                                     int stop) {
  if (ssd == nullptr) return;
  ssd->startscrollright(start, stop);
}

void mgos_ssd1306_start_scroll_left(Adafruit_SSD1306 *ssd, int start,
                                    int stop) {
  if (ssd == nullptr) return;
  ssd->startscrollleft(start, stop);
}

void mgos_ssd1306_start_scroll_diag_right(Adafruit_SSD1306 *ssd, int start,
                                          int stop) {
  if (ssd == nullptr) return;
  ssd->startscrolldiagright(start, stop);
}

void mgos_ssd1306_start_scroll_diag_left(Adafruit_SSD1306 *ssd, int start,
                                         int stop) {
  if (ssd == nullptr) return;
  ssd->startscrolldiagleft(start, stop);
}

void mgos_ssd1306_stop_scroll(Adafruit_SSD1306 *ssd) {
  if (ssd == nullptr) return;
  ssd->stopscroll();
}

void mgos_ssd1306_dim(Adafruit_SSD1306 *ssd, int dim) {
  if (ssd == nullptr) return;
  ssd->dim(dim);
}

void mgos_ssd1306_draw_pixel(Adafruit_SSD1306 *ssd, int x, int y, int color) {
  if (ssd == nullptr) return;
  ssd->drawPixel(x, y, color);
}

void mgos_ssd1306_draw_fast_vline(Adafruit_SSD1306 *ssd, int x, int y, int h,
                                  int color) {
  if (ssd == nullptr) return;
  ssd->drawFastVLine(x, y, h, color);
}

void mgos_ssd1306_draw_fast_hline(Adafruit_SSD1306 *ssd, int x, int y, int w,
                                  int color) {
  if (ssd == nullptr) return;
  ssd->drawFastHLine(x, y, w, color);
}

// Adafruit_GFX
int mgos_ssd1306_make_xy_pair(int x, int y) {
  return ((int) x << 16) + y;
}

void mgos_ssd1306_draw_circle(Adafruit_SSD1306 *ssd, int x0y0, int r,
                              int color) {
  if (ssd == nullptr) return;
  ssd->drawCircle((x0y0 >> 16), x0y0, r, color);
}

void mgos_ssd1306_draw_circle_helper(Adafruit_SSD1306 *ssd, int x0y0, int r,
                                     int cornername, int color) {
  if (ssd == nullptr) return;
  ssd->drawCircleHelper((x0y0 >> 16), x0y0, r, cornername, color);
}

void mgos_ssd1306_fill_circle(Adafruit_SSD1306 *ssd, int x0y0, int r,
                              int color) {
  if (ssd == nullptr) return;
  ssd->fillCircle((x0y0 >> 16), x0y0, r, color);
}

void mgos_ssd1306_fill_circle_helper(Adafruit_SSD1306 *ssd, int x0y0, int r,
                                     int cornername, int delta, int color) {
  if (ssd == nullptr) return;
  ssd->fillCircleHelper((x0y0 >> 16), x0y0, r, cornername, delta, color);
}

void mgos_ssd1306_draw_triangle(Adafruit_SSD1306 *ssd, int x0y0, int x1y1,
                                int x2y2, int color) {
  if (ssd == nullptr) return;
  ssd->drawTriangle((x0y0 >> 16), x0y0, (x1y1 >> 16), x1y1, (x2y2 >> 16), x2y2,
                    color);
}

void mgos_ssd1306_fill_triangle(Adafruit_SSD1306 *ssd, int x0y0, int x1y1,
                                int x2y2, int color) {
  if (ssd == nullptr) return;
  ssd->fillTriangle((x0y0 >> 16), x0y0, (x1y1 >> 16), x1y1, (x2y2 >> 16), x2y2,
                    color);
}

void mgos_ssd1306_draw_round_rect(Adafruit_SSD1306 *ssd, int x0y0, int w, int h,
                                  int radius, int color) {
  if (ssd == nullptr) return;
  ssd->drawRoundRect((x0y0 >> 16), x0y0, w, h, radius, color);
}

void mgos_ssd1306_fill_round_rect(Adafruit_SSD1306 *ssd, int x0y0, int w, int h,
                                  int radius, int color) {
  if (ssd == nullptr) return;
  ssd->fillRoundRect((x0y0 >> 16), x0y0, w, h, radius, color);
}

void mgos_ssd1306_draw_char(Adafruit_SSD1306 *ssd, int xy, int c, int color,
                            int bg, int size) {
  if (ssd == nullptr) return;
  ssd->drawChar((xy >> 16), xy, c, color, bg, size);
}

void mgos_ssd1306_set_cursor(Adafruit_SSD1306 *ssd, int x, int y) {
  if (ssd == nullptr) return;
  ssd->setCursor(x, y);
}

void mgos_ssd1306_set_text_color(Adafruit_SSD1306 *ssd, int c) {
  if (ssd == nullptr) return;
  ssd->setTextColor(c);
}

void mgos_ssd1306_set_text_color_bg(Adafruit_SSD1306 *ssd, int c, int bg) {
  if (ssd == nullptr) return;
  ssd->setTextColor(c, bg);
}

void mgos_ssd1306_set_text_size(Adafruit_SSD1306 *ssd, int s) {
  if (ssd == nullptr) return;
  ssd->setTextSize(s);
}

void mgos_ssd1306_set_text_wrap(Adafruit_SSD1306 *ssd, boolean w) {
  if (ssd == nullptr) return;
  ssd->setTextWrap(w);
}

int mgos_ssd1306_write(Adafruit_SSD1306 *ssd, const char *buffer, int size) {
  if (ssd == nullptr) return 0;
  return ssd->Print::write(buffer, size);
}

int mgos_ssd1306_height(Adafruit_SSD1306 *ssd) {
  if (ssd == nullptr) return 0;
  return ssd->height();
}

int mgos_ssd1306_width(Adafruit_SSD1306 *ssd) {
  if (ssd == nullptr) return 0;
  return ssd->width();
}

int mgos_ssd1306_get_rotation(Adafruit_SSD1306 *ssd) {
  if (ssd == nullptr) return 0;
  return ssd->getRotation();
}

void mgos_ssd1306_set_rotation(Adafruit_SSD1306 *ssd, int rotation) {
  if (ssd == nullptr) return;
  ssd->setRotation(rotation);
}

// get current cursor position (get rotation safe maximum values, using: width()
// for x, height() for y)
int mgos_ssd1306_get_cursor_x(Adafruit_SSD1306 *ssd) {
  if (ssd == nullptr) return 0;
  return ssd->getCursorX();
}

int mgos_ssd1306_get_cursor_y(Adafruit_SSD1306 *ssd) {
  if (ssd == nullptr) return 0;
  return ssd->getCursorY();
}
