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
 * PWM-controlled RGB LED.
 * Example:
 *   struct mgos_pwm_rgb_led led;
 *   mgos_pwm_rgb_led_init(&led, 16, 17, 18);
 *   mgos_pwm_rgb_led_set(&led, 255, 255, 255, 255);  // White, max brightness
 *   mgos_pwm_rgb_led_set(&led, 255,   0,   0, 127);  // Red, half brightness
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mgos_pwm_rgb_led {
  int freq;
  uint8_t r, g, b, br; /* Current values or R, G, B and brightness. */
  int gpio_r, gpio_g, gpio_b;
};

#define MGOS_PWM_RGB_LED_DEFAULT_FREQ 400 /* Hz */

/* Init the LED pins. */
bool mgos_pwm_rgb_led_init(struct mgos_pwm_rgb_led *led, int gpio_r, int gpio_g,
                           int gpio_b);

/* Set color and brightenss, 1-255. */
void mgos_pwm_rgb_led_set(struct mgos_pwm_rgb_led *led, uint8_t r, uint8_t g,
                          uint8_t b, uint8_t br);

/* Set color. */
void mgos_pwm_rgb_led_set_color(struct mgos_pwm_rgb_led *led, uint8_t r,
                                uint8_t g, uint8_t b);

/* Set color as a 24-bit RGB value. */
void mgos_pwm_rgb_led_set_color_rgb(struct mgos_pwm_rgb_led *led, uint32_t rgb);

/* Set brightness. */
void mgos_pwm_rgb_led_set_brightness(struct mgos_pwm_rgb_led *led, uint8_t br);

/* Set PWM frequency. */
bool mgos_pwm_rgb_led_set_freq(struct mgos_pwm_rgb_led *led, int freq);

/* Disable PWM and release the pins. */
void mgos_pwm_rgb_led_deinit(struct mgos_pwm_rgb_led *led);

#ifdef __cplusplus
}
#endif /* __cplusplus */
