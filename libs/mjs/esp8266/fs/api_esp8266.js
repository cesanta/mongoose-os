let ESP8266 = {
  // ## **`ESP8266.deepSleep(microseconds)`**
  // Deep Sleep given number of microseconds.
  // Return value: false on error, otherwise does not return.
  deepSleep: ffi('int mgos_system_deep_sleep_d(double)'),
};
