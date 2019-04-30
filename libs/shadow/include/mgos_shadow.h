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
 * Cross-cloud device cloud state API.
 */

#ifndef MGOS_SHADOW_H
#define MGOS_SHADOW_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "mgos_event.h"

#include "common/mg_str.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MGOS_SHADOW_BASE MGOS_EVENT_BASE('S', 'D', 'W')

/* In the comment, the type of `void *ev_data` is specified */
enum mgos_shadow_event {
  MGOS_SHADOW_GET = MGOS_SHADOW_BASE, /* NULL */
  MGOS_SHADOW_UPDATE,                 /* struct mgos_shadow_update_data */
  MGOS_SHADOW_CONNECTED,              /* NULL */
  MGOS_SHADOW_GET_ACCEPTED,           /* struct mg_str - shadow state */
  MGOS_SHADOW_GET_REJECTED,           /* struct mgos_shadow_error */
  MGOS_SHADOW_UPDATE_ACCEPTED,        /* NULL */
  MGOS_SHADOW_UPDATE_REJECTED,        /* struct mgos_shadow_error */
  MGOS_SHADOW_UPDATE_DELTA,           /* struct mg_str: shadow delta */
};

/* ev_data for MGOS_SHADOW_UPDATE event. */
struct mgos_shadow_update_data {
  uint64_t version;
  const char *json_fmt;
  va_list ap;
};

/* ev_data for MGOS_SHADOW_*_REJECTED events. */
struct mgos_shadow_error {
  int code;
  struct mg_str message;
};

/* Stringify shadow event name */
const char *mgos_shadow_event_name(int ev);

/*
 * Request shadow state. Response will arrive via GET_ACCEPTED topic.
 * Note that MGOS automatically does this on every (re)connect if
 * device.shadow.get_on_connect is true (default).
 */
bool mgos_shadow_get(void);

/*
 * Send an update. Format string should define the value of the "state" key,
 * i.e. it should be an object with an update to the reported state, e.g.:
 * `mgos_shadow_updatef("{foo: %d, bar: %d}", foo, bar)`.
 * Response will arrive via UPDATE_ACCEPTED or REJECTED topic.
 * If you want the update to be aplied only if a particular version is
 * current,
 * specify the version. Otherwise set it to 0 to apply to any version.
 */
bool mgos_shadow_updatef(uint64_t version, const char *state_jsonf, ...);

/* "Simple" version of mgos_shadow_updatef, primarily for FFI.  */
bool mgos_shadow_update(double version, const char *state_json);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_SHADOW_H */
