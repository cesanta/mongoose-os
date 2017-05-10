/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

#include "common/cs_dbg.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_timers.h"

Adafruit_SSD1306 *display = nullptr;

static void timer_cb(void *arg);

void setup(void) {
  // I2C
  display = new Adafruit_SSD1306(5 /* RST GPIO */);

  // SPI.
  // On ESP8266 SPI and I2C cannot be enabled at the same time by default
  // because their pins overlap.
  // You will need to disable I2C and enable SPI:
  // mos config-set i2c.enable=false spi.enable=true
  // display = new Adafruit_SSD1306(21 /* DC */, 5 /* RST */, 17 /* CS */);

  display->begin(SSD1306_SWITCHCAPVCC);
  display->display();  // Displays the Adafruit logo.

  mgos_set_timer(1000 /* ms */, true /* repeat */, timer_cb, NULL);
}

static void timer_cb(void *arg) {
  static int i = 0;
  display->clearDisplay();
  display->setTextSize(2);
  display->setTextColor(WHITE);
  display->setCursor(32, 8);
  display->printf("i = %d", i);
  display->display();
  LOG(LL_INFO, ("i = %d", i));
  i++;
  (void) arg;
}

#if 0
void loop(void) {
  /* For now, do not use delay() inside loop, use timers instead. */
}
#endif
