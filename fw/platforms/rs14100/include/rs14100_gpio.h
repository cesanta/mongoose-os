/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Pin definition, including mode.
 * Domain is HP (0), ULP (1), UULP (2)
 */
#define RS14100_PIN(domain, pin_num, mode)                            \
  (((((int) (mode)) & 0xf) << 16) | ((((int) (domain)) & 0x3) << 8) | \
   (((int) (pin_num)) & 0xff))

#define RS14100_HP 0
#define RS14100_ULP 1
#define RS14100_UULP 2

#define RS14100_HP_GPIO(n) RS14100_PIN(RS14100_HP, ((n) &0x7f), 0)
#define RS14100_ULP_GPIO(n) RS14100_PIN(RS14100_ULP, ((n) &0xf), 0)
#define RS14100_UULP_GPIO(n) RS14100_PIN(RS14100_UULP, ((n) &0x7), 0)

#define RS14100_HP_PIN_NUM(pin) ((pin) &0x7f)
#define RS14100_ULP_PIN_NUM(pin) ((pin) &0xf)
#define RS14100_UULP_PIN_NUM(pin) ((pin) &0x7)
#define RS14100_PIN_DOMAIN(pin) (((pin) >> 8) & 0x3)
#define RS14100_PIN_MODE(pin) (((pin) >> 16) & 0xf)

#ifdef __cplusplus
}
#endif
