/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CS_MOS_LIBS_ARDUINO_ADAFRUIT_SSD1306_SRC_MGOS_ARDUINO_SSD1306_H_
#define CS_MOS_LIBS_ARDUINO_ADAFRUIT_SSD1306_SRC_MGOS_ARDUINO_SSD1306_H_

#include "Adafruit_SSD1306.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Resolution of SSD1306 display */
enum mgos_ssd1306_res {
  MGOS_SSD1306_RES_96_16,
  MGOS_SSD1306_RES_128_32,
  MGOS_SSD1306_RES_128_64,
};

/*
 * Create and return an Adafruit_SSD1306 instance for I2C.
 * `rst` is a reset pin, `res` is a resolution, one of the following:
 * - `MGOS_SSD1306_RES_96_16`
 * - `MGOS_SSD1306_RES_128_32`
 * - `MGOS_SSD1306_RES_128_64`
 */
Adafruit_SSD1306 *mgos_ssd1306_create_i2c(int rst, int res);

/*
 * Create and return an Adafruit_SSD1306 instance for SPI.
 * `rst` is a reset pin, `res` is a resolution, one of the following:
 * - `MGOS_SSD1306_RES_96_16`
 * - `MGOS_SSD1306_RES_128_32`
 * - `MGOS_SSD1306_RES_128_64`
 */
Adafruit_SSD1306 *mgos_ssd1306_create_spi(int dc, int rst, int cs, int res);

/*
 * Close Adafruit_SSD1306 indstance.
 */
void mgos_ssd1306_close(Adafruit_SSD1306 *ssd);

/*
 * Initialize the display. `vccstate` is a VCC state, one of those:
 * - `SSD1306_EXTERNALVCC`
 * - `SSD1306_SWITCHCAPVCC`
 * `i2caddr` is an I2C address (ignored if `create_spi` was used).
 * If `reset` is true, then the display controller will be reset.
 * Example:
 * ```javascript
 * mySSD1306 = mgos_ssd1306_create_i2c(12, MGOS_SSD1306_RES_96_16);
 * mgos_ssd1306_begin(mySSD1306, SSD1306_EXTERNALVCC, 0x42, true);
 * ```
 */
void mgos_ssd1306_begin(Adafruit_SSD1306 *ssd, int vccstate, int i2caddr,
                        bool reset);

/*
 * Send an arbitrary command `cmd`, which must be a number from 0 to 255.
 */
void mgos_ssd1306_command(Adafruit_SSD1306 *ssd, int c);

/*
 * Clear display.
 */
void mgos_ssd1306_clear_display(Adafruit_SSD1306 *ssd);

/*
 * Set invert mode: 0 - don't invert; 1 - invert.
 */
void mgos_ssd1306_invert_display(Adafruit_SSD1306 *ssd, int i);

/*
 * Put image data to the display.
 */
void mgos_ssd1306_display(Adafruit_SSD1306 *ssd);

/*
 * Activate a right handed scroll for rows `start` through `stop`.
 */
void mgos_ssd1306_start_scroll_right(Adafruit_SSD1306 *ssd, int start,
                                     int stop);

/*
 * Activate a left handed scroll for rows `start` through `stop`.
 */
void mgos_ssd1306_start_scroll_left(Adafruit_SSD1306 *ssd, int start, int stop);

/*
 * Activate a diagonal scroll for rows `start` through `stop`.
 */
void mgos_ssd1306_start_scroll_diag_right(Adafruit_SSD1306 *ssd, int start,
                                          int stop);

/*
 * Activate a diagonal scroll for rows `start` through `stop`.
 */
void mgos_ssd1306_start_scroll_diag_left(Adafruit_SSD1306 *ssd, int start,
                                         int stop);

/*
 * Stop scroll.
 */
void mgos_ssd1306_stop_scroll(Adafruit_SSD1306 *ssd);

/*
 * Set dim mode:
 * dim is 1: display is dimmed;
 * dim is 0: display is normal.
 */
void mgos_ssd1306_dim(Adafruit_SSD1306 *ssd, int dim);

/*
 * Set a single pixel at `x`, `y` to have the given `color`, the color being
 * one of the following: `BLACK`, `WHITE`, `INVERSE`.
 */
void mgos_ssd1306_draw_pixel(Adafruit_SSD1306 *ssd, int x, int y, int color);

/*
 * Draw a vertical line with height `h`, starting at `x`, `y`, with color
 * `color` (`BLACK`, `WHITE` or `INVERSE`)
 */
void mgos_ssd1306_draw_fast_vline(Adafruit_SSD1306 *ssd, int x, int y, int h,
                                  int color);

/*
 * Draw a horizontal line with width `w`, starting at `x`, `y`, with color
 * `color` (`BLACK`, `WHITE` or `INVERSE`)
 */
void mgos_ssd1306_draw_fast_hline(Adafruit_SSD1306 *ssd, int x, int y, int w,
                                  int color);

/*
 * Adafruit_GFX
 */
int mgos_ssd1306_make_xy_pair(int x, int y);

void mgos_ssd1306_draw_circle(Adafruit_SSD1306 *ssd, int x0y0, int r,
                              int color);

void mgos_ssd1306_draw_circle_helper(Adafruit_SSD1306 *ssd, int x0y0, int r,
                                     int cornername, int color);

void mgos_ssd1306_fill_circle(Adafruit_SSD1306 *ssd, int x0y0, int r,
                              int color);

void mgos_ssd1306_fill_circle_helper(Adafruit_SSD1306 *ssd, int x0y0, int r,
                                     int cornername, int delta, int color);

void mgos_ssd1306_draw_triangle(Adafruit_SSD1306 *ssd, int x0y0, int x1y1,
                                int x2y2, int color);

void mgos_ssd1306_fill_triangle(Adafruit_SSD1306 *ssd, int x0y0, int x1y1,
                                int x2y2, int color);

void mgos_ssd1306_draw_round_rect(Adafruit_SSD1306 *ssd, int x0y0, int w, int h,
                                  int radius, int color);

void mgos_ssd1306_fill_round_rect(Adafruit_SSD1306 *ssd, int x0y0, int w, int h,
                                  int radius, int color);

void mgos_ssd1306_draw_char(Adafruit_SSD1306 *ssd, int xy, int c, int color,
                            int bg, int size);

void mgos_ssd1306_set_cursor(Adafruit_SSD1306 *ssd, int x, int y);

void mgos_ssd1306_set_text_color(Adafruit_SSD1306 *ssd, int c);
void mgos_ssd1306_set_text_color_bg(Adafruit_SSD1306 *ssd, int c, int bg);
void mgos_ssd1306_set_text_size(Adafruit_SSD1306 *ssd, int s);
void mgos_ssd1306_set_text_wrap(Adafruit_SSD1306 *ssd, boolean w);

int mgos_ssd1306_write(Adafruit_SSD1306 *ssd, const char *buffer, int size);

int mgos_ssd1306_height(Adafruit_SSD1306 *ssd);
int mgos_ssd1306_width(Adafruit_SSD1306 *ssd);

int mgos_ssd1306_get_rotation(Adafruit_SSD1306 *ssd);
void mgos_ssd1306_set_rotation(Adafruit_SSD1306 *ssd, int rotation);

/*
 * get current cursor position (get rotation safe maximum values, using: width()
 * for x, height() for y)
 */
int mgos_ssd1306_get_cursor_x(Adafruit_SSD1306 *ssd);
int mgos_ssd1306_get_cursor_y(Adafruit_SSD1306 *ssd);

#ifdef __cplusplus
}
#endif

#endif /* CS_MOS_LIBS_ARDUINO_ADAFRUIT_SSD1306_SRC_MGOS_ARDUINO_SSD1306_H_ */
