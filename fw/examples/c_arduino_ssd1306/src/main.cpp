/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

#include "common/cs_dbg.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_timers.h"

Adafruit_SSD1306 *d1 = nullptr, *d2 = nullptr;

static void timer_cb(void *arg);

void setup(void) {
  // I2C
  d1 = new Adafruit_SSD1306(4 /* RST GPIO */, Adafruit_SSD1306::RES_128_64);

  if (d1 != nullptr) {
    d1->begin(SSD1306_SWITCHCAPVCC, 0x3D, true /* reset */);
    d1->display();
  }

  // SPI.
  // On ESP8266 SPI and I2C cannot be enabled at the same time by default
  // because their pins overlap.
  // You will need to disable I2C and enable SPI:
  // mos config-set i2c.enable=false spi.enable=true
  d2 = new Adafruit_SSD1306(21 /* DC */, 17 /* RST */, 5 /* CS */,
                            Adafruit_SSD1306::RES_128_32);

  if (d2 != nullptr) {
    d2->begin(SSD1306_SWITCHCAPVCC, 0 /* unused */, true /* reset */);
    d2->display();
  }

  mgos_set_timer(1000 /* ms */, true /* repeat */, timer_cb, NULL);
}

static void show_num(Adafruit_SSD1306 *d, const char *s, int i) {
  d->clearDisplay();
  d->setTextSize(2);
  d->setTextColor(WHITE);
  d->setCursor(d->width() / 4, d->height() / 4);
  d->printf("%s%d", s, i);
  d->display();
}

static void timer_cb(void *arg) {
  static int i = 0, j = 0;
  if (d1 != nullptr) show_num(d1, "i = ", i);
  if (d2 != nullptr) show_num(d2, "j = ", j);
  LOG(LL_INFO, ("i = %d, j = %d", i, j));
  i++;
  j++;
  (void) arg;
}

#if 0
void loop(void) {
  /* For now, do not use delay() inside loop, use timers instead. */
}
#endif
