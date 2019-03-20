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

#include <sys/time.h>

#include <stm32_sdk_hal.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "common/cs_dbg.h"

#include "mongoose.h"

#include "mgos_hal.h"
#include "mgos_sys_config.h"
#include "mgos_mongoose.h"
#include "mgos_timers.h"
#include "mgos_utils.h"

#include "stm32_uart.h"

void mgos_dev_system_restart(void) {
  HAL_NVIC_SystemReset();
}

void device_get_mac_address(uint8_t mac[6]) {
  static uint8_t s_dev_mac[6] = {0};
  if (s_dev_mac[0] != 0) {
    memcpy(mac, s_dev_mac, 6);
    return;
  }
  /*
   * Construct MAC address by using a Locally Administered Address OUI 12:34:...
   * and a unique suffix obtained from hashing the device's UID.
   */
  uint32_t uid[3] = {
      READ_REG(*((uint32_t *) UID_BASE)),
      READ_REG(*((uint32_t *) UID_BASE + 4)),
      READ_REG(*((uint32_t *) UID_BASE + 8)),
  };
  uint8_t digest[20];
  const uint8_t *msgs[1] = {(const uint8_t *) &uid[0]};
  size_t lens[1] = {sizeof(uid)};
  mg_hash_sha1_v(1, msgs, lens, digest);
  memcpy(&s_dev_mac[2], digest, 4);
  s_dev_mac[1] = 0x34;
  s_dev_mac[0] = 0x12;
  memcpy(mac, s_dev_mac, 6);
}

void mgos_msleep(uint32_t msecs) {
  mgos_usleep(msecs * 1000);
}

void HAL_Delay(__IO uint32_t ms) __attribute__((alias("mgos_msleep")));

/* Note: PLL must be enabled for RNG to work */
int mg_ssl_if_mbed_random(void *ctx, unsigned char *buf, size_t len) {
  RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
  RNG->CR = RNG_CR_RNGEN;
  size_t i = 0, j = 0;
  do {
    if (RNG->SR & RNG_SR_DRDY) {
      uint32_t rnd = RNG->DR;
      size_t l = MIN(len - i, sizeof(rnd));
      memcpy(buf + i, &rnd, l);
      i += l;
    } else {
      j++;
      if (j > 1000) {
        LOG(LL_ERROR,
            ("RNG is not working! ST 0x%lx CR 0x%lx", RNG->SR, RNG->CR));
        return -1;
      }
    }
  } while (i < len);
  (void) ctx;
  return 0;
}

uint32_t HAL_GetTick(void) {
  /* Overflow? Meh. HAL doesn't seem to care. */
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

/* LwIP time function, returns timestamp in milliseconds. */
uint32_t sys_now(void) {
  return HAL_GetTick();
}

uint32_t sys_jiffies(void) {
  return sys_now();
}

#define IWDG_1_SECOND 128
IWDG_HandleTypeDef hiwdg = {
    .Instance = IWDG,
    .Init =
        {
         .Prescaler = IWDG_PRESCALER_256,
         .Reload = 5 * IWDG_1_SECOND,
#ifdef IWDG_WINDOW_DISABLE
         .Window = IWDG_WINDOW_DISABLE,
#endif
        },
};

void mgos_wdt_enable(void) {
  HAL_IWDG_Init(&hiwdg);
}

void mgos_wdt_feed(void) {
  HAL_IWDG_Refresh(&hiwdg);
}

void mgos_wdt_set_timeout(int secs) {
  uint32_t new_reload = (secs * IWDG_1_SECOND);
  if (!IS_IWDG_RELOAD(new_reload)) {
    LOG(LL_ERROR, ("Invalid WDT reload value %lu", new_reload));
    return;
  }
  hiwdg.Init.Reload = new_reload;
  HAL_IWDG_Init(&hiwdg);
}

void mgos_wdt_disable(void) {
  static bool printed = false;
  if (!printed) {
    printed = true;
    LOG(LL_ERROR, ("Once enabled, WDT cannot be disabled!"));
  }
}

uint32_t mgos_get_cpu_freq(void) {
  return SystemCoreClock;
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
