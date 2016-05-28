#include <stdio.h>

#include "common/platform.h"
#include "fw/src/sj_app.h"
#include "fw/src/sj_gpio.h"

#if CS_PLATFORM == CS_P_ESP_LWIP
#define GPIO 12
#elif CS_PLATFORM == CS_P_CC3200
#define GPIO 64
#else
#error Unknown platform
#endif

int sj_app_init(struct v7 *v7) {
  (void) v7;
  printf("Hello, world!\n");
  sj_gpio_set_mode(GPIO, GPIO_MODE_OUTPUT, GPIO_PULL_FLOAT);
  sj_gpio_write(GPIO, GPIO_LEVEL_HIGH);

  return 1;
}
