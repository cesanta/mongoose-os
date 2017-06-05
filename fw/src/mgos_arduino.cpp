/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_app.h"

#include <Arduino.h>

#include "common/cs_dbg.h"
#include "mongoose/mongoose.h"

#include "fw/src/mgos_arduino.h"
#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_timers.h"

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

void interrupts(void) {
  mgos_ints_enable();
}

void noInterrupts(void) {
  mgos_ints_disable();
}

extern "C" {
static mgos_timer_id s_loop_timer;
}

void setup(void) __attribute__((weak));
void setup(void) {
}

void loop(void) __attribute__((weak));
void loop(void) {
  // It's a dummy loop, no need to invoke it.
  mgos_clear_timer(s_loop_timer);
}

extern "C" {
void loop_cb(void *arg) {
  loop();
  (void) arg;
}

enum mgos_init_result mgos_arduino_init(void) {
  setup();
  s_loop_timer = mgos_set_timer(0, true /* repeat */, loop_cb, NULL);
  return MGOS_INIT_OK;
}

}  // extern "C"
