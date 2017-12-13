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

enum {
  MGOS_EVENT_INIT_DONE = MGOS_EVENT_SYS, /* ev_data: NULL*/
  MGOS_EVENT_LOG,    /* ev_data: struct mgos_debug_hook_arg */
  MGOS_EVENT_REBOOT, /* ev_data: NULL */
};

/* Register a base event number in order to prevent event number conflicts. */
bool mgos_event_register_base(int base_event_number, const char *name);

/* Trigger an event. Return number of event handlers invoked. */
int mgos_event_trigger(int ev, void *ev_data);

/* Event handler signature. */
typedef void (*mgos_event_handler_t)(int ev, void *ev_data, void *userdata);

/* Add an event handler. Return true on success, false on error (e.g. OOM). */
bool mgos_event_add_handler(int ev, mgos_event_handler_t cb, void *userdata);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_INCLUDE_MGOS_EVENT_H_ */
