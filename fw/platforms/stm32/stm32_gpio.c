#include <stm32_sdk_hal.h>
#include <stdlib.h>
#include "fw/src/mgos_gpio_hal.h"
#include "common/queue.h"

static void stm32_preinit_pin(struct stm32_gpio_def *def) {
  if (def->inited != 0) {
    return;
  }
  def->init_info.Pin = def->gpio;
  def->init_info.Mode = GPIO_MODE_INPUT;
  def->init_info.Pull = GPIO_NOPULL;
  def->init_info.Speed = GPIO_SPEED_FREQ_LOW;
  def->inited = 1;
}

static struct stm32_gpio_def *get_pin_def(int pin) {
  if (pin < 0 || pin > STM32_GPIO_NUM) {
    return NULL;
  }
  struct stm32_gpio_def *def = &stm32_gpio_defs[pin];
  stm32_preinit_pin(def);
  return def;
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
  struct stm32_gpio_def *def = get_pin_def(pin);
  if (def == NULL) {
    return false;
  }
  return (bool) HAL_GPIO_ReadPin(def->port, def->gpio);
}

void mgos_gpio_write(int pin, bool level) {
  struct stm32_gpio_def *def = get_pin_def(pin);
  if (def == NULL) {
    return;
  }
  HAL_GPIO_WritePin(def->port, def->gpio, (GPIO_PinState) level);
}

bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode) {
  struct stm32_gpio_def *def = get_pin_def(pin);
  if (def == NULL) {
    return false;
  }
  def->init_info.Mode = mode;
  HAL_GPIO_Init(def->port, &def->init_info);
  return true;
}

bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull) {
  struct stm32_gpio_def *def = get_pin_def(pin);
  if (def == NULL) {
    return false;
  }
  def->init_info.Pull = pull;
  HAL_GPIO_Init(def->port, &def->init_info);
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
