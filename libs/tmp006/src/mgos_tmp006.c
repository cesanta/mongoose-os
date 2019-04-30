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

#include "mgos_tmp006.h"

#include "mgos.h"
#include "mgos_i2c.h"

#define TMP006_REG_SENSOR_VOLTAGE 0x00
#define TMP006_REG_DIE_TEMP 0x01
#define TMP006_REG_CONFIG 0x02

bool mgos_tmp006_setup(struct mgos_i2c *bus, uint8_t addr,
                       enum tmp006_conversion_rate conv_rate, bool drdy_en) {
  uint8_t val[3] = {TMP006_REG_CONFIG, 0x80, 0};
  /* Reset first. */
  if (!mgos_i2c_write(bus, addr, val, sizeof(val), true)) {
    LOG(LL_ERROR, ("Cannot reset, addr %x", addr));
    return false;
  }
  val[1] = 0x70 | (conv_rate << 1) | drdy_en;
  /* Configure. */
  return mgos_i2c_write(bus, addr, val, sizeof(val), true);
}

double mgos_tmp006_get_voltage(struct mgos_i2c *bus, uint8_t addr) {
  int v = mgos_i2c_read_reg_w(bus, addr, TMP006_REG_SENSOR_VOLTAGE);
  int voltage = (v & 0x80) ? -((1 << 16) - v) : v;
  LOG(LL_DEBUG, ("%d", v));
  return v < 0 ? TMP006_INVALID_READING : voltage * 0.00015625;
}

double mgos_tmp006_get_die_temp(struct mgos_i2c *bus, uint8_t addr) {
  int v = mgos_i2c_read_reg_w(bus, addr, TMP006_REG_DIE_TEMP);
  double temp = (double) (v >> 2) / 32.0;
  LOG(LL_DEBUG, ("%d, %.2f", v, temp));
  return v < 0 ? TMP006_INVALID_READING : temp;
}

bool mgos_tmp006_init(void) {
  return true;
}
