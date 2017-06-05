let ESP8266 = {
  // ## **`Sys.deepSleep(microseconds)`**
  // Deep Sleep given number of microseconds.
  // Return value: none.
  deepSleep:  ffi('void system_deep_sleep(int)'),
};
