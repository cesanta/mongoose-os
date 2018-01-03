/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

/*
 * Event API.
 *
 * Mongoose OS provides a way to get a notification when certain event
 * happens. Each event has an associated event data passed as `void *`.
 */

#ifndef CS_FW_INCLUDE_MGOS_EVENT_H_
#define CS_FW_INCLUDE_MGOS_EVENT_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
   * Fired on OTA status changes.
   *
   * ev_data: struct mgos_ota_status
   */
  MGOS_EVENT_OTA_STATUS,

  /*
   * Triggered when OTA needs to start, and one of the implementations handles
   * it and performs the OTA.
   *
   * ev_data: struct ota_request_param
   */
  MGOS_EVENT_OTA_REQUEST,

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
};

/* Parameter for the MGOS_EVENT_OTA_REQUEST event */
struct ota_request_param {
  char *location;
  void *updater_context;
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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_INCLUDE_MGOS_EVENT_H_ */
