/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>

#include "fw/src/miot_gpio.h"
#include "fw/src/miot_common.h"
#include "fw/src/miot_v7_ext.h"
#include "v7/v7.h"

#if MIOT_ENABLE_JS && MIOT_ENABLE_GPIO_API

extern struct v7 *v7;

#if 0
static v7_val_t s_isr_cb_proxy_v;

// XXX  - broken

static void gpio_intr_handler_proxy(void *arg) {
  char prop_name[15];
  int len;
  int pin = (intptr_t) arg;

  len = snprintf(prop_name, sizeof(prop_name), "_ih_%d", (int) pin);

  v7_val_t cb = v7_get(v7, v7_get_global(v7), prop_name, len);

  if (!v7_is_callable(v7, cb)) {
    return;
  }

  /* Forwarding call to common cbs queue */
  v7_val_t args = v7_mk_array(v7);
  v7_array_push(v7, args, v7_mk_number(v7, pin));
  miot_invoke_js_cb2(v7, s_isr_cb_proxy_v, cb, args);
}
#endif

MG_PRIVATE enum v7_err GPIO_setISR(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t pinv = v7_arg(v7, 0);
  v7_val_t typev = v7_arg(v7, 1);
  v7_val_t cb = v7_arg(v7, 2);
  v7_val_t current_cb;

  char prop_name[15];
  int pin, type, len, has_isr, new_isr_provided;

  if (!v7_is_number(pinv) || !v7_is_number(typev)) {
    printf("Invalid arguments\n");
    *res = v7_mk_boolean(v7, 0);
    goto clean;
  }

  pin = v7_get_double(v7, pinv);
  type = v7_get_double(v7, typev);

  len = snprintf(prop_name, sizeof(prop_name), "_ih_%d", (int) pin);
  current_cb = v7_get(v7, v7_get_global(v7), prop_name, len);
  has_isr = v7_is_callable(v7, current_cb);
  new_isr_provided = v7_is_callable(v7, cb);

  if (!has_isr && !new_isr_provided) {
    printf("Missing callback\n");
    *res = v7_mk_boolean(v7, 0);
    goto clean;
  };

  if (has_isr && new_isr_provided && current_cb != cb) {
    printf("Only one interruption handler is allowed for pin\n");
    *res = v7_mk_boolean(v7, 0);
    goto clean;
  }

  if (type == 0 && has_isr) {
    v7_set(v7, v7_get_global(v7), prop_name, len, V7_UNDEFINED);
  } else if (!has_isr && new_isr_provided) {
    v7_set(v7, v7_get_global(v7), prop_name, len, cb);
  }

#if 0
  // XXX
  if (type != 0) {
    miot_gpio_intr_init(gpio_intr_handler_proxy, v7);
    s_isr_cb_proxy_v = v7_mk_cfunction(isr_cb_proxy);
    v7_own(v7, &s_isr_cb_proxy_v);
    s_gpio_intr_installed = 1;
  }
  *res = v7_mk_boolean(v7,
                       miot_gpio_intr_set(pin, (enum gpio_int_mode) type) == 0);
#endif
  goto clean;

clean:
  return rcode;
}

MG_PRIVATE enum v7_err GPIO_setMode(struct v7 *v7, v7_val_t *res) {
  v7_val_t pinv = v7_arg(v7, 0);
  v7_val_t modev = v7_arg(v7, 1);
  v7_val_t pullv = v7_arg(v7, 2);
  int pin = v7_get_double(v7, pinv);
  int mode = v7_get_double(v7, modev);

  if (v7_is_number(pinv) && v7_is_number(modev) &&
      (mode == MIOT_GPIO_MODE_OUTPUT || v7_is_number(pullv))) {
    int pull = MIOT_GPIO_PULL_NONE;
    if (mode != MIOT_GPIO_MODE_OUTPUT) pull = v7_get_double(v7, pullv);
    *res = v7_mk_boolean(
        v7, (miot_gpio_set_mode(pin, (enum miot_gpio_mode) mode) == 0 &&
             miot_gpio_set_pull(pin, (enum miot_gpio_pull_type) pull)) == 0);
  } else {
    return v7_throwf(v7, "Error", "Invalid arguments");
  }

  return V7_OK;
}

MG_PRIVATE enum v7_err GPIO_write(struct v7 *v7, v7_val_t *res) {
  v7_val_t pinv = v7_arg(v7, 0);
  v7_val_t valv = v7_arg(v7, 1);
  int pin, val;

  if (!v7_is_number(pinv)) {
    printf("non-numeric pin\n");
    *res = V7_UNDEFINED;
  } else {
    pin = v7_get_double(v7, pinv);

    /*
     * We assume 0 if the value is "falsy",
     * and 1 if the value is "truthy"
     */
    val = !!v7_is_truthy(v7, valv);

    miot_gpio_write(pin, val);

    *res = v7_mk_boolean(v7, true);
  }

  return V7_OK;
}

MG_PRIVATE enum v7_err GPIO_read(struct v7 *v7, v7_val_t *res) {
  v7_val_t pinv = v7_arg(v7, 0);
  int pin;

  if (!v7_is_number(pinv)) {
    printf("non-numeric pin\n");
    *res = V7_UNDEFINED;
  } else {
    pin = v7_get_double(v7, pinv);
    *res = v7_mk_number(v7, miot_gpio_read(pin));
  }

  return V7_OK;
}

void miot_gpio_api_setup(struct v7 *v7) {
  v7_val_t gpio = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "GPIO", ~0, gpio);
  v7_set_method(v7, gpio, "setMode", GPIO_setMode);
  v7_set_method(v7, gpio, "read", GPIO_read);
  v7_set_method(v7, gpio, "write", GPIO_write);
  v7_set_method(v7, gpio, "setISR", GPIO_setISR);

  v7_set(v7, gpio, "IN", ~0, v7_mk_number(v7, MIOT_GPIO_MODE_INPUT));
  v7_set(v7, gpio, "OUT", ~0, v7_mk_number(v7, MIOT_GPIO_MODE_OUTPUT));

  v7_set(v7, gpio, "NONE", ~0, v7_mk_number(v7, MIOT_GPIO_PULL_NONE));
  v7_set(v7, gpio, "PULLUP", ~0, v7_mk_number(v7, MIOT_GPIO_PULL_UP));
  v7_set(v7, gpio, "PULLDOWN", ~0, v7_mk_number(v7, MIOT_GPIO_PULL_DOWN));

  v7_set(v7, gpio, "POSEDGE", ~0, v7_mk_number(v7, MIOT_GPIO_INT_EDGE_POS));
  v7_set(v7, gpio, "NEGEDGE", ~0, v7_mk_number(v7, MIOT_GPIO_INT_EDGE_NEG));
  v7_set(v7, gpio, "ANYEDGE", ~0, v7_mk_number(v7, MIOT_GPIO_INT_EDGE_ANY));
  v7_set(v7, gpio, "LOLEVEL", ~0, v7_mk_number(v7, MIOT_GPIO_INT_LEVEL_LO));
  v7_set(v7, gpio, "HILEVEL", ~0, v7_mk_number(v7, MIOT_GPIO_INT_LEVEL_HI));

  if (v7_exec_file(v7, "gpio.js", NULL) != V7_OK) {
    /* TODO(mkm): make setup functions return an error code */
  }
}

#endif /* MIOT_ENABLE_JS && MIOT_ENABLE_GPIO_API */
