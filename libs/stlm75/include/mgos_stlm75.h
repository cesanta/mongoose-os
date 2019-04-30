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

#include <stdbool.h>
#include <stdint.h>

#include "mgos_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// Lower 3 bits are determined by A2,A1,A0 pins, hence 0x48-0x4f are possible.
#define MGOS_STLM75_ADDR_000 0x48
#define MGOS_STLM75_ADDR_001 0x49
#define MGOS_STLM75_ADDR_010 0x4a
#define MGOS_STLM75_ADDR_011 0x4b
#define MGOS_STLM75_ADDR_100 0x4c
#define MGOS_STLM75_ADDR_101 0x4d
#define MGOS_STLM75_ADDR_110 0x4e
#define MGOS_STLM75_ADDR_111 0x4f

struct mgos_stlm75 {
  struct mgos_i2c *i2c;
  uint16_t addr;
};

struct mgos_stlm75 *mgos_stlm75_create(struct mgos_i2c *i2c, uint16_t addr);

void mgos_stlm75_init_ctx(struct mgos_stlm75 *ctx, struct mgos_i2c *i2c,
                          uint16_t addr);

bool mgos_stlm75_read_temp(struct mgos_stlm75 *ctx, float *temp);

// Interrup vs comparator mode. See datasheet for detailed explanation.
enum mgos_stlm75_int_mode {
  MGOS_STLM75_INT_MODE_COMP = 0,
  MGOS_STLM75_INT_MODE_INT = 1,
};

// Interrupt active polarity.
enum mgos_stlm75_int_pol {
  MGOS_STLM75_INT_POL_NEG = 0,
  MGOS_STLM75_INT_POL_POS = 1,
};

// "Fault tolerance" - the temperature value must exceed threshold
// for 1, 2, 4 or 6 cycles before interrupt is asserted.
enum mgos_stlm75_int_ft {
  MGOS_STLM75_INT_FT_1 = 0,
  MGOS_STLM75_INT_FT_2 = 1,
  MGOS_STLM75_INT_FT_4 = 2,
  MGOS_STLM75_INT_FT_6 = 3,
};

bool mgos_stlm75_set_alarm(struct mgos_stlm75 *ctx, float hi, float lo,
                           enum mgos_stlm75_int_mode mode,
                           enum mgos_stlm75_int_pol pol,
                           enum mgos_stlm75_int_ft ft);

// Put the device in or out of shutdown.
bool mgos_stlm75_shutdown(struct mgos_stlm75 *ctx, bool shutdown);

void mgos_stlm75_free(struct mgos_stlm75 *ctx);

#ifdef __cplusplus
}
#endif
