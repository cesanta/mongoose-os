#include <stdio.h>

#include "common/platform.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_timers.h"

#if CS_PLATFORM == CS_P_ESP8266
/* On ESP-12E there is a blue LED connected to GPIO2 (aka U1TX). */
#define LED_GPIO 2
#define BUTTON_GPIO 0 /* Usually a "Flash" button. */
#define BUTTON_PULL MGOS_GPIO_PULL_UP
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_POS
#elif CS_PLATFORM == CS_P_ESP32
/* Unfortunately, there is no LED on DevKitC, so this is random GPIO. */
#define LED_GPIO 17
#define BUTTON_GPIO 0 /* Usually a "Flash" button. */
#define BUTTON_PULL MGOS_GPIO_PULL_UP
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_POS
#elif CS_PLATFORM == CS_P_CC3200
/* On CC3200 LAUNCHXL pin 64 is the red LED. */
#define LED_GPIO 64                     /* The red LED on LAUNCHXL */
#define BUTTON_GPIO 15                  /* SW2 on LAUNCHXL */
#define BUTTON_PULL MGOS_GPIO_PULL_NONE /* External pull-downs */
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_NEG
#elif(CS_PLATFORM == CS_P_STM32) && defined(BSP_NUCLEO_F746ZG)
/* Nucleo-144 F746 */
#define LED_GPIO STM32_PIN_PB7     /* Blue LED */
#define BUTTON_GPIO STM32_PIN_PC13 /* Blue user button */
#define BUTTON_PULL MGOS_GPIO_PULL_NONE
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_POS
#elif(CS_PLATFORM == CS_P_STM32) && defined(BSP_DISCO_F746G)
/* Discovery-0 F746 */
#define LED_GPIO STM32_PIN_PI1     /* Green LED */
#define BUTTON_GPIO STM32_PIN_PI11 /* Blue user button */
#define BUTTON_PULL MGOS_GPIO_PULL_NONE
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_POS
#else
#error Unknown platform
#endif

static void blink_timer_cb(void *arg) {
  bool current_level = mgos_gpio_toggle(LED_GPIO);
  LOG(LL_INFO, ("%s", (current_level ? "Tick" : "Tock")));
  (void) arg;
}

static void button_cb(int pin, void *arg) {
  LOG(LL_INFO, ("Click!"));
  (void) pin;
  (void) arg;
}

enum mgos_app_init_result mgos_app_init(void) {
  { /* Print a message using a value from config. */
    printf("Hello, %s!\n", get_cfg()->hello.who);
  }

  { /* Set up the blinky timer. */
    mgos_gpio_set_mode(LED_GPIO, MGOS_GPIO_MODE_OUTPUT);
    mgos_set_timer(1000 /* ms */, true /* repeat */, blink_timer_cb, NULL);
  }

  { /* Set up a button handler */
    mgos_gpio_set_button_handler(BUTTON_GPIO, BUTTON_PULL, BUTTON_EDGE,
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

  return MGOS_APP_INIT_SUCCESS;
}
