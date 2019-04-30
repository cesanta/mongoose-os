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

#ifndef CS_MOS_LIBS_NEOPIXEL_INCLUDE_MGOS_NEOPIXEL_H_
#define CS_MOS_LIBS_NEOPIXEL_INCLUDE_MGOS_NEOPIXEL_H_

#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {  // Make sure we have C-declarations in C++ programs
#endif

/*
 * Pixel order: RGB, GRB, or BGR.
 */
enum mgos_neopixel_order {
  MGOS_NEOPIXEL_ORDER_RGB,
  MGOS_NEOPIXEL_ORDER_GRB,
  MGOS_NEOPIXEL_ORDER_BGR,
};

/*
 * Opaque neopixel instance
 */
struct mgos_neopixel;

/*
 * Create and return a NeoPixel strip object. Example:
 * ```c
 * struct mgos_neopixel *mystrip = mgos_neopixel_create(
 *     5, 16, MGOS_NEOPIXEL_ORDER_GRB);
 * mgos_neopixel_set(mystrip, 0, 12, 34, 56);
 * mgos_neopixel_show(mystrip);
 *
 * mgos_neopixel_clear(mystrip);
 * mgos_neopixel_set(mystrip, 1, 12, 34, 56);
 * mgos_neopixel_show(mystrip);
 * ```
 */
struct mgos_neopixel *mgos_neopixel_create(int pin, int num_pixels,
                                           enum mgos_neopixel_order order);

/*
 * Set i-th pixel's RGB value. Each color (`r`, `g`, `b`) should be an integer
 * from 0 to 255; they are ints and not `uint8_t`s just for the FFI.
 *
 * Note that this only affects in-memory value of the pixel; you'll need to
 * call `mgos_neopixel_show()` to apply changes.
 */
void mgos_neopixel_set(struct mgos_neopixel *np, int i, int r, int g, int b);

/*
 * Clear in-memory values of the pixels.
 */
void mgos_neopixel_clear(struct mgos_neopixel *np);

/*
 * Output values of the pixels.
 */
void mgos_neopixel_show(struct mgos_neopixel *np);

/*
 * Free neopixel instance.
 */
void mgos_neopixel_free(struct mgos_neopixel *np);

#if defined(__cplusplus)
}
#endif

#endif /* CS_MOS_LIBS_NEOPIXEL_INCLUDE_MGOS_NEOPIXEL_H_ */
