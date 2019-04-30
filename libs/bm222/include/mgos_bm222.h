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
 * BM222 accelerometer API.
 *
 * See https://www.bosch-sensortec.com/bst/products/all_products/bma222e
 * for more information.
 */

#ifndef CS_MOS_LIBS_BM222_SRC_MGOS_BM222_H_
#define CS_MOS_LIBS_BM222_SRC_MGOS_BM222_H_

#include <stdbool.h>
#include <stdint.h>

#include "mgos_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// Open BM222 sensor on a given I2C address.
// Return true on success, false on error.
bool bm222_init(struct mgos_i2c *i2c, uint8_t addr);

// Read sensor values. Return true on success, false on error.
bool bm222_read(struct mgos_i2c *i2c, uint8_t addr, int *x, int *y, int *z);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_BM222_SRC_MGOS_BM222_H_ */
