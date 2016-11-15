#include <stdio.h>

#include "common/platform.h"
#include "fw/src/miot_app.h"
#include "fw/src/miot_gpio.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_timers.h"

#if CS_PLATFORM == CS_P_ESP8266
/* On ESP-12E there is a blue LED connected to GPIO2 (aka U1TX). */
#define GPIO 2
#elif CS_PLATFORM == CS_P_CC3200
/* On CC3200 LAUNCHXL pin 64 is the red LED. */
#define GPIO 64 /* The red LED on LAUNCHXL */
#else
#error Unknown platform
#endif

static void blink_timer_cb(void *arg) {
  static enum gpio_level l = GPIO_LEVEL_LOW;
  if (l == GPIO_LEVEL_LOW) {
    LOG(LL_INFO, ("Tick"));
    l = GPIO_LEVEL_HIGH;
  } else {
    LOG(LL_INFO, ("Tock"));
    l = GPIO_LEVEL_LOW;
  }
  miot_gpio_write(GPIO, l);
  (void) arg;
}

enum miot_app_init_result miot_app_init(void) {
  { /* Print a message using a value from config. */
    printf("Hello, %s!\n", get_cfg()->hello.who);
  }

  { /* Set up the blinky timer. */
    miot_gpio_set_mode(GPIO, GPIO_MODE_OUTPUT, GPIO_PULL_FLOAT);
    miot_set_c_timer(1000 /* ms */, true /* repeat */, blink_timer_cb, NULL);
  }

  { /* Read a file. */
    FILE *fp = fopen("README.txt", "r");
    if (fp != NULL) {
      char buf[100];
      int n = fread(buf, 1, sizeof(buf), fp);
      if (n > 0) {
        fwrite(buf, 1, n, stdout);
      }
      fclose(fp);
    }
  }

  return MIOT_APP_INIT_SUCCESS;
}
