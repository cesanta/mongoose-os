let OTA = {
  // ## **`OTA.evdataOtaStatusMsg(evdata)`**
  // Getter function for the `evdata` given to the event callback for the event
  // `Event.OTA_STATUS`, see `Event.addHandler()` in `api_events.js`.
  evdataOtaStatusMsg: ffi('char *mgos_ota_status_get_msg(void *)'),
};
