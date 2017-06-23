let ESP32 = {
  // ## **`ESP32.temp()`**
  // Read built-in temperature sensor. Return value: integer.
  temp:  ffi('int temprature_sens_read(void)'),

  // ## **`ESP32.hall()`**
  // Read built-in Hall sensor. Return value: integer.
  hall:  ffi('int hall_sens_read(void)'),
};
