// Arduino DallasTemperature library API. Source C API is defined at:
// [mgos_arduino_dallas_temp.h](https://github.com/cesanta/mongoose-os/blob/master/arduino_drivers/Arduino/mgos_arduino_dallas_temp.h)

let DallasTemperature = {
  // Error codes
  DEVICE_DISCONNECTED_C: -127.0,
  DEVICE_DISCONNECTED_F: -196.6,
  DEVICE_DISCONNECTED_RAW: -7040,

  _gt: ffi('int mgos_arduino_dt_get_temp(void *, char *)'),
  _gtc: ffi('int mgos_arduino_dt_get_tempc(void *, char *)'),
  _gtf: ffi('int mgos_arduino_dt_get_tempf(void *, char *)'),
  _gtcbi: ffi('int mgos_arduino_dt_get_tempc_by_index(void *, int)'),
  _gtfbi: ffi('int mgos_arduino_dt_get_tempf_by_index(void *, int)'),
  _byte2hex: function(byte) {
    let hex_char = '0123456789abcdef';
    return hex_char[(byte >> 4) & 0x0F] + hex_char[byte & 0x0F];
  },

  // Initialize DallasTemperature library. Return value: OneWire handle opaque pointer.
  init: ffi('void *mgos_arduino_dt_init(void *)'),

  // Close DallasTemperature handle. Return value: none.
  close: ffi('void mgos_arduino_dt_close(void *)'),

  // Initialise 1-Wire bus
  begin: ffi('void mgos_arduino_dt_begin(void *)'),

  // Returns the number of devices found on the bus.
  // Returns always 0 if an operaiton failed.
  getDeviceCount: ffi('int mgos_arduino_dt_get_device_count(void *)'),

  // Returns 1 if address is valid.
  // Return always 0 if an operaiton failed.
  validAddress: ffi('int mgos_arduino_dt_valid_address(void *, char *)'),

  // Returns 1 if address is of the family of sensors the lib supports.
  // Return always 0 if an operaiton failed.
  validFamily: ffi('int mgos_arduino_dt_valid_family(void *, char *)'),

  // Finds an address at a given index on the bus.
  // Return 0 if the device was not found or an operaiton failed.
  // Returns 1 otherwise.
  getAddress: ffi('int mgos_arduino_dt_get_address(void *, char *, int)'),

  // Attempt to determine if the device at the given address is connected to the bus.
  // Return 0 if the device is not connected or an operaiton failed.
  // Returns 1 otherwise.
  isConnected: ffi('int mgos_arduino_dt_is_connected(void *, char *)'),

  // Attempt to determine if the device at the given address is connected to the bus.
  // Also allows for updating the read scratchpad.
  // Return false if the device is not connected or an operaiton failed.
  // Returns true otherwise.
  isConnectedUpdateScratchPad: ffi('int mgos_arduino_dt_is_connected_sp(void *, char *)'),

  // Read device's scratchpad.
  // Return 0 if an operaiton failed.
  // Returns 1 otherwise.
  readScratchPad: ffi('int mgos_arduino_dt_read_scratch_pad(void *, char *, char *)'),

  // Write device's scratchpad.
  writeScratchPad: ffi('void mgos_arduino_dt_write_scratch_pad(void *, char *, char *)'),

  // Read device's power requirements.
  // Return 1 if device needs parasite power.
  // Return always 0 if an operaiton failed.
  readPowerSupply: ffi('int mgos_arduino_dt_read_power_supply(void *, char *)'),

  // Get global resolution.
  getGlobalResolution: ffi('int mgos_arduino_dt_get_global_resolution(void *)'),

  // Set global resolution to 9, 10, 11, or 12 bits.
  setGlobalResolution: ffi('void mgos_arduino_dt_set_global_resolution(void *, int)'),

  // Returns the device resolution: 9, 10, 11, or 12 bits.
  // Returns 0 if device not found or if an operaiton failed.
  getResolution: ffi('int mgos_arduino_dt_get_resolution(void *, char *)'),

  // Set resolution of a device to 9, 10, 11, or 12 bits
  // If new resolution is out of range, 9 bits is used.
  // Return 1 if a new value was stored.
  // Returns 0 otherwise.
  setResolution: ffi('int mgos_arduino_dt_set_resolution(void *, char *, int, int)'),

  // Sets/gets the waitForConversion flag.
  setWaitForConversion: ffi('void mgos_arduino_dt_set_wait_for_conversion(void *, int)'),

  // Gets the value of the waitForConversion flag.
  // Return always 0 if an operaiton failed.
  getWaitForConversion: ffi('int mgos_arduino_dt_get_wait_for_conversion(void *)'),

  // Sets the checkForConversion flag.
  setCheckForConversion: ffi('void mgos_arduino_dt_set_check_for_conversion(void *, int)'),

  // Gets the value of the waitForConversion flag.
  // Return always 0 if an operaiton failed.
  getCheckForConversion: ffi('int mgos_arduino_dt_get_check_for_conversion(void *)'),

  // Sends command for all devices on the bus to perform a temperature conversion.
  // Returns 0 if a device is disconnected or if an operaiton failed.
  // Returns 1 otherwise.
  requestTemperatures: ffi('void mgos_arduino_dt_request_temperatures(void *)'),

  // Sends command for one device to perform a temperature conversion by address.
  // Returns 0 if a device is disconnected or if an operaiton failed.
  // Returns 1  otherwise.
  requestTemperaturesByAddress: ffi('int mgos_arduino_dt_request_temperatures_by_address(void *, char *)'),

  // Sends command for one device to perform a temperature conversion by index.
  // Returns 0 if a device is disconnected or if an operaiton failed.
  // Returns 1 otherwise.
  requestTemperaturesByIndex: ffi('int mgos_arduino_dt_request_temperatures_by_index(void *, int)'),

  // Returns temperature raw value (12 bit integer of 1/128 degrees C)
  // or DEVICE_DISCONNECTED_RAW if an operaiton failed.
  getTemp: function(dt, addr) {
    // C-functions output value of “1234” equals 12.34 Deg.
    return this._gt(dt, addr) / 100.0;
  },

  // Returns temperature in degrees C
  // or DEVICE_DISCONNECTED_C if an operaiton failed.
  getTempC: function(dt, addr) {
    // C-functions output value of “1234” equals 12.34 Deg.
    return this._gtc(dt, addr) / 100.0;
  },

  // Returns temperature in degrees F
  // or DEVICE_DISCONNECTED_F if an operaiton failed.
  getTempF: function(dt, addr) {
    // C-functions output value of “1234” equals 12.34 Deg.
    return this._gtf(dt, addr) / 100.0;
  },

  // Get temperature for device index in degrees C (slow)
  // or DEVICE_DISCONNECTED_C if an operaiton failed.
  getTempCByIndex: function(dt, idx) {
    // C-functions output value of “1234” equals 12.34 Deg.
    return this._gtcbi(dt, idx) / 100.0;
  },

  // Get temperature for device index in degrees F (slow)
  // or DEVICE_DISCONNECTED_F if an operaiton failed.
  getTempFByIndex: function(dt, idx) {
    // C-functions output value of “1234” equals 12.34 Deg.
    return this._gtfbi(dt, idx) / 100.0;
  },

  // Returns 1 if the bus requires parasite power.
  // Returns always 0 if an operaiton failed.
  isParasitePowerMode: ffi('int mgos_arduino_dt_is_parasite_power_mode(void *)'),

  // Is a conversion complete on the wire?
  // Return always 0 if an operaiton failed.
  isConversionComplete: ffi('int mgos_arduino_dt_is_conversion_complete(void *)'),

  // Returns number of milliseconds to wait till conversion is complete (based on IC datasheet)
  // or 0 if an operaiton failed.
  millisToWaitForConversion: ffi('int mgos_arduino_dt_millis_to_wait_for_conversion(void *, int)'),

  // Sets the high alarm temperature for a device.
  // Accepts a char. Valid range is -55C - 125C.
  setHighAlarmTemp: ffi('void mgos_arduino_dt_set_high_alarm_temp(char *, int)'),

  // Sets the low alarm temperature for a device.
  // Accepts a char. Valid range is -55C - 125C.
  setLowAlarmTemp: ffi('void mgos_arduino_dt_set_low_alarm_temp(char *, int)'),

  // Returns a signed char with the current high alarm temperature for a device
  // in the range -55C - 125C or DEVICE_DISCONNECTED_C if an operaiton failed.
  getHighAlarmTemp: ffi('int mgos_arduino_dt_get_high_alarm_temp(char *)'),

  // Returns a signed char with the current low alarm temperature for a device
  // in the range -55C - 125C or DEVICE_DISCONNECTED_C if an operaiton failed.
  getLowAlarmTemp: ffi('int mgos_arduino_dt_get_low_alarm_temp(char *)'),

  // Resets internal variables used for the alarm search.
  resetAlarmSearch: ffi('void mgos_arduino_dt_reset_alarm_search(void)'),

  // Search the wire for devices with active alarms.
  // Returns true then it has enumerated the next device.
  // Returns false if there are no devices or an operaiton failed.
  // If a new device is found then its address is copied to param #2.
  // Use resetAlarmSearch to start over.
  alarmSearch: ffi('int mgos_arduino_dt_alarm_search(char *)'),

  // Returns true if device address might have an alarm condition
  // (only an alarm search can verify this).
  // Return always false if an operaiton failed.
  hasAlarm: ffi('int mgos_arduino_dt_has_alarm(char *)'),

  // Returns true if any device is reporting an alarm on the bus
  // Return always false if an operaiton failed.
  hasAlarms: ffi('int mgos_arduino_dt_has_alarms(void)'),

  // Get device address in hex format
  toHexStr: function(addr) {
    let res = '';
    for (let i = 0; i < addr.length; i++) {
      res += this._byte2hex(addr.charCodeAt(i));
    }
    return res;
  },
};
