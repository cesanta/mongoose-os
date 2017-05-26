let ExtraFuncs = {
  // ## **`Sys.deepsleep(microseconds)`**
  // Deep Sleep given number of microseconds.
  // Return value: none.
  deepsleep:  ffi('void system_deep_sleep(int)'),
  disconnectwifi:  ffi('void mgos_wifi_disconnect()'),
  connectwifi:  ffi('void mgos_wifi_connect()')
};
