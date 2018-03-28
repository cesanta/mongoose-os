/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * OTA API.
 *
 * See https://mongoose-os.com/docs/book/ota.html for more details about
 * Mongoose OS OTA mechanism.
 */

#ifndef CS_FW_SRC_MGOS_UPDATER_H_
#define CS_FW_SRC_MGOS_UPDATER_H_

#include <stdbool.h>

#include "frozen.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mgos_upd_file_info {
  char name[50];
  uint32_t size;
  uint32_t processed;
};

struct mgos_upd_info {
  /* Data from the manifest, available from BEGIN until END */
  struct json_token name;
  struct json_token platform;
  struct json_token version;
  struct json_token build_id;
  struct json_token parts;

  /* Current file, available in PROGRESS. */
  struct mgos_upd_file_info current_file;
};

enum mgos_upd_event {
  /* ev_data = NULL */
  MGOS_UPD_EV_INIT = 1,
  /* ev_data = const struct mgos_upd_info * */
  MGOS_UPD_EV_BEGIN = 2,
  /* ev_data = const struct mgos_upd_info * */
  MGOS_UPD_EV_PROGRESS = 3,
  /* ev_data = struct update_context * */
  MGOS_UPD_EV_END = 4,
  /* ev_data = NULL */
  MGOS_UPD_EV_ROLLBACK = 5,
  /* ev_data = NULL */
  MGOS_UPD_EV_COMMIT = 6,
  /* ev_data = NULL */
  MGOS_UPD_EV_ERROR = 7,
};

/*
 * User application can register a callback on FW update events.
 * An event is dispatched to the callback at various stages:
 *  - INIT: the very beginning of an update, nothing is known yet.
 *  - BEGIN: Update manifest has been parsed, metadata fields are known.
 *  - PROGRESS: Invoked repeatedly as files are being processed.
 *  - END: Invoked at the end, with the overall result of the update.
 *
 * At the INIT and BEGIN stage the app can interfere and decline an update
 * by returning false from the callback.
 */

typedef bool (*mgos_upd_event_cb)(enum mgos_upd_event ev, const void *ev_arg,
                                  void *cb_arg);

/*
 * Set update event callback.
 *
 * Example code:
```
bool upd_cb(enum mgos_upd_event ev, const void *ev_arg, void *cb_arg) {
  switch (ev) {
    case MGOS_UPD_EV_INIT: {
      LOG(LL_INFO, ("INIT"));
      return true;
    }
    case MGOS_UPD_EV_BEGIN: {
      const struct mgos_upd_info *info = (const struct mgos_upd_info *) ev_arg;
      LOG(LL_INFO, ("BEGIN %.*s", (int) info->build_id.len,
                    info->build_id.ptr));
      return true;
    }
    case MGOS_UPD_EV_PROGRESS: {
      const struct mgos_upd_info *info = (const struct mgos_upd_info *) ev_arg;
      LOG(LL_INFO, ("Progress: %s %d of %d", info->current_file.name,
            info->current_file.processed, info->current_file.size));
      break;
    }
    case MGOS_UPD_EV_END: {
      int result = *((int *) ev_arg);
      LOG(LL_INFO, ("END, result %d", result));
      break;
    }
  }
  (void) cb_arg;
  return false;
}
```
*/
void mgos_upd_set_event_cb(mgos_upd_event_cb cb, void *cb_arg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_UPDATER_H_ */
