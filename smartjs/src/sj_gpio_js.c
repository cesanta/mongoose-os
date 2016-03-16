/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "v7/v7.h"
#include "sj_gpio.h"
#include "sj_common.h"

#ifndef SJ_DISABLE_GPIO

static int s_gpio_intr_installed = 0;

/* TODO(alashkin): use the same v7 pointer in all files/ports */
static struct v7 *s_v7;

static void gpio_intr_handler_proxy(int pin, enum gpio_level level) {
  char prop_name[15];
  int len;
  v7_val_t res, args;

  len = snprintf(prop_name, sizeof(prop_name), "_ih_%d", (int) pin);

  v7_val_t cb = v7_get(s_v7, v7_get_global(s_v7), prop_name, len);

  if (!v7_is_callable(s_v7, cb)) {
    return;
  }

  args = v7_mk_array(s_v7);
  v7_array_push(s_v7, args, v7_mk_number(pin));
  v7_array_push(s_v7, args, v7_mk_number(level));

  if (v7_apply(s_v7, cb, v7_mk_undefined(), args, &res) != V7_OK) {
    /* TODO(mkm): make it print stack trace */
    fprintf(stderr, "cb threw an exception\n");
  }
}

SJ_PRIVATE enum v7_err GPIO_setisr(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t pinv = v7_arg(v7, 0);
  v7_val_t typev = v7_arg(v7, 1);
  v7_val_t cb = v7_arg(v7, 2);
  v7_val_t current_cb;

  char prop_name[15];
  int pin, type, len, has_isr, new_isr_provided;

  if (!v7_is_number(pinv) || !v7_is_number(typev)) {
    printf("Invalid arguments\n");
    *res = v7_mk_boolean(0);
    goto clean;
  }

  pin = v7_to_number(pinv);
  type = v7_to_number(typev);

  len = snprintf(prop_name, sizeof(prop_name), "_ih_%d", (int) pin);
  current_cb = v7_get(v7, v7_get_global(v7), prop_name, len);
  has_isr = v7_is_callable(v7, current_cb);
  new_isr_provided = v7_is_callable(v7, cb);

  if (!has_isr && !new_isr_provided) {
    printf("Missing callback\n");
    *res = v7_mk_boolean(0);
    goto clean;
  };

  if (has_isr && new_isr_provided && current_cb != cb) {
    printf("Only one interruption handler is allowed for pin\n");
    *res = v7_mk_boolean(0);
    goto clean;
  }

  if (type == 0 && has_isr) {
    v7_set(v7, v7_get_global(v7), prop_name, len, v7_mk_undefined());
  } else if (!has_isr && new_isr_provided) {
    v7_set(v7, v7_get_global(v7), prop_name, len, cb);
  }

  if (type != 0 && !s_gpio_intr_installed) {
    sj_gpio_intr_init(gpio_intr_handler_proxy);
    s_gpio_intr_installed = 1;
  }

  *res = v7_mk_boolean(sj_gpio_intr_set(pin, type) == 0);
  goto clean;

clean:
  return rcode;
}

SJ_PRIVATE enum v7_err GPIO_setmode(struct v7 *v7, v7_val_t *res) {
  v7_val_t pinv = v7_arg(v7, 0);
  v7_val_t modev = v7_arg(v7, 1);
  v7_val_t pullv = v7_arg(v7, 2);
  int pin, mode, pull;

  if (!v7_is_number(pinv) || !v7_is_number(modev) || !v7_is_number(pullv)) {
    printf("Invalid arguments");
    *res = v7_mk_undefined();
  } else {
    pin = v7_to_number(pinv);
    mode = v7_to_number(modev);
    pull = v7_to_number(pullv);
    *res = v7_mk_boolean(sj_gpio_set_mode(pin, mode, pull) == 0);
  }

  return V7_OK;
}

SJ_PRIVATE enum v7_err GPIO_write(struct v7 *v7, v7_val_t *res) {
  v7_val_t pinv = v7_arg(v7, 0);
  v7_val_t valv = v7_arg(v7, 1);
  int pin, val;

  if (!v7_is_number(pinv)) {
    printf("non-numeric pin\n");
    *res = v7_mk_undefined();
  } else {
    pin = v7_to_number(pinv);

    /*
     * We assume 0 if the value is "falsy",
     * and 1 if the value is "truthy"
     */
    val = !!v7_is_truthy(v7, valv);

    *res = v7_mk_boolean(sj_gpio_write(pin, val) == 0);
  }

  return V7_OK;
}

SJ_PRIVATE enum v7_err GPIO_read(struct v7 *v7, v7_val_t *res) {
  v7_val_t pinv = v7_arg(v7, 0);
  int pin;

  if (!v7_is_number(pinv)) {
    printf("non-numeric pin\n");
    *res = v7_mk_undefined();
  } else {
    pin = v7_to_number(pinv);
    *res = v7_mk_number(sj_gpio_read(pin));
  }

  return V7_OK;
}

void sj_gpio_api_setup(struct v7 *v7) {
  s_v7 = v7;
  v7_val_t gpio = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "GPIO", ~0, gpio);
  v7_set_method(v7, gpio, "setmode", GPIO_setmode);
  v7_set_method(v7, gpio, "read", GPIO_read);
  v7_set_method(v7, gpio, "write", GPIO_write);
  v7_set_method(v7, gpio, "setisr", GPIO_setisr);
}

#else

void sj_gpio_api_setup(struct v7 *v7) {
  (void) v7;
}

#endif
