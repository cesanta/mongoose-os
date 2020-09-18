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
 * Mongoose OS provides a way to get a notification when certain event
 * happens. Each event has an associated event data passed as `void *`.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Macro to generate unique base event number.
 * A library can use the last byte (LSB) to create up to 256 unique
 * events (see enum below on how Mongoose OS core defines its events).
 * A library should call mgos_event_register_base() in order to claim
 * it and prevent event number conflicts.
 */
#define MGOS_EVENT_BASE(a, b, c) ((a) << 24 | (b) << 16 | (c) << 8)

/*
 * These events are registered by the MGOS core.
 * Other events could be registered by the external libraries.
 */
#define MGOS_EVENT_SYS MGOS_EVENT_BASE('M', 'O', 'S')

/*
 * System mos events
 */
enum mgos_event_sys {
  /*
   * Fired when all core modules and libs are initialized (Right after printing
   * `Init done` to the console).
   *
   * ev_data: NULL
   */
  MGOS_EVENT_INIT_DONE = MGOS_EVENT_SYS,

  /*
   * Fired when anything is printed to the debug console, see `struct
   * mgos_debug_hook_arg`
   *
   * ev_data: struct mgos_debug_hook_arg
   */
  MGOS_EVENT_LOG,

  /*
   * Fired right before restarting the system (but also before unmounting
   * filesystems, disconnecting from the wifi, etc)
   *
   * ev_data: NULL
   */
  MGOS_EVENT_REBOOT,

  /*
   * Fired when time is changed with `mgos_settimeofday()`.
   *
   * ev_data: `struct mgos_time_changed_arg`.
   *
   * Example:
   * ```c
   * static void my_time_change_cb(int ev, void *evd, void *arg) {
   *   struct mgos_time_changed_arg *ev_data = (struct mgos_time_changed_arg *)
   *evd;
   *   LOG(LL_INFO, ("Time has changed by %d", ev_data->delta));
   *
   *   (void) ev;
   *   (void) arg;
   * }
   *
   * // ...
   *
   * mgos_event_add_handler(MGOS_EVENT_TIME_CHANGED, my_time_change_cb, NULL);
   * ```
   */
  MGOS_EVENT_TIME_CHANGED,

  /*
   * Fired when device is connected/disconnected to the cloud server.
   * ev_data: `struct mgos_cloud_arg`
   */
  MGOS_EVENT_CLOUD_CONNECTED,
  MGOS_EVENT_CLOUD_DISCONNECTED,
  MGOS_EVENT_CLOUD_CONNECTING,

  /*
   * Fired when a reboot is scheduled after certain time.
   *
   * ev_data: struct mgos_event_reboot_after_arg
   */
  MGOS_EVENT_REBOOT_AFTER,
};

/* Parameter for the MGOS_EVENT_CLOUD_* events */
enum mgos_cloud_type {
  MGOS_CLOUD_MQTT,
  MGOS_CLOUD_DASH,
  MGOS_CLOUD_AWS,
  MGOS_CLOUD_AZURE,
  MGOS_CLOUD_GCP,
  MGOS_CLOUD_WATSON,
};
struct mgos_cloud_arg {
  enum mgos_cloud_type type;
};

/* Arg for MGOS_EVENT_REBOOT_AFTER */
struct mgos_event_reboot_after_arg {
  int64_t reboot_at_uptime_micros;
};

/*
 * Register a base event number in order to prevent event number conflicts.
 * Use `MGOS_EVENT_BASE()` macro to get `base_event_number`; `name` is an
 * arbitrary name of the module who registers the base number.
 *
 * Example:
 * ```c
 * #define MY_EVENT_BASE MGOS_EVENT_BASE('F', 'O', 'O')
 *
 * enum my_event {
 *   MY_EVENT_AAA = MY_EVENT_BASE,
 *   MY_EVENT_BBB,
 *   MY_EVENT_CCC,
 * };
 *
 * // And somewhere else:
 * mgos_event_register_base(MY_EVENT_BASE, "my module foo");
 * ```
 */
bool mgos_event_register_base(int base_event_number, const char *name);

/* Trigger an event `ev` with the event data `ev_data`. Return number of event
 * handlers invoked. */
int mgos_event_trigger(int ev, void *ev_data);

/* Event handler signature. */
typedef void (*mgos_event_handler_t)(int ev, void *ev_data, void *userdata);

/*
 * Add an event handler. Return true on success, false on error (e.g. OOM).
 *
 * Example:
 * ```c
 * static void system_restart_cb(int ev, void *ev_data, void *userdata) {
 *   LOG(LL_INFO, ("Going to reboot!"));
 *   (void) ev;
 *   (void) ev_data;
 *   (void) userdata;
 * }
 *
 * // And somewhere else:
 * mgos_event_add_handler(MGOS_EVENT_REBOOT, system_restart_cb, NULL);
 * ```
 */
bool mgos_event_add_handler(int ev, mgos_event_handler_t cb, void *userdata);

/*
 * Like `mgos_event_add_handler()`, but subscribes on all events in the given
 * group `evgrp`. Event group includes all events from `evgrp & ~0xff` to
 * `evgrp | 0xff`.
 *
 * Example:
 * ```c
 * static void sys_event_cb(int ev, void *ev_data, void *userdata) {
 *   LOG(LL_INFO, ("Got system event %d", ev));
 *   (void) ev;
 *   (void) ev_data;
 *   (void) userdata;
 * }
 *
 * // And somewhere else:
 * mgos_event_add_handler(MGOS_EVENT_SYS, sys_event_cb, NULL);
 * ```
 */
bool mgos_event_add_group_handler(int evgrp, mgos_event_handler_t cb,
                                  void *userdata);

/*
 * Remove an event handler.
 * Both cb and userdata must match the initial add invocation.
 * Returns true if a handler was found and removed, false if there was no
 * such handler in the first place.
 */
bool mgos_event_remove_handler(int ev, mgos_event_handler_t cb, void *userdata);
bool mgos_event_remove_group_handler(int evgrp, mgos_event_handler_t cb,
                                     void *userdata);

#ifdef __cplusplus
}
#endif
