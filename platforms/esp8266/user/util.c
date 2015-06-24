#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "v7.h"
#include "mem.h"
#include <math.h>
#include <stdlib.h>
#include "v7_esp.h"
#include "util.h"

ICACHE_FLASH_ATTR void set_gpio(int g, int v) {
#define GPIO_SET(pin) gpio_output_set(1 << pin, 0, 1 << pin, 0);
#define GPIO_CLR(pin) gpio_output_set(0, 1 << pin, 1 << pin, 0);
  if (v) {
    GPIO_SET(g);
  } else {
    GPIO_CLR(g);
  }
}

ICACHE_FLASH_ATTR int read_gpio_pin(int g) {
  gpio_output_set(0, 0, 0, 1 << g);
  return (gpio_input_get() & (1 << g)) != 0;
}

ICACHE_FLASH_ATTR
int await_change(int gpio, int *max_cycles) {
  int v1, v2, n;
  v1 = read_gpio_pin(gpio);
  for (n = *max_cycles; (*max_cycles)-- > 0;) {
    v2 = read_gpio_pin(gpio);
    if (v2 != v1) {
      return (n - *max_cycles);
    }
  }
  return 0;
}

#if !defined(V7_NO_FS) && !defined(NO_EXEC_INITJS)
ICACHE_FLASH_ATTR void v7_run_startup() {
  v7_val_t v;
  static char buf[15];
  /*
   * It is a question - should we print "Executing
   * and print message if init.js is not found
   * For the moment - print in order
   * to let user know that v7 has some "init.js"
   */
  printf("\n\rExecuting init.js\n\r");
  int res = v7_exec_file(v7, &v, "init.js");
  if (res != V7_OK) {
    char *p;
    p = v7_to_json(v7, v, buf, sizeof(buf));
    printf("init.js execution: %s\n\r", p);
    if (p != buf) {
      free(p);
    }
  }
}
#endif
