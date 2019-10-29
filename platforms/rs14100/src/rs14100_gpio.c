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

#include "rs14100_gpio.h"

#include <stdlib.h>

#include "common/cs_dbg.h"

#include "mgos_gpio_hal.h"

#include "rs14100_sdk.h"

#define GPIO_25_30_CONFIG_REG *((volatile uint32_t *) (MISC_CONFIG_BASE + 0xc))

static struct {
  uint8_t hp[64];
  uint8_t ulp[16];
  uint8_t uulp[5];
} s_rs14100_gpio_od;

const char *mgos_gpio_str(int pin, char buf[8]) {
  int i = 0;
  if (pin >= 0) {
    switch (RS14100_PIN_DOMAIN(pin)) {
      case RS14100_HP:
        break;
      case RS14100_UULP:
        buf[i++] = 'U';
      // fallthrough
      case RS14100_ULP:
        buf[i++] = 'U';
        break;
      default:
        break;
    }
    int pin_num = RS14100_HP_PIN_NUM(pin);
    if (pin_num < 10) {
      buf[i++] = '0' + pin_num;
    } else {
      buf[i++] = '0' + (pin_num / 10);
      buf[i++] = '0' + (pin_num % 10);
    }
  } else {
    buf[i++] = '-';
  }
  buf[i++] = '\0';
  return buf;
}

IRAM bool mgos_gpio_read(int pin) {
  int pin_num;
  volatile EGPIO_Type *regs;
  switch (RS14100_PIN_DOMAIN(pin)) {
    case RS14100_HP: {
      regs = EGPIO;
      pin_num = RS14100_HP_PIN_NUM(pin);
      break;
    }
    case RS14100_ULP: {
      regs = EGPIO1;
      pin_num = RS14100_ULP_PIN_NUM(pin);
      break;
    }
    case RS14100_UULP: {
      pin_num = RS14100_UULP_PIN_NUM(pin);
      return RSI_NPSSGPIO_GetPin(pin_num);
    }
    default:
      return false;
  }
  int port_num = (pin_num / 16);
  uint32_t pin_mask = (1 << (pin_num % 16));
  return ((regs->PORT_CONFIG[port_num].PORT_READ_REG & pin_mask) != 0);
}

IRAM bool mgos_gpio_read_out(int pin) {
  int pin_num;
  volatile EGPIO_Type *regs;
  switch (RS14100_PIN_DOMAIN(pin)) {
    case RS14100_HP: {
      regs = EGPIO;
      pin_num = RS14100_HP_PIN_NUM(pin);
      break;
    }
    case RS14100_ULP: {
      regs = EGPIO1;
      pin_num = RS14100_ULP_PIN_NUM(pin);
      break;
    }
    case RS14100_UULP: {
      pin_num = RS14100_UULP_PIN_NUM(pin);
      return MCU_RET->NPSS_GPIO_CNTRL[pin_num].NPSS_GPIO_CTRLS_b.NPSS_GPIO_OUT;
    }
    default:
      return false;
  }
  int port_num = (pin_num / 16);
  uint32_t pin_mask = (1 << (pin_num % 16));
  return ((regs->PORT_CONFIG[port_num].PORT_LOAD_REG & pin_mask) != 0);
}

IRAM void mgos_gpio_write(int pin, bool level) {
  int pin_num, od;
  volatile EGPIO_Type *regs;
  switch (RS14100_PIN_DOMAIN(pin)) {
    case RS14100_HP: {
      regs = EGPIO;
      pin_num = RS14100_HP_PIN_NUM(pin);
      od = s_rs14100_gpio_od.hp[pin_num];
      break;
    }
    case RS14100_ULP: {
      regs = EGPIO1;
      pin_num = RS14100_ULP_PIN_NUM(pin);
      od = s_rs14100_gpio_od.ulp[pin_num];
      break;
    }
    case RS14100_UULP: {
      pin_num = RS14100_UULP_PIN_NUM(pin);
      if (pin_num > 4) return;
      RSI_NPSSGPIO_SetPin(pin_num, level);
      if (s_rs14100_gpio_od.uulp[pin_num]) {
        RSI_NPSSGPIO_SetDir(pin_num, level);
      }
      return;
    }
    default:
      return;
  }
  regs->PIN_CONFIG[pin_num].BIT_LOAD_REG = level;
  if (od) {
    regs->PIN_CONFIG[pin_num].GPIO_CONFIG_REG_b.DIRECTION = level;
  }
}

// According to HRM 10.3.2.1 PAD Configuration
static void rs14100_gpio_claim_pad(int n) {
  if (n >= 25 && n <= 30) {
    /* Something is missing here, GPIO 25-30 do not work. TODO(rojer): Fix. */
    SDIO_CNTD_TO_TASS = (1 << 5);
    GPIO_25_30_CONFIG_REG |= (1 << 10);
    RSI_EGPIO_HostPadsGpioModeEnable(n);
  } else {
    static const uint8_t s_pin_to_padsel_bit_no[64] = {
        0,  0,  0,  0,  0,  0,   // 0-5 - always NWP (flash)
        1,  1,  1,  1,           // 6:9
        2,  2,                   // 10,11
        3,                       // 12
        4,  4,                   // 13,14
        5,  5,  5,               // 15:17
        6,  6,                   // 18,19
        7,                       // 20
        0,  0,  0,  0,           // 21:24 - always HP
        0,  0,  0,  0,  0,  0,   // 25:30 - CTRL2
        8,  8,  8,  8,           // 31:34
        9,  9,  9,               // 35:37
        0,  0,  0,  0,           // 38:41 - always HP
        10, 10, 10, 10,          // 42:45
        11, 11, 11,              // 46:48
        12, 12, 12,              // 49:51
        13, 13, 13, 13,          // 52:55
        14, 14,                  // 56,57
        15, 15, 15, 15, 15, 15,  // 58:63
    };
    uint8_t padsel_bit_no = s_pin_to_padsel_bit_no[n];
    if (padsel_bit_no > 0) {
      PADSELECTION |= (1 << padsel_bit_no);
    }
  }
}

static bool rs14100_gpio_clk_en(int pin) {
  switch (RS14100_PIN_DOMAIN(pin)) {
    case RS14100_HP: {
      RSI_CLK_PeripheralClkEnable(M4CLK, EGPIO_CLK, ENABLE_STATIC_CLK);
      break;
    }
    case RS14100_ULP: {
      RSI_ULPSS_PeripheralEnable(ULPCLK, ULP_EGPIO_CLK, ENABLE_STATIC_CLK);
      break;
    }
    case RS14100_UULP: {
      // Nothing to do, it's always on.
      break;
    }
    default:
      return false;
  }
  return true;
}

bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode) {
  int pin_num, pin_mode = RS14100_PIN_MODE(pin);
  bool oen, ren, od;
  if (!rs14100_gpio_clk_en(pin)) return false;
  // OEN=0 enables output driver, REN=1 enables the receiver.
  ren = (mode == MGOS_GPIO_MODE_INPUT);
  od = (mode == MGOS_GPIO_MODE_OUTPUT_OD);
  if (od) {
    // OD output is implemented by manipulating OEN (per HRM 16.6.3.2):
    // to output 1, output driver is disabled (OEN=1),
    // to output 0 driver is enabled (OEN=0).
    oen = mgos_gpio_read_out(pin);
  } else {
    oen = !(mode == MGOS_GPIO_MODE_OUTPUT);
  }
  switch (RS14100_PIN_DOMAIN(pin)) {
    case RS14100_HP: {
      pin_num = RS14100_HP_PIN_NUM(pin);
      if (pin_num <= 5 || pin_num > 79) return false;
      if (pin_num < 64) rs14100_gpio_claim_pad(pin_num);
      EGPIO->PIN_CONFIG[pin_num].GPIO_CONFIG_REG_b.MODE = pin_mode;
      if (pin_num > 64) {
        // Set SOC_GPIO mode for ULP-controlled pins.
        EGPIO1->PIN_CONFIG[pin_num - 64].GPIO_CONFIG_REG_b.MODE = 6;
      }
      EGPIO->PIN_CONFIG[pin_num].GPIO_CONFIG_REG_b.DIRECTION = oen;
      if (ren) {
        RSI_EGPIO_PadReceiverEnable(pin_num);
      } else {
        RSI_EGPIO_PadReceiverDisable(pin_num);
      }
      s_rs14100_gpio_od.hp[pin_num] = od;
      break;
    }
    case RS14100_ULP: {
      pin_num = RS14100_ULP_PIN_NUM(pin);
      if (pin_num < 0 || pin_num > 15) return false;
      EGPIO1->PIN_CONFIG[pin_num].GPIO_CONFIG_REG_b.MODE = pin_mode;
      EGPIO1->PIN_CONFIG[pin_num].GPIO_CONFIG_REG_b.DIRECTION = oen;
      if (ren) {
        RSI_EGPIO_UlpPadReceiverEnable(pin_num);
      } else {
        RSI_EGPIO_UlpPadReceiverDisable(pin_num);
      }
      s_rs14100_gpio_od.ulp[pin_num] = od;
      break;
    }
    case RS14100_UULP: {
      pin_num = RS14100_UULP_PIN_NUM(pin);
      if (pin_num < 0 || pin_num > 4) return false;
      RSI_NPSSGPIO_SetDir(pin_num, oen);
      RSI_NPSSGPIO_InputBufferEn(pin_num, ren);
      RSI_NPSSGPIO_SetPinMux(pin_num, pin_mode);
      s_rs14100_gpio_od.uulp[pin_num] = od;
      break;
    }
    default:
      return false;
  }
  return true;
}

bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull) {
  en_driver_state_t eds;
  switch (pull) {
    case MGOS_GPIO_PULL_NONE:
      eds = HiZ;
      break;
    case MGOS_GPIO_PULL_UP:
      eds = Pullup;
      break;
    case MGOS_GPIO_PULL_DOWN:
      eds = Pulldown;
      break;
    default:
      return false;
  }
  switch (RS14100_PIN_DOMAIN(pin)) {
    case RS14100_HP: {
      RSI_EGPIO_PadDriverDisableState(RS14100_HP_PIN_NUM(pin), eds);
      break;
    }
    case RS14100_ULP: {
      // NB: ULP pin pull-up/down settings are applied to groups of 4:
      // 0:3, 3:7, 8:11 and 12:15 have pull setting controlled by the same bit.
      RSI_EGPIO_UlpPadDriverDisableState(RS14100_ULP_PIN_NUM(pin),
                                         (en_ulp_driver_disable_state_t) eds);
      break;
    }
    case RS14100_UULP: {
      return false;
    }
    default:
      return false;
  }
  return true;
}

bool mgos_gpio_setup_output(int pin, bool level) {
  if (!rs14100_gpio_clk_en(pin)) return false;
  mgos_gpio_write(pin, level);
  return mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT);
}

void mgos_gpio_hal_clear_int(int pin) {
  // TODO(rojer)
  (void) pin;
}

bool mgos_gpio_hal_set_int_mode(int pin, enum mgos_gpio_int_mode mode) {
  // TODO(rojer)
  (void) pin;
  (void) mode;
  return false;
}

bool mgos_gpio_hal_enable_int(int pin) {
  // TODO(rojer)
  (void) pin;
  return false;
}

bool mgos_gpio_hal_disable_int(int pin) {
  // TODO(rojer)
  (void) pin;
  return false;
}

enum mgos_init_result mgos_gpio_hal_init(void) {
  return MGOS_INIT_OK;
}
