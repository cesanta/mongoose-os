/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * View this file on GitHub:
 * [mgos_dht.c](https://github.com/cesanta/mongoose-os/libs/dht/blob/master/src/mgos_dht.c)
 */

#include <stdint.h>
#include "mgos_dht.h"
#include "mgos_gpio.h"
#include "mgos_hal.h"
#include "mongoose.h"

#ifndef IRAM
#define IRAM
#endif

#define MGOS_DHT_READ_DELAY (2)

struct mgos_dht {
  int pin;
  enum dht_type type;
  uint8_t data[5];
  bool last_result;
  struct mgos_dht_stats stats;
};

IRAM static bool dht_wait(int pin, int lvl, uint32_t usecs) {
  uint32_t t = 0;
  while (mgos_gpio_read(pin) != lvl) {
    if (t == usecs) {
      t = 0;
      break;
    }
    mgos_usleep(1);
    t++;
  }
  return t != 0;
}

IRAM static bool dht_read(struct mgos_dht *dht) {
  if (dht == NULL) return false;
  double start = mg_time();
  dht->stats.read++;
  if ((start - dht->stats.last_read_time) < MGOS_DHT_READ_DELAY) {
    dht->stats.read_success_cached++;
    return dht->last_result;
  }
  dht->stats.last_read_time = start;
  dht->last_result = false;
  memset(dht->data, 0, 5);

  mgos_gpio_set_mode(dht->pin, MGOS_GPIO_MODE_INPUT);
  mgos_gpio_set_pull(dht->pin, MGOS_GPIO_PULL_UP);
  mgos_msleep(10);

  /* Send start signal at least 10ms (18ms for DHT11) to ensure
     sensor could detect it */
  mgos_gpio_set_mode(dht->pin, MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_write(dht->pin, 0);
  mgos_msleep(18);

  /* Enter critical section */
  mgos_ints_disable();
  /* Pulls up the data bus and wait 20-40us for sensors response */
  mgos_gpio_set_mode(dht->pin, MGOS_GPIO_MODE_INPUT);
  mgos_gpio_set_pull(dht->pin, MGOS_GPIO_PULL_UP);
  mgos_usleep(40);

  /* The sensor sets low the bus 80us as response signal,
     then sets high 80us for preparation to send data */
  if (!dht_wait(dht->pin, 1, 90) || !dht_wait(dht->pin, 0, 90)) {
    mgos_ints_enable();
    return false;
  }

  /* Wait a rising edge and sleep after that 50 us.
     If a pin is in low state, then it was a pulse 26-28 us (0),
     otherwise - ~ 70us (1) */
  mgos_usleep(10);
  for (int i = 0, j; i < 40; i++) {
    if (!dht_wait(dht->pin, 1, 50)) {
      mgos_ints_enable();
      return false;
    }
    mgos_usleep(50);
    j = i / 8;
    dht->data[j] <<= 1;
    if (mgos_gpio_read(dht->pin)) {
      dht->data[j] |= 1;
      mgos_usleep(50);
    }
  }
  /* Exit critical section */
  mgos_ints_enable();

  if (dht->data[4] ==
      ((dht->data[0] + dht->data[1] + dht->data[2] + dht->data[3]) & 0xFF)) {
    dht->last_result = true;
    dht->stats.read_success++;
    dht->stats.read_success_usecs+=1000000*(mg_time()-start);
    dht->stats.last_read_time=start;
  }
  return dht->last_result;
}

struct mgos_dht *mgos_dht_create(int pin, enum dht_type type) {
  struct mgos_dht *dht = calloc(1, sizeof(*dht));
  if (dht == NULL) return NULL;
  memset(dht, 0, sizeof(struct mgos_dht));
  dht->pin = pin;
  dht->type = type;
  if (!mgos_gpio_set_mode(dht->pin, MGOS_GPIO_MODE_INPUT) ||
      !mgos_gpio_set_pull(dht->pin, MGOS_GPIO_PULL_UP)) {
    mgos_dht_close(dht);
    return NULL;
  }
  return dht;
}

void mgos_dht_close(struct mgos_dht *dht) {
  free(dht);
  dht = NULL;
}

float mgos_dht_get_temp(struct mgos_dht *dht) {
  float res = NAN;

  if (dht_read(dht)) {
    switch (dht->type) {
      case DHT11:
        res = dht->data[2];
        break;
      case DHT22:
      case DHT21:
        res = dht->data[2] & 0x7F;
        res *= 256;
        res += dht->data[3];
        res *= 0.1;
        if (dht->data[2] & 0x80) res *= -1;
        break;
    }
  }
  return res;
}

float mgos_dht_get_humidity(struct mgos_dht *dht) {
  float res = NAN;

  if (dht_read(dht)) {
    switch (dht->type) {
      case DHT11:
        res = dht->data[0];
        break;
      case DHT22:
      case DHT21:
        res = dht->data[0];
        res *= 256;
        res += dht->data[1];
        res *= 0.1;
        break;
    }
  }
  return res;
}

bool mgos_dht_getStats(struct mgos_dht *dht, struct mgos_dht_stats *stats) {
  if (!dht || !stats)
    return false;

  memcpy((void *)stats, (const void *)&dht->stats, sizeof(struct mgos_dht_stats));
  return true;
}

bool mgos_dht_init(void) {
  return true;
}
