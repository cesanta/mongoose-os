#include <stdio.h>

#include "common/platform.h"
#include "common/cs_file.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_timers.h"
#include "fw/src/mgos_hal.h"
#include "mjs.h"

#if CS_PLATFORM == CS_P_ESP8266
#define LED_GPIO 2 /* On ESP-12E there is a blue LED connected to GPIO2  */
#elif CS_PLATFORM == CS_P_ESP32
#define LED_GPIO 17 /* No LED on DevKitC, use random GPIO. */
#elif CS_PLATFORM == CS_P_CC3200
#define LED_GPIO 64 /* The red LED on LAUNCHXL */
#elif CS_PLATFORM == CS_P_MBED
#define LED_GPIO 0x6D
#else
#error Unknown platform
#endif

int get_led_gpio_pin(void) {
  return LED_GPIO;
}

/* Manual symbol resolver */
void *my_dlsym(void *handle, const char *name) {
  (void) handle;

#define EXPORT(s)                        \
  do {                                   \
    if (strcmp(name, #s) == 0) return s; \
  } while (0)

  EXPORT(get_led_gpio_pin);
  EXPORT(mgos_gpio_set_mode);
  EXPORT(mgos_gpio_toggle);
  EXPORT(mgos_gpio_read);
  EXPORT(mgos_gpio_write);
  EXPORT(mgos_set_timer);
  return NULL;
}

enum mgos_app_init_result mgos_app_init(void) {
  /* Initialize JavaScript engine */
  struct mjs *mjs = mjs_create();
  mjs_set_ffi_resolver(mjs, my_dlsym);
  mjs_err_t err = mjs_exec_file(mjs, "init.js", NULL);
  if (err != MJS_OK) {
    LOG(LL_ERROR, ("MJS exec error: %s\n", mjs_strerror(mjs, err)));
  }

  return MGOS_APP_INIT_SUCCESS;
}
