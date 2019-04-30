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

#include "mgos_bm222.h"

#include "mgos.h"
#include "mgos_i2c.h"

#define BM222_REG_PMU_BW 0x10
#define BM222_PMU_BW_7_81_HZ 0x8
#define BM222_PMU_BW_31_25_HZ 0xA
#define BM222_PMU_BW_62_5_HZ 0xB
#define BM222_PMU_BW_125_HZ 0xC

#define BM222_REG_BGW_SOFTRESET 0x14
#define BM222_DO_SOFT_RESET 0xB6

#define BM222_REG_FIFO_CONFIG_1 0x3e
#define BM222_FIFO_MODE_BYPASS 0x00
#define BM222_REG_FIFO_STATUS 0x0e
#define BM222_REG_FIFO_DATA 0x3f

bool bm222_init(struct mgos_i2c *i2c, uint8_t addr) {
  uint8_t val1[] = {BM222_REG_BGW_SOFTRESET, BM222_DO_SOFT_RESET};
  uint8_t val2[] = {BM222_REG_FIFO_CONFIG_1, BM222_FIFO_MODE_BYPASS};
  uint8_t val3[] = {BM222_REG_PMU_BW, BM222_PMU_BW_125_HZ};

  if (!mgos_i2c_write(i2c, addr, val1, sizeof(val1), true)) {
    LOG(LL_ERROR, ("Cannot reset, addr %x", addr));
    return false;
  }
  mgos_usleep(2000);

  if (!mgos_i2c_write(i2c, addr, val2, sizeof(val2), true)) {
    LOG(LL_ERROR, ("Cannot init fifo, addr %x", addr));
    return false;
  }
  if (!mgos_i2c_write(i2c, addr, val3, sizeof(val3), true)) {
    LOG(LL_ERROR, ("Cannot set PMU, addr %x", addr));
    return false;
  }
  return true;
}

bool bm222_read(struct mgos_i2c *i2c, uint8_t addr, int *x, int *y, int *z) {
  int val = mgos_i2c_read_reg_b(i2c, addr, BM222_REG_FIFO_STATUS);
  if (val < 0 || (val & 0x7f) < 1) {
    LOG(LL_ERROR, ("Error reading fifo status, addr %x, val %d", addr, val));
    return false;
  }
  LOG(LL_DEBUG, ("FIFO len: %d (ovf? %d)", val & 0x7f, val & 0x80));
  uint8_t buf[6];
  if (!mgos_i2c_read_reg_n(i2c, addr, BM222_REG_FIFO_DATA, sizeof(buf), buf)) {
    LOG(LL_ERROR, ("Cannot read fifo, addr %x", addr));
    return false;
  }
  if (x != NULL) *x = buf[0] | (buf[1] << 8);
  if (y != NULL) *y = buf[2] | (buf[3] << 8);
  if (z != NULL) *z = buf[4] | (buf[5] << 8);
  return true;
}

bool mgos_bm222_init(void) {
  return true;
}
