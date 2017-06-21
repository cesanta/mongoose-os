let ADC = {
  // ## **`ADC.read(pin)`**
  // Read `pin` analog value, return an integer.
  read: ffi('int mgos_adc_read(int)'),
};
