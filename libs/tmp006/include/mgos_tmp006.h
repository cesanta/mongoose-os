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

#ifndef CS_MOS_LIBS_TMP006_SRC_MGOS_TMP006_H_
#define CS_MOS_LIBS_TMP006_SRC_MGOS_TMP006_H_

#include <stdint.h>
#include <stdbool.h>

#include "mgos_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Conversion rate for the TMP006 sensor, see `mgos_tmp006_setup()`.
 */
enum tmp006_conversion_rate {
  TMP006_CONV_4 = 0,
  TMP006_CONV_2,
  TMP006_CONV_1,
  TMP006_CONV_1_2,
  TMP006_CONV_1_4,
};

/*
 * Invalid voltage or temperature value, see `mgos_tmp006_get_voltage()` and
 * `mgos_tmp006_get_die_temp()`
 */
#define TMP006_INVALID_READING (-1000.0)

/*
 * Initialize TMP006 driver on the given I2C `bus` and `addr`, with
 * the given conversion rate (see `enum tmp006_conversion_rate`). To enable
 * `DRDY` pin, set `drdy_en` to `true`.
 *
 * Returns `true` in case of success, `false` otherwise.
 */
bool mgos_tmp006_setup(struct mgos_i2c *bus, uint8_t addr,
                       enum tmp006_conversion_rate rate, bool drdy_en);

/*
 * Get voltage from the TMP006 sensor at the given i2c bus and addr. In case of
 * failure, returns `TMP006_INVALID_READING`.
 */
double mgos_tmp006_get_voltage(struct mgos_i2c *i2c, uint8_t addr);

/*
 * Get temperature in C degrees from the TMP006 sensor at the given i2c bus and
 * addr. In case of failure, returns `TMP006_INVALID_READING`.
 */
double mgos_tmp006_get_die_temp(struct mgos_i2c *i2c, uint8_t addr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_TMP006_SRC_MGOS_TMP006_H_ */
