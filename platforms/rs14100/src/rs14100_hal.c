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

#include "mgos.h"
#include "mgos_hal.h"

#include "mongoose.h"

#include "rs14100_sdk.h"

struct rs14100_bg_scan_params g_wifi_sta_bg_scan_params;
struct rs14100_roaming_params g_wifi_sta_roaming_params;

uint32_t mgos_bitbang_n100_cal = 0;

uint32_t mgos_get_cpu_freq(void) {
  return SystemCoreClock;
}

int mg_ssl_if_mbed_random(void *ctx, unsigned char *buf, size_t len) {
  uint32_t rnd;
  if ((HWRNG->HWRNG_CTRL_REG & 0x3) == 0) {
    RSI_CLK_PeripheralClkEnable(M4CLK, HWRNG_CLK, ENABLE_STATIC_CLK);
    HWRNG->HWRNG_CTRL_REG = 1; /* Enable, true random mode */
    // Wait for RNG to start up.
    for (rnd = HWRNG->HWRNG_RAND_NUM_REG; HWRNG->HWRNG_RAND_NUM_REG == rnd;) {
    }
  }
  size_t i = 0;
  do {
    uint32_t rnd = HWRNG->HWRNG_RAND_NUM_REG;
    size_t l = MIN(len - i, sizeof(rnd));
    memcpy(buf + i, &rnd, l);
    i += l;
  } while (i < len);
  (void) ctx;
  return 0;
}

uint32_t rs14100_get_lf_fsm_clk(void) {
  switch (MCU_AON->MCUAON_KHZ_CLK_SEL_POR_RESET_STATUS_b.AON_KHZ_CLK_SEL) {
    case 1:
      return 32000;
    case 2:
      return 32000;
    case 4:
      return 32768;
  }
  return 0;
}

#if !MG_LWIP
uint32_t swap_bytes_32(uint32_t x) {
  return (((x & 0xff) << 24) | ((x & 0xff00) << 8) | ((x & 0xff0000) >> 8) |
          ((x & 0xff000000) >> 24));
}

uint16_t swap_bytes_16(uint16_t x) {
  return ((x << 8) | (x >> 8));
}

const char *inet_ntop(int af, const void *src, char *dst, int size) {
  switch (af) {
    case AF_INET: {
      if (size < 16) return NULL;
      uint32_t a = (((struct in_addr *) src)->s_addr);
      sprintf(dst, "%lu.%lu.%lu.%lu", (a & 0xff), ((a >> 8) & 0xff),
              ((a >> 16) & 0xff), ((a >> 24) & 0xff));
      break;
    }
#if 0
    case AF_INET6: {
      if (size < 48) return NULL;
      sprintf(dst, "...");
      break;
    }
#endif
    default:
      return NULL;
  }
  return dst;
}

char *inet_ntoa(struct in_addr in) {
  static char str[16];
  return (char *) inet_ntop(AF_INET, &in, str, sizeof(str));
}

#endif
