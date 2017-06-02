#include <stdio.h>

#include "common/platform.h"
#include "common/cs_file.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_timers.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_dlsym.h"
#include "mjs.h"

#if CS_PLATFORM == CS_P_ESP8266
#define LED_GPIO 2 /* On ESP-12E there is a blue LED connected to GPIO2  */
#elif CS_PLATFORM == CS_P_ESP32
#define LED_GPIO 21 /* No LED on DevKitC, use random GPIO close to GND pin */
#elif CS_PLATFORM == CS_P_CC3200
#define LED_GPIO 64 /* The red LED on LAUNCHXL */
#elif(CS_PLATFORM == CS_P_STM32) && defined(BSP_NUCLEO_F746ZG)
/* Nucleo-144 F746 */
#define LED_GPIO STM32_PIN_PB7 /* Blue LED */
#elif(CS_PLATFORM == CS_P_STM32) && defined(BSP_DISCO_F746G)
/* Discovery-0 F746 */
#define LED_GPIO STM32_PIN_PI1 /* Green LED */
#else
#error Unknown platform
#endif

int get_led_gpio_pin(void) {
  return LED_GPIO;
}

enum mgos_app_init_result mgos_app_init(void) {
  /* Initialize JavaScript engine */
  int mem1, mem2, mem3;
  mem1 = mgos_get_free_heap_size();
  struct mjs *mjs = mjs_create();
  mem2 = mgos_get_free_heap_size();
  mjs_set_ffi_resolver(mjs, mgos_dlsym);
  mjs_err_t err = mjs_exec_file(mjs, "init.js", 1, NULL);
  if (err != MJS_OK) {
    mjs_print_error(mjs, stdout, NULL, 1 /* print_stack_trace */);
  }
  mem3 = mgos_get_free_heap_size();
  LOG(LL_INFO, ("mJS memory stat: before init: %d "
                "after init: %d after init.js: %d",
                mem1, mem2, mem3));
  return MGOS_APP_INIT_SUCCESS;
}
