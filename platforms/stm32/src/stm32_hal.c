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

#include "mgos_core_dump.h"
#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_sys_config.h"
#include "mgos_timers.h"
#include "mgos_utils.h"

#include "stm32_system.h"
#include "stm32_uart.h"

void mgos_dev_system_restart(void) {
  HAL_NVIC_SystemReset();
  // Not reached.
  while (1) {
  }
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

void device_set_mac_address(uint8_t mac[6]) {
  // TODO set mac address
  (void) mac;
}

void HAL_Delay(__IO uint32_t ms) {
  mgos_msleep(ms);
}

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

WWDG_HandleTypeDef hwwdg = {
    .Instance = WWDG,
    .Init =
        {
            .Prescaler = WWDG_PRESCALER_8,
            .Window = 0x7f,
            .Counter = 0x7f,
            .EWIMode = WWDG_EWI_ENABLE,
        },
};

#define WWDT_MAGIC_OFF 0xf001

static int s_wwdt_ttl = 0;
static int s_wwdt_reload = 50;

IRAM static void stm32_wwdg_int_handler(void) {
  if (s_wwdt_ttl != WWDT_MAGIC_OFF) {
    s_wwdt_ttl--;
    if ((s_wwdt_ttl & 0xffff0000) == 0) {
      WRITE_REG(hwwdg.Instance->CR, hwwdg.Init.Counter);
    } else {
      // TTL expired or has been smashed, explode.
      // Refresh one last time to get the message out.
      WRITE_REG(hwwdg.Instance->CR, hwwdg.Init.Counter);
      mgos_cd_printf("!! WDT\n");
      // Trigger core dump.
      abort();
      // Not reached.
    }
  } else {
    WRITE_REG(hwwdg.Instance->CR, hwwdg.Init.Counter);
  }
  __HAL_WWDG_CLEAR_FLAG(&hwwdg, WWDG_FLAG_EWIF);
}

void mgos_wdt_enable(void) {
  __HAL_RCC_WWDG_CLK_ENABLE();
  stm32_set_int_handler(WWDG_IRQn, stm32_wwdg_int_handler);
  HAL_NVIC_SetPriority(WWDG_IRQn, 0, 0);  // Highest possible prio.
  HAL_NVIC_EnableIRQ(WWDG_IRQn);
  HAL_WWDG_Init(&hwwdg);
}

void mgos_wdt_feed(void) {
  s_wwdt_ttl = s_wwdt_reload;
  WRITE_REG(hwwdg.Instance->CR, hwwdg.Init.Counter);
  /*
   * For backward compatibility with older bootloaders we have to feed IWDG.
   * We only do it if we detect the calue BL sets (10 seconds).
   */
  if (IWDG->RLR == 10 * 128) {
    IWDG->KR = IWDG_KEY_RELOAD;
  }
}

void mgos_wdt_set_timeout(int secs) {
  uint32_t f_wwdt = HAL_RCC_GetPCLK1Freq();  // Valid for F2, F4, F7 and L4.
  uint32_t ints_per_sec = f_wwdt / 4096 / 8 / (0x7f - 0x41);
  s_wwdt_reload = ints_per_sec * secs;
  mgos_wdt_feed();
}

void mgos_wdt_disable(void) {
  s_wwdt_ttl = WWDT_MAGIC_OFF;
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
