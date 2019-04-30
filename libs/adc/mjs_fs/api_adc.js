let ADC = {
  // ## **`ADC.enable(pin)`**
  // Configure and enable ADC for a `pin`,
  // return 1 if success, 0 otherwise.
  enable: ffi('int mgos_adc_enable(int)'),
  // ## **`ADC.read(pin)`**
  // Read `pin` analog value, return an integer.
  //
  // Note for ESP8266 platform:
  // with this function, you can also measure the power voltage
  // of VDD33 pin 3 and 4. Then:
  // 1) TOUT pin has to be floating in the circuit
  // 	(not connected to anything);
  // 2) In mos.yaml must be set this feature:
  // 	build_vars:
  // 		MGOS_ADC_MODE_VDD: 1
  // 3) The return value may be different in different Wi-Fi modes,
  // 	for example, in Modem-sleep mode or in normal Wi-Fi working
  // 	mode.
  // Return value: Power voltage of VDD33; unit: 1/1024 V.
  read: ffi('int mgos_adc_read(int)'),
};
