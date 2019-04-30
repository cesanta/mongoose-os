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

#ifndef CS_MOS_LIBS_WATSON_INCLUDE_MGOS_WATSON_H_
#define CS_MOS_LIBS_WATSON_INCLUDE_MGOS_WATSON_H_

#include <stdint.h>

#include "common/mg_str.h"

#include "mgos_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_WATSON_EV_BASE MGOS_EVENT_BASE('I', 'B', 'M')

/* In the comment, the type of `void *ev_data` is specified */
enum mgos_watson_event {
  /* Connected to the Azure cloud. Arg: NULL */
  MGOS_WATSON_EV_CONNECT = MGOS_WATSON_EV_BASE,
  /* Disonnected from the cloud. Arg: NULL */
  MGOS_WATSON_EV_CLOSE,
};

/* Returns true if Watson connection is up, false otherwise. */
bool mgos_watson_is_connected(void);

/*
 * Send an event, in JSON format.
 * The message should be an object with a "d" key and properties, e.g.:
 * `mgos_watson_send_eventf("{d: {foo: %d}}", foo);`
 */
bool mgos_watson_send_event_jsonf(const char *event_id, const char *json_fmt,
                                  ...);
bool mgos_watson_send_event_jsonp(const struct mg_str *event_id,
                                  const struct mg_str *body);

#ifdef __cplusplus
}
#endif

#endif /* CS_MOS_LIBS_WATSON_INCLUDE_MGOS_WATSON_H_ */
