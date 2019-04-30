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
 * DHT sensor API.
 *
 * See https://learn.adafruit.com/dht/overview for more information.
 */

#ifndef CS_MOS_LIBS_DHT_INCLUDE_MGOS_DHT_H_
#define CS_MOS_LIBS_DHT_INCLUDE_MGOS_DHT_H_

#include <math.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Supported sensor types */
enum dht_type { DHT11 = 11, DHT21 = 21, AM2301 = 21, DHT22 = 22, AM2302 = 22 };

struct mgos_dht;
struct mgos_dht_stats {
  double last_read_time;         // value of mg_time() upon last call to _read()
  uint32_t read;                 // calls to _read()
  uint32_t read_success;         // successful _read()
  uint32_t read_success_cached;  // calls to _read() which were cached
  // Note: read_errors := read - read_success - read_success_cached
  double read_success_usecs;     // time spent in successful uncached _read()
};


/* Initialise DHT sensor. Return an opaque DHT handle, or `NULL` on error. */
struct mgos_dht *mgos_dht_create(int pin, enum dht_type type);

/* Close DHT handle. */
void mgos_dht_close(struct mgos_dht *dht);

/* Return temperature in DegC or 'NAN' on failure. */
float mgos_dht_get_temp(struct mgos_dht *dht);

/* Return humidity in % or 'NAN' on failure. */
float mgos_dht_get_humidity(struct mgos_dht *dht);

/*
 * Returns the running statistics on the sensor interaction, the user provides
 * a pointer to a `struct mgos_dht_stats` object, which is filled in by this
 * call.
 *
 * Upon success, true is returned. Otherwise, false is returned, in which case
 * the contents of `stats` is undetermined.
 */
bool mgos_dht_getStats(struct mgos_dht *dht, struct mgos_dht_stats *stats);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_DHT_INCLUDE_MGOS_DHT_H_ */
