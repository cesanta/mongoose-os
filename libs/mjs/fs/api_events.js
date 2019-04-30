let Event = {
  // ## **`Event.addHandler(ev, callback, userdata)`**
  // Add a handler for the given event `ev`. Callback should look like:
  //
  // function(ev, evdata, userdata) { /* ... */ }
  //
  // Example:
  // ```javascript
  //
  // Event.addHandler(Event.REBOOT, function(ev, evdata, ud) {
  //   print("Going to reboot!");
  // }, null);
  // ```
  addHandler: ffi(
      'bool mgos_event_add_handler(int, void(*)(int, void *, userdata), userdata)'),

  // ## **`Event.addGroupHandler(evgrp, callback, userdata)`**
  // Like `Event.addHandler()`, but subscribes on all events in the given
  // event group `evgrp`. Event group includes all events from `evgrp & ~0xff`
  // to `evgrp | 0xff`.
  //
  // Example:
  // ```javascript
  //
  // Event.addGroupHandler(Event.SYS, function(ev, evdata, ud) {
  //   print("Sys event:", ev);
  // }, null);
  // ```
  addGroupHandler: ffi(
      'bool mgos_event_add_group_handler(int, void(*)(int, void *, userdata), userdata)'),

  // ## **`Event.on(event_num, callback, userdata)`**
  // Alias for Event.addHandler
  on: function(ev, cb, cbdata) {
    this.addHandler(ev, cb, cbdata);
    return this;
  },

  // ## **`Event.regBase(base_event_number, name)`**
  // Register a base event number in order to prevent event number conflicts.
  // Use `Event.baseNumber(id)` to get `base_event_number`; `name` is an
  // arbitrary event name.
  //
  // Example:
  // ```javascript
  // let bn = Event.baseNumber("ABC");
  // if (!Event.regBase(bn, "My module")) {
  //   die("Failed to register base event number");
  // }
  //
  // let MY_EVENT_FOO = bn + 0;
  // let MY_EVENT_BAR = bn + 1;
  // let MY_EVENT_BAZ = bn + 2;
  // ```
  regBase: ffi('bool mgos_event_register_base(int, char *)'),

  // ## **`Event.baseNumber(id)`**
  // Generates unique base event number (32-bit) by a 3-char string.
  // LSB is always zero, and a library can use it to create up to 256 unique
  // events.
  //
  // A library should call `Event.regBase()` in order to claim
  // it and prevent event number conflicts. (see example there)
  baseNumber: function(id) {
    if (id.length !== 3) {
      die('Base event id should have exactly 3 chars');
      return -1;
    }

    return (id.at(0) << 24) | (id.at(1) << 16) | (id.at(2) << 8);
  },

  // ## **`Event.trigger(ev, evdata)`**
  // Trigger an event with the given id `ev` and event data `evdata`.
  trigger: ffi('int mgos_event_trigger(int, void *)'),

  // ## **`Event.evdataLogStr(evdata)`**
  // Getter function for the `evdata` given to the event callback for the event
  // `Event.LOG`, see `Event.addHandler()`.
  evdataLogStr: function(evdata) {
    return mkstr(Event._gdd(evdata), 0, Event._gdl(evdata), true);
  },

  _gdd: ffi('void *mgos_debug_event_get_ptr(void *)'),
  _gdl: ffi('int mgos_debug_event_get_len(void *)'),
};

Event.SYS = Event.baseNumber('MOS');

// NOTE: INIT_DONE is unavailable here because init.js is executed in
// INIT_DONE hook

// ## **`Event.LOG`**
// System event which is triggered every time something is printed to the
// log.  In the callback, use `Event.evdataLogStr(evdata)` to get string
// which was printed.
Event.LOG = Event.SYS + 1;

// ## **`Event.REBOOT`**
// System event which is triggered right before going to reboot. `evdata`
// is irrelevant for this event.
Event.REBOOT = Event.SYS + 2;

// ## **`Event.OTA_STATUS`**
// System event which is triggered when OTA status changes.
//
// In the callback, use `OTA.evdataOtaStatusMsg(evdata)` from `api_ota.js` to
// get the OTA status message.
Event.OTA_TIME_CHANGED = Event.SYS + 3;

// ## **`Event.CLOUD_CONNECTED`**
// Triggered when device is connected to the cloud (mqtt, dash)
Event.CLOUD_CONNECTED = Event.SYS + 4;

// ## **`Event.CLOUD_DISCONNECTED`**
// Triggered when device is disconnected from the cloud
Event.CLOUD_DISCONNECTED = Event.SYS + 5;
