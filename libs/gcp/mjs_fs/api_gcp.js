let GCP = {
  // ## **`GCP.isConnected()`**
  // Return value: true if GCP connection is up, false otherwise.
  isConnected: ffi('bool mgos_gcp_is_connected()'),

  sendEvent: ffi('bool mgos_gcp_send_eventp(struct mg_str *)'),
};
