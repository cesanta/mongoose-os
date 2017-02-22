// ADC API.  Source C API is defined at:
// [mgos_adc.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_adc.h)

let ADC = {
  // **`ADC.read(pin)`** - read pin analog value
  read: ffi('int mgos_adc_read(int)'),
};
