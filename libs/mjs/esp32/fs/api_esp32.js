let ESP32 = {
  // ## **`ESP32.temp()`**
  // Read built-in temperature sensor. Return value: integer.
  temp: ffi('int temprature_sens_read(void)'),

  // ## **`ESP32.hall()`**
  // Read built-in Hall sensor. Return value: integer.
  hall: ffi('int hall_sens_read(void)'),

  // ## **`ESP32.deepSleep(microseconds)`**
  // Deep Sleep given number of microseconds.
  // Return value: does not return.
  deepSleep: ffi('void mgos_esp_deep_sleep_d(double)'),
};
