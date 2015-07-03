#include "ets_sys.h"
#include "osapi.h"
#include "v7.h"
#include "v7_gpio.h"
#include "v7_esp.h"

static int s_gpio_intr_installed = 0;

ICACHE_FLASH_ATTR static void gpio_intr_handler_proxy(int pin, int level) {
  char prop_name[15];
  int len;
  v7_val_t args;

  len = snprintf(prop_name, sizeof(prop_name), "_ih_%d", (int) pin);

  v7_val_t cb = v7_get(v7, v7_get_global_object(v7), prop_name, len);

  if (!v7_is_function(cb)) {
    return;
  }

  args = v7_create_array(v7);
  v7_array_push(v7, args, v7_create_number(pin));
  v7_array_push(v7, args, v7_create_number(level));

  v7_apply(v7, cb, v7_create_undefined(), args);
}

ICACHE_FLASH_ATTR static v7_val_t GPIO_setisr(struct v7 *v7, v7_val_t this_obj,
                                              v7_val_t args) {
  v7_val_t pinv = v7_array_get(v7, args, 0);
  v7_val_t typev = v7_array_get(v7, args, 1);
  v7_val_t cb = v7_array_get(v7, args, 2);
  v7_val_t current_cb;

  char prop_name[15];
  int pin, type, len, has_isr, new_isr_provided;

  if (!v7_is_number(pinv) || !v7_is_number(type)) {
    printf("Invalid arguments\n");
    return v7_create_boolean(0);
  }

  pin = v7_to_number(pinv);
  type = v7_to_number(typev);

  len = snprintf(prop_name, sizeof(prop_name), "_ih_%d", (int) pin);
  current_cb = v7_get(v7, v7_get_global_object(v7), prop_name, len);
  has_isr = v7_is_function(current_cb);
  new_isr_provided = v7_is_function(cb);

  if (!has_isr && !new_isr_provided) {
    printf("Missing callback\n");
    return v7_create_boolean(0);
  };

  if (has_isr && new_isr_provided && current_cb != cb) {
    printf("Only one interruption handler is allowed for pin\n");
    return v7_create_boolean(0);
  }

  if (type == 0 && has_isr) {
    v7_set(v7, v7_get_global_object(v7), prop_name, len, 0,
           v7_create_undefined());
  } else if (!has_isr && new_isr_provided) {
    v7_set(v7, v7_get_global_object(v7), prop_name, len, 0, cb);
  }

  if (type != 0 && !s_gpio_intr_installed) {
    v7_gpio_intr_init(gpio_intr_handler_proxy);
    s_gpio_intr_installed = 1;
  }

  return v7_create_boolean(v7_gpio_intr_set(pin, type) == 0);
}

ICACHE_FLASH_ATTR static v7_val_t GPIO_setmode(struct v7 *v7, v7_val_t this_obj,
                                               v7_val_t args) {
  v7_val_t pinv = v7_array_get(v7, args, 0);
  v7_val_t modev = v7_array_get(v7, args, 1);
  v7_val_t pullv = v7_array_get(v7, args, 2);
  int pin, mode, pull;

  if (!v7_is_number(pinv) || !v7_is_number(modev) || !v7_is_number(pullv)) {
    printf("Invalid arguments");
    return v7_create_undefined();
  }

  pin = v7_to_number(pinv);
  mode = v7_to_number(modev);
  pull = v7_to_number(pullv);

  return v7_create_boolean(v7_gpio_set_mode(pin, mode, pull) == 0);
}

ICACHE_FLASH_ATTR static v7_val_t GPIO_write(struct v7 *v7, v7_val_t this_obj,
                                             v7_val_t args) {
  v7_val_t pinv = v7_array_get(v7, args, 0);
  v7_val_t valv = v7_array_get(v7, args, 1);
  int pin, val;

  if (!v7_is_number(pinv)) {
    printf("non-numeric pin\n");
    return v7_create_undefined();
  }
  pin = v7_to_number(pinv);
  val = v7_to_number(valv);

  return v7_create_boolean(v7_gpio_write(pin, val) == 0);
}

ICACHE_FLASH_ATTR static v7_val_t GPIO_read(struct v7 *v7, v7_val_t this_obj,
                                            v7_val_t args) {
  v7_val_t pinv = v7_array_get(v7, args, 0);
  int pin;

  if (!v7_is_number(pinv)) {
    printf("non-numeric pin\n");
    return v7_create_undefined();
  }

  pin = v7_to_number(pinv);
  return v7_create_number(v7_gpio_read(pin));
}

ICACHE_FLASH_ATTR void init_gpiojs(struct v7 *v7) {
  v7_val_t gpio = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "GPIO", 4, 0, gpio);
  v7_set_method(v7, gpio, "setmode", GPIO_setmode);
  v7_set_method(v7, gpio, "read", GPIO_read);
  v7_set_method(v7, gpio, "write", GPIO_write);
  v7_set_method(v7, gpio, "setisr", GPIO_setisr);
}
