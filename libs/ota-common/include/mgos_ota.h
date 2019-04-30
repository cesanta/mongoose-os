/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * OTA API.
 *
 * See https://mongoose-os.com/docs/mongoose-os/userguide/ota.md for more
 * details about Mongoose OS OTA mechanism.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/mg_str.h"

#include "mgos_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_EVENT_OTA_BASE MGOS_EVENT_BASE('O', 'T', 'A')
enum mgos_event_ota {
  MGOS_EVENT_OTA_BEGIN =
      MGOS_EVENT_OTA_BASE, /* ev_data: struct mgos_ota_begin_arg */
  MGOS_EVENT_OTA_STATUS,   /* ev_data: struct mgos_ota_status */
};

struct mgos_ota_file_info {
  char name[50];
  uint32_t size;
  uint32_t processed;
  uint32_t crc32;
  /* Part that corresponds to this file (JSON). */
  struct mg_str part;
};

struct mgos_ota_manifest_info {
  /* Entire manifest, as JSON. */
  struct mg_str manifest;
  /* Some fields, pre-parsed from the manifest. */
  struct mg_str name;
  struct mg_str platform;
  struct mg_str version;
  struct mg_str build_id;
  struct mg_str parts;
};

enum mgos_ota_result {
  MGOS_UPD_WAIT = 0,
  MGOS_UPD_OK,
  MGOS_UPD_SKIP,
  MGOS_UPD_ABORT,
};

struct mgos_ota_begin_arg {
  struct mgos_ota_manifest_info mi;
  /*
   * The default is to continue but handler may set ABORT or WAIT.
   * In case or ABORT, the update is aborted immediately.
   * In case of WAIT, updater will pause the update and periodically
   * re-raise the event until the action is OK or ABORT.
   * Since there can be multiple handlers, be careful:
   *  - If you'd like to continue, don't touch the field at all.
   *  - If you wait to wait, check it's not ABORT (set by somebody else).
   */
  enum mgos_ota_result result;
};

enum mgos_ota_state {
  MGOS_OTA_STATE_IDLE = 0, /* idle */
  MGOS_OTA_STATE_PROGRESS, /* "progress" */
  MGOS_OTA_STATE_ERROR,    /* "error" */
  MGOS_OTA_STATE_SUCCESS,  /* "success" */
};

struct mgos_ota_status {
  bool is_committed;
  int commit_timeout;
  int partition;
  enum mgos_ota_state state;
  const char *msg;      /* stringified state */
  int progress_percent; /* valid only for "progress" state */
};

struct mgos_ota_end_arg {
  int result;
  const char *message;
};

struct mgos_ota_opts {
  int timeout;
  int commit_timeout;
  bool ignore_same_version;
};

const char *mgos_ota_state_str(enum mgos_ota_state state);

void mgos_ota_boot_finish(bool is_successful, bool is_first);
bool mgos_ota_commit();
bool mgos_ota_is_in_progress(void);
bool mgos_ota_is_committed();
bool mgos_ota_is_first_boot(void);
bool mgos_ota_revert(bool reboot);
bool mgos_ota_get_status(struct mgos_ota_status *);
/* Apply update on first boot, usually involves merging filesystem. */
int mgos_ota_apply_update(void);

int mgos_ota_get_commit_timeout(void);
bool mgos_ota_set_commit_timeout(int commit_timeout);

#ifdef __cplusplus
}
#endif
