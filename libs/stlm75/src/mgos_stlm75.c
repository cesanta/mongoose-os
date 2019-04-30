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

#include "mgos_stlm75.h"

#include <stdlib.h>

#define MGOS_STLM75_REG_TEMP 0
#define MGOS_STLM75_REG_CONF 1
#define MGOS_STLM75_REG_TOS 2
#define MGOS_STLM75_REG_THYS 3

static float reg_val_to_temp(uint16_t val) {
  int16_t val16 = ((int16_t) val) >> 7;
  return ((float) val16) * 0.5f;
}

static uint16_t temp_to_reg_val(float temp) {
  int16_t val16 = (int16_t)(temp / 0.5f);
  return ((uint16_t)(val16 << 7));
}

struct mgos_stlm75 *mgos_stlm75_create(struct mgos_i2c *i2c, uint16_t addr) {
  if (i2c == NULL) return NULL;
  struct mgos_stlm75 *ctx = (struct mgos_stlm75 *) calloc(1, sizeof(*ctx));
  if (ctx != NULL) mgos_stlm75_init_ctx(ctx, i2c, addr);
  return ctx;
}

void mgos_stlm75_init_ctx(struct mgos_stlm75 *ctx, struct mgos_i2c *i2c,
                          uint16_t addr) {
  ctx->i2c = i2c;
  ctx->addr = addr;
}

bool mgos_stlm75_read_temp(struct mgos_stlm75 *ctx, float *temp) {
  int val = mgos_i2c_read_reg_w(ctx->i2c, ctx->addr, MGOS_STLM75_REG_TEMP);
  if (val < 0) {
    *temp = 0;
    return false;
  }
  *temp = reg_val_to_temp(val);
  return true;
}

bool mgos_stlm75_set_alarm(struct mgos_stlm75 *ctx, float hi, float lo,
                           enum mgos_stlm75_int_mode mode,
                           enum mgos_stlm75_int_pol pol,
                           enum mgos_stlm75_int_ft ft) {
  uint8_t cfg = ((((uint8_t) ft) << 3) | (((uint8_t) pol) << 2) |
                 (((uint8_t) mode) << 1));
  if (!mgos_i2c_write_reg_w(ctx->i2c, ctx->addr, MGOS_STLM75_REG_TOS,
                            temp_to_reg_val(hi)) ||
      !mgos_i2c_write_reg_w(ctx->i2c, ctx->addr, MGOS_STLM75_REG_THYS,
                            temp_to_reg_val(lo)) ||
      !mgos_i2c_setbits_reg_b(ctx->i2c, ctx->addr, MGOS_STLM75_REG_CONF, 1, 4,
                              (cfg >> 1))) {
    return false;
  }
  return true;
}

bool mgos_stlm75_shutdown(struct mgos_stlm75 *ctx, bool shutdown) {
  return mgos_i2c_setbits_reg_b(ctx->i2c, ctx->addr, MGOS_STLM75_REG_CONF, 0, 1,
                                (uint8_t) shutdown);
}

void mgos_stlm75_free(struct mgos_stlm75 *ctx) {
  free(ctx);
}

bool mgos_stlm75_init(void) {
  return true;
}
