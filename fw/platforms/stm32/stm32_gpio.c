#include <stm32_hal.h>
#include <stdlib.h>
#include "fw/src/mgos_gpio_hal.h"
#include "common/queue.h"


struct gpio_info {
  int pin;
  uint16_t gpio_pin;
  GPIO_TypeDef* gpiox;
  GPIO_InitTypeDef init_info;
  SLIST_ENTRY(gpio_info) gpio_infos;
};
SLIST_HEAD(s_gpio_infos, gpio_info) s_gpio_infos = SLIST_HEAD_INITIALIZER(s_gpio_infos);

static void get_stm_pin(int pin, GPIO_TypeDef** gpiox, uint16_t *gpio_pin) {
  /* STM GPIO is a combination of port and pin inside this port */
  uint16_t pin_offset = (pin & 0xFFFF0000) >> 16;
  *gpiox = (GPIO_TypeDef *) (AHB1PERIPH_BASE + pin_offset);
  *gpio_pin = (pin & 0x0000FFFF);
}

static struct gpio_info* get_gpio_info(int pin) {
  struct gpio_info *gi = NULL, *gi_tmp;
  SLIST_FOREACH_SAFE(gi, &s_gpio_infos, gpio_infos, gi_tmp) {
    if (gi->pin == pin) {
      break;
    }
  }

  if (gi == NULL || gi->pin != pin) {
    gi = (struct gpio_info *) calloc(1, sizeof(*gi));
    SLIST_INSERT_HEAD(&s_gpio_infos, gi, gpio_infos);
    get_stm_pin(pin, &gi->gpiox, &gi->gpio_pin);
    gi->pin = pin;
    gi->init_info.Pin = gi->gpio_pin;
    gi->init_info.Mode = GPIO_MODE_INPUT;
    gi->init_info.Pull = GPIO_NOPULL;
    gi->init_info.Speed = GPIO_SPEED_FREQ_LOW;
  }

  return gi;
}

enum mgos_init_result mgos_gpio_dev_init(void) {
  /* Do nothing here */
  return MGOS_INIT_OK;
}

bool mgos_gpio_toggle(int pin) {
  bool value = mgos_gpio_read(pin);
  mgos_gpio_write(pin, !value);
  return !value;
}

bool mgos_gpio_read(int pin) {
  if (pin < GPIO_PIN_0) {
    return false;
  }
  GPIO_TypeDef *gpiox;
  uint16_t gpio_pin;
  get_stm_pin(pin, &gpiox, &gpio_pin);
  return (bool) HAL_GPIO_ReadPin(gpiox, gpio_pin);
}

void mgos_gpio_write(int pin, bool level) {
  if (pin < GPIO_PIN_0) {
    return;
  }
  GPIO_TypeDef *gpiox;
  uint16_t gpio_pin;
  get_stm_pin(pin, &gpiox, &gpio_pin);
  HAL_GPIO_WritePin(gpiox, gpio_pin, (GPIO_PinState) level);
}

bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode) {
  if (pin < GPIO_PIN_0) {
    return false;
  }
  struct gpio_info *gi = get_gpio_info(pin);
  gi->init_info.Mode = mode;
  HAL_GPIO_Init(gi->gpiox, &gi->init_info);
  return true;
}

bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull) {
  if (pin < GPIO_PIN_0) {
    return false;
  }
  struct gpio_info *gi = get_gpio_info(pin);
  gi->init_info.Pull = pull;
  HAL_GPIO_Init(gi->gpiox, &gi->init_info);
  return true;
}

void mgos_gpio_dev_int_done(int pin) {
  /* TODO(alashkin): implement */
  (void) pin;
}

bool mgos_gpio_dev_set_int_mode(int pin, enum mgos_gpio_int_mode mode) {
  /* TODO(alashkin): implement */
  (void) pin;
  (void) mode;
}

bool mgos_gpio_enable_int(int pin) {
  /* TODO(alashkin): implement */
  (void) pin;
}

bool mgos_gpio_disable_int(int pin) {
  /* TODO(alashkin): implement */
  (void) pin;
}
