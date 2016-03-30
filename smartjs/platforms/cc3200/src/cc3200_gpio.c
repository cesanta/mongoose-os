/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * CC3200 GPIO support.
 * Documentation: TRM (swru367), Chapter 5.
 */

#include "smartjs/src/sj_gpio.h"

#include <inttypes.h>
#include <stdio.h>

#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_ocp_shared.h"
#include "hw_gpio.h"
#include "hw_ints.h"
#include "interrupt.h"
#include "pin.h"
#include "prcm.h"
#include "rom_map.h"

#include "oslib/osi.h"

#include "cc3200_sj_hal.h"

/*
 * pin is a literal pin number, as that seems to be the custom in TI land.
 *
 * For pin functions, see tables 16-6 and 16-7 of the TRM (p. 482)
 *
 * Pins 20 and 45 have the GPIO function but are not listed as available
 * in table 16-6.
 * Pin 52 / GPIO32 is an output-only and is only available when not using
 * 32 KHz crystal. It is also the only GPIO in block 5, so we exclude it.
 */

/* clang-format off */
static char s_pin_to_gpio_map[64] = {
  10, 11, 12, 13, 14, 15, 16, 17, -1, -1,
  -1, -1, -1, -1, 22, 23, 24, 28, -1, -1,
  25, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,
  -1, -1, 30, -1,  1, -1,  2,  3,  4,  5,
   6,  7,  8,  9
};
static char s_gpio_to_pin_map[32] = {
  50, 55, 57, 58, 59, 60, 61, 62, 63, 64,
   1,  2,  3,  4,  5,  6,  7,  8, -1, -1,
  -1, -1, 15, 16, 17, 21, -1, -1, 18, -1,
  53, -1
};
/* clang-format on */

uint32_t s_enabled_ints = 0;

int pin_to_gpio_no(int pin) {
  if (pin < 1 || pin > 64) return -1;
  return s_pin_to_gpio_map[pin - 1];
}

static uint32_t gpio_no_to_port_base(int gpio_no) {
  return (GPIOA0_BASE + 0x1000 * (gpio_no / 8));
}

int sj_gpio_set_mode(int pin, enum gpio_mode mode, enum gpio_pull_type pull) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return -1;
  if (mode == GPIO_MODE_INOUT) return -1; /* CC3200 does not support in+out. */
  if (mode == GPIO_MODE_INT) return -1;   /* TODO(rojer) */

  uint32_t port_no = (gpio_no / 8); /* A0 - A4 */
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t port_bit_no = (gpio_no % 8);
  uint32_t port_bit_mask = (1 << port_bit_no);

  MAP_PRCMPeripheralClkEnable(PRCM_GPIOA0 + port_no, PRCM_RUN_MODE_CLK);

  uint32_t pad_config = PIN_MODE_0 /* GPIO is always mode 0 */;
  if (mode == GPIO_MODE_OUTPUT) {
    HWREG(port_base + GPIO_O_GPIO_DIR) |= port_bit_mask;
    pad_config |= 0xA0; /* drive strength 10mA. */
  } else {
    HWREG(port_base + GPIO_O_GPIO_DIR) &= ~port_bit_mask;
  }
  if (pull == GPIO_PULL_PULLUP) {
    pad_config |= PIN_TYPE_STD_PU;
  } else if (pull == GPIO_PULL_PULLDOWN) {
    pad_config |= PIN_TYPE_STD_PD;
  }
  uint32_t pad_reg =
      (OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CONFIG_0 + (gpio_no * 4));
  HWREG(pad_reg) = pad_config;
  return 0;
}

enum gpio_level sj_gpio_read(int pin) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return GPIO_LEVEL_ERR;
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t amask = (1 << (gpio_no % 8)) << 2;
  return (HWREG(port_base + GPIO_O_GPIO_DATA + amask)) ? GPIO_LEVEL_HIGH
                                                       : GPIO_LEVEL_LOW;
}

int sj_gpio_write(int pin, enum gpio_level level) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return GPIO_LEVEL_ERR;
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t amask = (1 << (gpio_no % 8)) << 2;
  if (level == GPIO_LEVEL_HIGH) {
    HWREG(port_base + GPIO_O_GPIO_DATA + amask) = 0xFF;
  } else {
    HWREG(port_base + GPIO_O_GPIO_DATA + amask) = 0;
  }
  return 0;
}

extern OsiMsgQ_t s_v7_q;

static void gpio_common_int_handler(uint32_t port_base, uint8_t offset) {
  uint32_t ints =
      HWREG(port_base + GPIO_O_GPIO_MIS) & (s_enabled_ints >> offset);
  uint32_t vals = HWREG(port_base + GPIO_O_GPIO_DATA + (0xff << 2));
  uint8_t gpio_no;
  HWREG(port_base + GPIO_O_GPIO_ICR) = ints; /* Clear all ints. */
  for (gpio_no = offset; ints != 0; ints >>= 1, vals >>= 1, gpio_no++) {
    if (!(ints & 1)) continue;
    int pin = s_gpio_to_pin_map[gpio_no];
    if (pin < 0) continue;
    struct sj_event e = {
        .type = GPIO_INT_EVENT, .data = (void *) ((pin << 1) | (vals & 1)),
    };
    osi_MsgQWrite(&s_v7_q, &e, OSI_NO_WAIT);
  }
}

static void gpio_a0_int_handler() {
  gpio_common_int_handler(GPIOA0_BASE, 0);
}

static void gpio_a1_int_handler() {
  gpio_common_int_handler(GPIOA1_BASE, 8);
}

static void gpio_a2_int_handler() {
  gpio_common_int_handler(GPIOA2_BASE, 16);
}

static void gpio_a3_int_handler() {
  gpio_common_int_handler(GPIOA3_BASE, 24);
}

int sj_gpio_intr_set(int pin, enum gpio_int_mode type) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return -1;
  uint32_t port_no = (gpio_no / 8); /* A0 - A4 */
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t port_bit_no = (gpio_no % 8);
  uint32_t port_bit_mask = (1 << port_bit_no);

  HWREG(port_base + GPIO_O_GPIO_IM) &= ~port_bit_mask;
  switch (type) {
    case GPIO_INTR_OFF:
      break;
    case GPIO_INTR_POSEDGE:
    case GPIO_INTR_NEGEDGE:
      HWREG(port_base + GPIO_O_GPIO_IS) &= ~port_bit_mask;  /* Edge */
      HWREG(port_base + GPIO_O_GPIO_IBE) &= ~port_bit_mask; /* Single */
      if (type == GPIO_INTR_POSEDGE) {
        HWREG(port_base + GPIO_O_GPIO_IEV) |= port_bit_mask; /* Rising */
      } else {
        HWREG(port_base + GPIO_O_GPIO_IEV) &= ~port_bit_mask; /* Falling */
      }
      break;
    case GPIO_INTR_ANYEDGE:
      HWREG(port_base + GPIO_O_GPIO_IS) &= ~port_bit_mask; /* Edge */
      HWREG(port_base + GPIO_O_GPIO_IBE) |= port_bit_mask; /* Any */
      break;
    case GPIO_INTR_LOLEVEL:
    case GPIO_INTR_HILEVEL:
      HWREG(port_base + GPIO_O_GPIO_IS) |= port_bit_mask; /* Level */
      if (type == GPIO_INTR_LOLEVEL) {
        HWREG(port_base + GPIO_O_GPIO_IEV) &= ~port_bit_mask; /* Low */
      } else {
        HWREG(port_base + GPIO_O_GPIO_IEV) |= port_bit_mask; /* High */
      }
      break;
  }
  if (type != GPIO_INTR_OFF) {
    P_OSI_INTR_ENTRY handlers[4] = {gpio_a0_int_handler, gpio_a1_int_handler,
                                    gpio_a2_int_handler, gpio_a3_int_handler};
    int int_no = (INT_GPIOA0 + port_no);
    osi_InterruptRegister(int_no, handlers[port_no], INT_PRIORITY_LVL_1);
    IntEnable(int_no);
    s_enabled_ints |= (1 << gpio_no);
    HWREG(port_base + GPIO_O_GPIO_RIS) &= ~port_bit_mask;
    HWREG(port_base + GPIO_O_GPIO_IM) |= port_bit_mask;
  } else {
    s_enabled_ints &= ~(1 << gpio_no);
  }
  return 0;
}
