// ADC API.  Source C API is defined at:
// [mgos_adc.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_adc.h)

let ADC = {
  // ## **`ADC.read(pin)`**
  // Read `pin` analog value, return an integer.
  read: ffi('int mgos_adc_read(int)'),
};
