#include <stdio.h>

#include "common/platform.h"
#include "fw/src/miot_app.h"
#include "fw/src/miot_gpio.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_timers.h"

#if CS_PLATFORM == CS_P_ESP8266
/* On ESP-12E there is a blue LED connected to GPIO2 (aka U1TX). */
#define LED_GPIO 2
#define BUTTON_GPIO 0 /* Usually a "Flash" button. */
#define BUTTON_PULL MIOT_GPIO_PULL_UP
#define BUTTON_EDGE MIOT_GPIO_INT_EDGE_POS
#elif CS_PLATFORM == CS_P_ESP32
/* Unfortunately, there is no LED on DevKitC, so this is random GPIO. */
#define LED_GPIO 17
#define BUTTON_GPIO 0 /* Usually a "Flash" button. */
#define BUTTON_PULL MIOT_GPIO_PULL_UP
#define BUTTON_EDGE MIOT_GPIO_INT_EDGE_POS
#elif CS_PLATFORM == CS_P_CC3200
/* On CC3200 LAUNCHXL pin 64 is the red LED. */
#define LED_GPIO 64                     /* The red LED on LAUNCHXL */
#define BUTTON_GPIO 15                  /* SW2 on LAUNCHXL */
#define BUTTON_PULL MIOT_GPIO_PULL_NONE /* External pull-downs */
#define BUTTON_EDGE MIOT_GPIO_INT_EDGE_NEG
#elif CS_PLATFORM == CS_P_MBED
#define LED_GPIO 0x6D
#define BUTTON_GPIO 0
#define BUTTON_PULL MIOT_GPIO_PULL_NONE
#define BUTTON_EDGE MIOT_GPIO_INT_EDGE_POS
#else
#error Unknown platform
#endif

static void blink_timer_cb(void *arg) {
  bool current_level = miot_gpio_toggle(LED_GPIO);
  LOG(LL_INFO, ("%s", (current_level ? "Tick" : "Tock")));
  (void) arg;
}

static void button_cb(int pin, void *arg) {
  LOG(LL_INFO, ("Click!"));
  (void) pin;
  (void) arg;
}

enum miot_app_init_result miot_app_init(void) {
  { /* Print a message using a value from config. */
    printf("Hello, %s!\n", get_cfg()->hello.who);
  }

  { /* Set up the blinky timer. */
    miot_gpio_set_mode(LED_GPIO, MIOT_GPIO_MODE_OUTPUT);
    miot_set_timer(1000 /* ms */, true /* repeat */, blink_timer_cb, NULL);
  }

  { /* Set up a button handler */
    miot_gpio_set_button_handler(BUTTON_GPIO, BUTTON_PULL, BUTTON_EDGE,
                                 50 /* debounce_ms */, button_cb, NULL);
  }

  { /* Read a file. */
    FILE *fp = fopen("README.txt", "r");
    if (fp != NULL) {
      char buf[100] = {0};
      int n = fread(buf, 1, sizeof(buf), fp);
      if (n > 0) {
        printf("%s\n", buf);
      }
      fclose(fp);
    }
  }

  return MIOT_APP_INIT_SUCCESS;
}
