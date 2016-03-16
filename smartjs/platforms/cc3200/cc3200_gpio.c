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
#include "pin.h"
#include "prcm.h"
#include "rom_map.h"

/*
 * pin is a literal pin number, as that seems to be the custom in TI land.
 *
 * For pin functions, see tables 16-6 and 16-7 of the TRM (p. 482)
 */
static int pin_to_gpio_no(int pin) {
  if (pin >= 1 && pin <= 8) return (pin + 9);   /* GPIO10 - 17 */
  if (pin >= 15 && pin <= 17) return (pin + 7); /* GPIO22 - 24 */
  if (pin == 18) return 28;
  /* Pin 20 has the GPIO function but is not available in table 16-6. */
  if (pin == 21) return 25;
  /* Pin 45 is not listed as available in table 16-6. */
  if (pin == 50) return 0;
  /* Pin 52 / GPIO32 is magic, output-only when not using 32 KHz crystal. */
  if (pin == 53) return 30;
  if (pin == 55) return 1;
  if (pin >= 57 && pin <= 64) return (pin - 55); /* GPIO2 - 9 */
  return -1;
}

static uint32_t gpio_no_to_gpio_base(int gpio_no) {
  return (GPIOA0_BASE + 0x1000 * (gpio_no / 8));
}

int sj_gpio_set_mode(int pin, enum gpio_mode mode, enum gpio_pull_type pull) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return -1;
  if (mode == GPIO_MODE_INOUT) return -1; /* CC3200 does not support in+out. */
  if (mode == GPIO_MODE_INT) return -1;   /* TODO(rojer) */

  uint32_t gpio_port_no = (gpio_no / 8); /* A0 - A4 */
  uint32_t gpio_port_bit_no = (gpio_no % 8);
  uint32_t gpio_base = gpio_no_to_gpio_base(gpio_no);
  uint32_t gpio_port_bit_mask = (1 << gpio_port_bit_no);

  MAP_PRCMPeripheralClkEnable(PRCM_GPIOA0 + gpio_port_no, PRCM_RUN_MODE_CLK);

  uint32_t pad_config = PIN_MODE_0 /* GPIO is always mode 0 */;
  if (mode == GPIO_MODE_OUTPUT) {
    HWREG(gpio_base + GPIO_O_GPIO_DIR) |= gpio_port_bit_mask;
    pad_config |= 0xA0; /* drive strength 10mA. */
  } else {
    HWREG(gpio_base + GPIO_O_GPIO_DIR) &= ~gpio_port_bit_mask;
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
  uint32_t gpio_base = gpio_no_to_gpio_base(gpio_no);
  uint32_t amask = (1 << (gpio_no % 8)) << 2;
  return (HWREG(gpio_base + GPIO_O_GPIO_DATA + amask)) ? GPIO_LEVEL_HIGH
                                                       : GPIO_LEVEL_LOW;
}

int sj_gpio_write(int pin, enum gpio_level level) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return GPIO_LEVEL_ERR;
  uint32_t gpio_base = gpio_no_to_gpio_base(gpio_no);
  uint32_t amask = (1 << (gpio_no % 8)) << 2;
  if (level == GPIO_LEVEL_HIGH) {
    HWREG(gpio_base + GPIO_O_GPIO_DATA + amask) = 0xFF;
  } else {
    HWREG(gpio_base + GPIO_O_GPIO_DATA + amask) = 0;
  }
  return 0;
}

int sj_gpio_intr_set(int pin, enum gpio_int_mode type) {
  /* TODO(rojer): Implement this. */
  return -1;
}

void sj_gpio_intr_init(f_gpio_intr_handler_t cb) {
  /* TODO(rojer): Implement this. */
}
