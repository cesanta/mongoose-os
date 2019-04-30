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

#ifndef CS_MOS_LIBS_CRON_SRC_TEST_MGOS_LOCATION_H_
#define CS_MOS_LIBS_CRON_SRC_TEST_MGOS_LOCATION_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mgos_location_lat_lon {
  double lat;
  double lon;
};

bool mgos_location_get(struct mgos_location_lat_lon *loc);

/* TEST */
void location_set(double lat, double lon);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_CRON_SRC_TEST_MGOS_LOCATION_H_ */
