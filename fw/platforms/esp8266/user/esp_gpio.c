/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <ets_sys.h>
#include "fw/src/miot_gpio.h"

#include "esp_gpio.h"
#include "common/platforms/esp8266/esp_missing_includes.h"
#include "common/cs_dbg.h"
#include "esp_periph.h"

#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <user_interface.h>
#include <stdlib.h>

struct intr_disp_params {
  f_gpio_intr_handler_t cb;
  void *cb_arg;
};

/* These declarations are missing in SDK headers since ~1.0 */
#define PERIPHS_IO_MUX_PULLDWN BIT6
#define PIN_PULLDWN_DIS(PIN_NAME) \
  CLEAR_PERI_REG_MASK(PIN_NAME, PERIPHS_IO_MUX_PULLDWN)
#define PIN_PULLDWN_EN(PIN_NAME) \
  SET_PERI_REG_MASK(PIN_NAME, PERIPHS_IO_MUX_PULLDWN)

#define GPIO_PIN_COUNT 16

static uint16_t int_map[GPIO_PIN_COUNT] = {0};

#define GPIO_TASK_QUEUE_LEN 25

static os_event_t gpio_task_queue[GPIO_TASK_QUEUE_LEN];
/* TODO(alashkin): introduce some kind of tasks priority registry */
#define TASK_PRIORITY 2

#define GPIO_TASK_SIG 0x123

#define GPIO_INTR_TYPE_ONCLICK 6
#define GPIO_ONCLICK_SKIP_INTR_COUNT 15

static void gpio16_set_output_mode(void) {
  WRITE_PERI_REG(
      PAD_XPD_DCDC_CONF,
      (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32_t) 0x1);

  WRITE_PERI_REG(
      RTC_GPIO_CONF,
      (READ_PERI_REG(RTC_GPIO_CONF) & (uint32_t) 0xfffffffe) | (uint32_t) 0x0);

  WRITE_PERI_REG(RTC_GPIO_ENABLE,
                 (READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32_t) 0xfffffffe) |
                     (uint32_t) 0x1);
}

static void gpio16_output_set(uint8_t value) {
  WRITE_PERI_REG(RTC_GPIO_OUT,
                 (READ_PERI_REG(RTC_GPIO_OUT) & (uint32_t) 0xfffffffe) |
                     (uint32_t)(value & 1));
}

static void gpio16_set_input_mode(void) {
  WRITE_PERI_REG(
      PAD_XPD_DCDC_CONF,
      (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32_t) 0x1);

  WRITE_PERI_REG(
      RTC_GPIO_CONF,
      (READ_PERI_REG(RTC_GPIO_CONF) & (uint32_t) 0xfffffffe) | (uint32_t) 0x0);

  WRITE_PERI_REG(RTC_GPIO_ENABLE,
                 READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32_t) 0xfffffffe);
}

static uint8_t gpio16_input_get(void) {
  return (uint8_t)(READ_PERI_REG(RTC_GPIO_IN_DATA) & 1);
}

int miot_gpio_set_mode(int pin, enum gpio_mode mode, enum gpio_pull_type pull) {
  struct gpio_info *gi;

  if (pin == 16) {
    if (mode == GPIO_MODE_INPUT) {
      gpio16_set_input_mode();
    } else {
      gpio16_set_output_mode();
    }
    return 0;
  }

  gi = get_gpio_info(pin);

  if (gi == NULL) {
    return -1;
  }

  switch (pull) {
    case GPIO_PULL_PULLUP:
      PIN_PULLDWN_DIS(gi->periph);
      PIN_PULLUP_EN(gi->periph);
      break;
    case GPIO_PULL_PULLDOWN:
      PIN_PULLUP_DIS(gi->periph);
      PIN_PULLDWN_EN(gi->periph);
      break;
    case GPIO_PULL_FLOAT:
      PIN_PULLUP_DIS(gi->periph);
      PIN_PULLDWN_DIS(gi->periph);
      break;
    default:
      return -1;
  }

  switch (mode) {
    case GPIO_MODE_INOUT:
      PIN_FUNC_SELECT(gi->periph, gi->func);
      break;

    case GPIO_MODE_INPUT:
      PIN_FUNC_SELECT(gi->periph, gi->func);
      GPIO_DIS_OUTPUT(GPIO_ID_PIN(pin));
      break;

    case GPIO_MODE_OUTPUT:
      ENTER_CRITICAL(ETS_GPIO_INUM);

      PIN_FUNC_SELECT(gi->periph, gi->func);

      gpio_pin_intr_state_set(GPIO_ID_PIN(pin), GPIO_PIN_INTR_DISABLE);

      GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin));
      GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(pin)),
                     GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(pin))) &
                         (~GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)));

      EXIT_CRITICAL(ETS_GPIO_INUM);
      break;
  }

  return 0;
}

int miot_gpio_write(int pin, enum gpio_level level) {
  level &= 0x1;

  if (pin == 16) {
    gpio16_output_set(level);
    return 0;
  }

  if (get_gpio_info(pin) == NULL) {
    /* Just verifying pin number */
    return -1;
  }

  GPIO_OUTPUT_SET(GPIO_ID_PIN(pin), level);
  return 0;
}

enum gpio_level miot_gpio_read(int pin) {
  if (pin == 16) {
    return 0x1 & gpio16_input_get();
  }

  if (get_gpio_info(pin) == NULL) {
    /* Just verifying pin number */
    return -1;
  }

  return 0x1 & GPIO_INPUT_GET(GPIO_ID_PIN(pin));
}

/*
 * How mode 6 ("button") works (example):
 * 1. Button's GPIO level is 0 (low)
 * 2. We setup handler for level = 1 (high). That means, we will get
 * interruption
 * once level become high (e.g. user presses button)
 * 3. User presses button, level becomes high
 * 4. Once level become stable, callback is called AND handler for level = 1 is
 * removed. Handler for level 0 is set.
 * 5. User still press the button, but we don't have any interruption, because
 * we already waiting for level = 0
 * 6. User releases the button. Level become 0, stabilizes, callback is called
 * AND we starting to wait level = 1.
 * So, while user holds key we don't have any interruption.
 * 7. And so on, and so on.
*/
IRAM static void v7_gpio_process_on_click(int pin, int level,
                                          f_gpio_intr_handler_t callback) {
  if (GPIO_PIN_INTR_HILEVEL - (int_map[pin] & 0xF) != level) {
    /*
     * In order to a avoid false positive, waiting
     * for level became stable
     * Using 8 MSB in int_map[] for counter, so
     * instead of count++ using count += 0x100 (1 << 8)
     */
    int_map[pin] -= 0x100;
  } else {
    int_map[pin] += 0x100;
  }

  if ((int_map[pin] & 0xFF00) == 0) {
    /*
     * Ok, we have GPIO_ONCLICK_SKIP_INTR_COUNT interuptions
     * of the same type, it is time to call callback
     * and switch handler to opposite type (low <-> high)
     * Here we just adjust int_map[] content,
     * real interruption status will be changed in v7_gpio_intr_dispatcher()
     */
    int_map[pin] = GPIO_ONCLICK_SKIP_INTR_COUNT << 8 | 0xF0 |
                   (GPIO_PIN_INTR_HILEVEL - level);
    system_os_post(TASK_PRIORITY,
                   (uint32_t) GPIO_TASK_SIG << 16 | pin << 8 | level,
                   (os_param_t) callback);
  } else {
    miot_reenable_intr(pin);
  }
}

IRAM static void v7_gpio_intr_dispatcher(void *arg) {
  struct intr_disp_params *params = (struct intr_disp_params *) arg;
  uint8_t i, level;

  uint32_t gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

  for (i = 0; i < GPIO_PIN_COUNT; i++) {
    if ((int_map[i] & 0xF) && (gpio_status & BIT(i))) {
      gpio_pin_intr_state_set(GPIO_ID_PIN(i), GPIO_PIN_INTR_DISABLE);
      GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(i));
      level = 0x1 & GPIO_INPUT_GET(GPIO_ID_PIN(i));

      if ((int_map[i] & 0xF0) == 0xF0) {
        /* this is "on click" handler */
        v7_gpio_process_on_click(i, level, params->cb);
      } else {
        system_os_post(TASK_PRIORITY,
                       (uint32_t) GPIO_TASK_SIG << 16 | i << 8 | level,
                       (os_param_t) params);
      }

      /*
       * NOTE: callback must reenable interruptions by
       * calling miot_reenable_intr
       */
    }
  }
}

void miot_reenable_intr(int pin) {
  gpio_pin_intr_state_set(GPIO_ID_PIN(pin), int_map[pin] & 0xF);
}

void v7_gpio_task(os_event_t *event) {
  if (event->sig >> 16 != GPIO_TASK_SIG) {
    return;
  }

  struct intr_disp_params *params = (struct intr_disp_params *) event->par;
  params->cb((event->sig & 0xFFFF) >> 8, event->sig & 0xFF, params->cb_arg);
}

void miot_gpio_intr_init(f_gpio_intr_handler_t cb, void *arg) {
  static struct intr_disp_params *p = NULL;

  if (p == NULL) {
    p = malloc(sizeof(*p));
    if (p == NULL) {
      LOG(LL_ERROR, ("Out of memory"));
      return;
    }
  }

  p->cb_arg = arg;
  p->cb = cb;

  system_os_task(v7_gpio_task, TASK_PRIORITY, gpio_task_queue,
                 GPIO_TASK_QUEUE_LEN);
  ETS_GPIO_INTR_ATTACH(v7_gpio_intr_dispatcher, p);
}

static void v7_setup_on_click(int pin) {
  uint8_t current_level = miot_gpio_read(pin);
  /*
   * if current level is high, set interupt on low (4)
   * else set on high (5)
   */
  uint8_t type = GPIO_PIN_INTR_HILEVEL - current_level;
  int_map[pin] = GPIO_ONCLICK_SKIP_INTR_COUNT << 8 | 0xF0 | type;
}

int miot_gpio_intr_set(int pin, enum gpio_int_mode type) {
  if (get_gpio_info(pin) == NULL) {
    return -1;
  }

  /*
   * ESP allows to setup only 1 GPIO interruption handler
   * and calls it for interupt on any pin
   * So, we use 1 main function (dispatcher)
   * and use int_map variable as a map
   * pin <-> user interruption setup
   */
  int_map[pin] = type;

  if (type == GPIO_INTR_TYPE_ONCLICK) {
    v7_setup_on_click(pin);
  }

  ENTER_CRITICAL(ETS_GPIO_INUM);
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin));

  gpio_pin_intr_state_set(GPIO_ID_PIN(pin), int_map[pin] & 0xF);
  EXIT_CRITICAL(ETS_GPIO_INUM);

  return 0;
}

/* From gpio_register.h */
#define PERIPHS_GPIO_BASEADDR 0x60000300
#define GPIO_IN_ADDRESS 0x18
#define GPIO_STRAPPING 0x0000ffff
#define GPIO_STRAPPING_S 16

/* You'd think pins would map the same way as input, but no. GPIO0 is bit 1. */
#define GPIO_STRAPPING_PIN_0 0x2

bool esp_strapping_to_bootloader() {
  uint32_t strapping_v =
      (READ_PERI_REG(PERIPHS_GPIO_BASEADDR + GPIO_IN_ADDRESS) >>
       GPIO_STRAPPING_S) &
      GPIO_STRAPPING;
  return (strapping_v & GPIO_STRAPPING_PIN_0) == 0; /* GPIO0 is strapped to 0 */
}
