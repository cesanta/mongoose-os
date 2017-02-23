#include "fw/src/mgos_app.h"

#include <Arduino.h>

#include "common/cs_dbg.h"
#include "mongoose/mongoose.h"

#include "fw/src/Arduino/mgos_arduino.h"
#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_init.h"

void pinMode(uint8_t pin, uint8_t mode) {
  switch (mode) {
    case INPUT:
    case INPUT_PULLUP:
      mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_INPUT);
      mgos_gpio_set_pull(pin, (mode == INPUT_PULLUP ? MGOS_GPIO_PULL_UP
                                                    : MGOS_GPIO_PULL_NONE));
      break;
    case OUTPUT:
      mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT);
      break;
    default:
      LOG(LL_ERROR, ("Unsupported mode: %u", mode));
  }
}

int digitalRead(uint8_t pin) {
  return mgos_gpio_read(pin);
}

void digitalWrite(uint8_t pin, uint8_t val) {
  mgos_gpio_write(pin, val);
}

void delay(unsigned int ms) {
  mgos_usleep(ms * 1000);
}

void delayMicroseconds(unsigned int us) {
  mgos_usleep(us);
}

unsigned long millis(void) {
  return mg_time() * 1000;
}

unsigned long micros(void) {
  return mg_time() * 1000000;
}

extern "C" {

void loop_cb(void *arg) {
  loop();
  mgos_invoke_cb(loop_cb, NULL, false /* from_isr */);
  (void) arg;
}

enum mgos_init_result mgos_arduino_init(void) {
  setup();
  mgos_invoke_cb(loop_cb, NULL, false /* from_isr */);
  return MGOS_INIT_OK;
}
}
