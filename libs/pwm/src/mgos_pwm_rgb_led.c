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

/*
 * View this file on GitHub:
 * [mgos_pwm_rgb_led.c](https://github.com/cesanta/mongoose-os/libs/pwm/blob/master/src/mgos_pwm_rgb_led.c)
 */

#include "mgos_pwm_rgb_led.h"

#include <string.h>

#include "mgos_pwm.h"

static bool mgos_pwm_rgb_led_apply(struct mgos_pwm_rgb_led *led) {
  float brv = led->br * (1.0f / 255);
  float rv = led->r * (1.0f / 255) * brv;
  float gv = led->g * (1.0f / 255) * brv;
  float bv = led->b * (1.0f / 255) * brv;
  return (mgos_pwm_set(led->gpio_r, led->freq, rv) &&
          mgos_pwm_set(led->gpio_g, led->freq, gv) &&
          mgos_pwm_set(led->gpio_b, led->freq, bv));
}

bool mgos_pwm_rgb_led_init(struct mgos_pwm_rgb_led *led, int gpio_r, int gpio_g,
                           int gpio_b) {
  memset(led, 0, sizeof(*led));
  led->gpio_r = gpio_r;
  led->gpio_g = gpio_g;
  led->gpio_b = gpio_b;
  led->freq = MGOS_PWM_RGB_LED_DEFAULT_FREQ;
  led->br = 255;
  return mgos_pwm_rgb_led_apply(led);
}

void mgos_pwm_rgb_led_set(struct mgos_pwm_rgb_led *led, uint8_t r, uint8_t g,
                          uint8_t b, uint8_t br) {
  led->r = r;
  led->g = g;
  led->b = b;
  led->br = br;
  mgos_pwm_rgb_led_apply(led);
}

void mgos_pwm_rgb_led_set_color(struct mgos_pwm_rgb_led *led, uint8_t r,
                                uint8_t g, uint8_t b) {
  mgos_pwm_rgb_led_set(led, r, g, b, led->br);
}

void mgos_pwm_rgb_led_set_color_rgb(struct mgos_pwm_rgb_led *led,
                                    uint32_t rgb) {
  mgos_pwm_rgb_led_set(led, (rgb >> 16) & 0xff, (rgb >> 8) & 0xff, (rgb & 0xff),
                       led->br);
}

void mgos_pwm_rgb_led_set_brightness(struct mgos_pwm_rgb_led *led, uint8_t br) {
  mgos_pwm_rgb_led_set(led, led->r, led->g, led->b, br);
}

bool mgos_pwm_rgb_led_set_freq(struct mgos_pwm_rgb_led *led, int freq) {
  led->freq = freq;
  return mgos_pwm_rgb_led_apply(led);
}

void mgos_pwm_rgb_led_deinit(struct mgos_pwm_rgb_led *led) {
  mgos_pwm_set(led->gpio_r, 0, 0);
  mgos_pwm_set(led->gpio_g, 0, 0);
  mgos_pwm_set(led->gpio_b, 0, 0);
}
