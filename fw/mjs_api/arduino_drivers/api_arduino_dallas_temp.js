// Arduino DallasTemperature library API. Source C API is defined at:
// [mgos_arduino_dallas_temp.h](https://github.com/cesanta/mongoose-os/blob/master/arduino_drivers/Arduino/mgos_arduino_dallas_temp.h)

let DallasTemperature = {
  // Error codes
  DEVICE_DISCONNECTED_C: -127.0,
  DEVICE_DISCONNECTED_F: -196.6,
  DEVICE_DISCONNECTED_RAW: -7040,

  _create: ffi('void *mgos_arduino_dt_create(void *)'),
  _close: ffi('void mgos_arduino_dt_close(void *)'),
  _begin: ffi('void mgos_arduino_dt_begin(void *)'),
  _gdc: ffi('int mgos_arduino_dt_get_device_count(void *)'),
  _va: ffi('int mgos_arduino_dt_valid_address(void *, char *)'),
  _vf: ffi('int mgos_arduino_dt_valid_family(void *, char *)'),
  _ga: ffi('int mgos_arduino_dt_get_address(void *, char *, int)'),
  _isc: ffi('int mgos_arduino_dt_is_connected(void *, char *)'),
  _iscsp: ffi('int mgos_arduino_dt_is_connected_sp(void *, char *, char *)'),
  _rsp: ffi('int mgos_arduino_dt_read_scratch_pad(void *, char *, char *)'),
  _wsp: ffi('void mgos_arduino_dt_write_scratch_pad(void *, char *, char *)'),
  _rps: ffi('int mgos_arduino_dt_read_power_supply(void *, char *)'),
  _ggr: ffi('int mgos_arduino_dt_get_global_resolution(void *)'),
  _sgr: ffi('void mgos_arduino_dt_set_global_resolution(void *, int)'),
  _gr: ffi('int mgos_arduino_dt_get_resolution(void *, char *)'),
  _sr: ffi('int mgos_arduino_dt_set_resolution(void *, char *, int, int)'),
  _swfc: ffi('void mgos_arduino_dt_set_wait_for_conversion(void *, int)'),
  _gwfc: ffi('int mgos_arduino_dt_get_wait_for_conversion(void *)'),
  _scfc: ffi('void mgos_arduino_dt_set_check_for_conversion(void *, int)'),
  _gcfc: ffi('int mgos_arduino_dt_get_check_for_conversion(void *)'),
  _rts: ffi('void mgos_arduino_dt_request_temperatures(void *)'),
  _rtsba: ffi('int mgos_arduino_dt_request_temperatures_by_address(void *, char *)'),
  _rtsbi: ffi('int mgos_arduino_dt_request_temperatures_by_index(void *, int)'),
  _gt: ffi('int mgos_arduino_dt_get_temp(void *, char *)'),
  _gtc: ffi('int mgos_arduino_dt_get_tempc(void *, char *)'),
  _gtf: ffi('int mgos_arduino_dt_get_tempf(void *, char *)'),
  _gtcbi: ffi('int mgos_arduino_dt_get_tempc_by_index(void *, int)'),
  _gtfbi: ffi('int mgos_arduino_dt_get_tempf_by_index(void *, int)'),
  _isppm: ffi('int mgos_arduino_dt_is_parasite_power_mode(void *)'),
  _iscc: ffi('int mgos_arduino_dt_is_conversion_complete(void *)'),
  _mtwfc: ffi('int mgos_arduino_dt_millis_to_wait_for_conversion(void *, int)'),
  _shat: ffi('void mgos_arduino_dt_set_high_alarm_temp(char *, int)'),
  _slat: ffi('void mgos_arduino_dt_set_low_alarm_temp(char *, int)'),
  _ghat: ffi('int mgos_arduino_dt_get_high_alarm_temp(char *)'),
  _glat: ffi('int mgos_arduino_dt_get_low_alarm_temp(char *)'),
  _ras: ffi('void mgos_arduino_dt_reset_alarm_search(void *)'),
  _as: ffi('int mgos_arduino_dt_alarm_search(void *, char *)'),
  _ha: ffi('int mgos_arduino_dt_has_alarm(void *, char *)'),
  _has: ffi('int mgos_arduino_dt_has_alarms(void *)'),

  _byte2hex: function(byte) {
    let hex_char = '0123456789abcdef';
    return hex_char[(byte >> 4) & 0x0F] + hex_char[byte & 0x0F];
  },

  _proto: {
    // Close DallasTemperature handle. Return value: none.
    close: function() {
      return DallasTemperature._close(this.dt);
    },

    // Initialise 1-Wire bus
    begin: function() {
      return DallasTemperature._begin(this.dt);
    },

    // Returns the number of devices found on the bus.
    // Returns always 0 if an operaiton failed.
    getDeviceCount: function() {
      return DallasTemperature._gdc(this.dt);
    },

    // Returns 1 if address is valid.
    // Return always 0 if an operaiton failed.
    validAddress: function(addr) {
      return DallasTemperature._va(this.dt, addr);
    },

    // Returns 1 if address is of the family of sensors the lib supports.
    // Return always 0 if an operaiton failed.
    validFamily: function(addr) {
      return DallasTemperature._vf(this.dt, addr);
    },

    // Finds an address at a given index on the bus.
    // Return 0 if the device was not found or an operaiton failed.
    // Returns 1 otherwise.
    getAddress: function(addr, idx) {
      return DallasTemperature._ga(this.dt, addr, idx);
    },

    // Attempt to determine if the device at the given address is connected to the bus.
    // Return 0 if the device is not connected or an operaiton failed.
    // Returns 1 otherwise.
    isConnected: function(addr) {
      return DallasTemperature._isc(this.dt, addr);
    },

    // Attempt to determine if the device at the given address is connected to the bus.
    // Also allows for updating the read scratchpad.
    // Return false if the device is not connected or an operaiton failed.
    // Returns true otherwise.
    isConnectedUpdateScratchPad: function(addr, sp) {
      return DallasTemperature._iscsp(this.dt, addr, sp);
    },

    // Read device's scratchpad.
    // Return 0 if an operaiton failed.
    // Returns 1 otherwise.
    readScratchPad: function(addr, sp) {
      return DallasTemperature._rsp(this.dt, addr, sp);
    },

    // Write device's scratchpad.
    writeScratchPad: function(addr, sp) {
      return DallasTemperature._wsp(this.dt, addr, sp);
    },

    // Read device's power requirements.
    // Return 1 if device needs parasite power.
    // Return always 0 if an operaiton failed.
    readPowerSupply: function(addr) {
      return DallasTemperature._rps(this.dt, addr);
    },

    // Get global resolution.
    getGlobalResolution: function() {
      return DallasTemperature._ggr(this.dt);
    },

    // Set global resolution to 9, 10, 11, or 12 bits.
    setGlobalResolution: function(res) {
      return DallasTemperature._sgr(this.dt, res);
    },
    
    // Returns the device resolution: 9, 10, 11, or 12 bits.
    // Returns 0 if device not found or if an operaiton failed.
    getResolution: function(addr) {
      return DallasTemperature._gr(this.dt, addr);
    },

    // Set resolution of a device to 9, 10, 11, or 12 bits
    // If new resolution is out of range, 9 bits is used.
    // Return 1 if a new value was stored.
    // Returns 0 otherwise.
    setResolution: function(addr, res, skip) {
      return DallasTemperature._sr(this.dt, addr, res, skip);
    },

    // Sets/gets the waitForConversion flag.
    setWaitForConversion: function(f) {
      return DallasTemperature._swfc(this.dt, f);
    },

    // Gets the value of the waitForConversion flag.
    // Return always 0 if an operaiton failed.
    getWaitForConversion: function() {
      return DallasTemperature._gwfc(this.dt);
    },

    // Sets the checkForConversion flag.
    setCheckForConversion: function(f) {
      return DallasTemperature._scfc(this.dt, f);
    },

    // Gets the value of the waitForConversion flag.
    // Return always 0 if an operaiton failed.
    getCheckForConversion: function() {
      return DallasTemperature._gcfc(this.dt);
    },

    // Sends command for all devices on the bus to perform a temperature conversion.
    // Returns 0 if a device is disconnected or if an operaiton failed.
    // Returns 1 otherwise.
    requestTemperatures: function() {
      return DallasTemperature._rts(this.dt);
    },

    // Sends command for one device to perform a temperature conversion by address.
    // Returns 0 if a device is disconnected or if an operaiton failed.
    // Returns 1  otherwise.
    requestTemperaturesByAddress: function(addr) {
      return DallasTemperature._rtsba(this.dt, addr);
    },

    // Sends command for one device to perform a temperature conversion by index.
    // Returns 0 if a device is disconnected or if an operaiton failed.
    // Returns 1 otherwise.
    requestTemperaturesByIndex: function(idx) {
      return DallasTemperature._rtsbi(this.dt, idx);
    },

    // Returns temperature raw value (12 bit integer of 1/128 degrees C)
    // or DEVICE_DISCONNECTED_RAW if an operaiton failed.
    getTemp: function(addr) {
      // C-functions output value of “1234” equals 12.34 Deg.
      return DallasTemperature._gt(this.dt, addr) / 100.0;
    },

    // Returns temperature in degrees C
    // or DEVICE_DISCONNECTED_C if an operaiton failed.
    getTempC: function(addr) {
      // C-functions output value of “1234” equals 12.34 Deg.
      return DallasTemperature._gtc(this.dt, addr) / 100.0;
    },

    // Returns temperature in degrees F
    // or DEVICE_DISCONNECTED_F if an operaiton failed.
    getTempF: function(addr) {
      // C-functions output value of “1234” equals 12.34 Deg.
      return DallasTemperature._gtf(this.dt, addr) / 100.0;
    },

    // Get temperature for device index in degrees C (slow)
    // or DEVICE_DISCONNECTED_C if an operaiton failed.
    getTempCByIndex: function(idx) {
      // C-functions output value of “1234” equals 12.34 Deg.
      return DallasTemperature._gtcbi(this.dt, idx) / 100.0;
    },

    // Get temperature for device index in degrees F (slow)
    // or DEVICE_DISCONNECTED_F if an operaiton failed.
    getTempFByIndex: function(idx) {
      // C-functions output value of “1234” equals 12.34 Deg.
      return DallasTemperature._gtfbi(this.dt, idx) / 100.0;
    },

    // Returns 1 if the bus requires parasite power.
    // Returns always 0 if an operaiton failed.
    isParasitePowerMode: function() {
      return DallasTemperature._isppm(this.dt);
    },
    
    // Is a conversion complete on the wire?
    // Return always 0 if an operaiton failed.
    isConversionComplete: function() {
      return DallasTemperature._iscc(this.dt);
    },

    // Returns number of milliseconds to wait till conversion is complete (based on IC datasheet)
    // or 0 if an operaiton failed.
    millisToWaitForConversion: function(res) {
      return DallasTemperature._mtwfc(this.dt, res);
    },

    // Sets the high alarm temperature for a device.
    // Accepts a char. Valid range is -55C - 125C.
    setHighAlarmTemp: function(grc) {
      return DallasTemperature._shat(this.dt, grc);
    },

    // Sets the low alarm temperature for a device.
    // Accepts a char. Valid range is -55C - 125C.
    setLowAlarmTemp: function(grc) {
      return DallasTemperature._slat(this.dt, grc);
    },

    // Returns a signed char with the current high alarm temperature for a device
    // in the range -55C - 125C or DEVICE_DISCONNECTED_C if an operaiton failed.
    getHighAlarmTemp: function() {
      return DallasTemperature._ghat(this.dt);
    },

    // Returns a signed char with the current low alarm temperature for a device
    // in the range -55C - 125C or DEVICE_DISCONNECTED_C if an operaiton failed.
    getLowAlarmTemp: function() {
      return DallasTemperature._glat(this.dt);
    },

    // Resets internal variables used for the alarm search.
    resetAlarmSearch: function() {
      return DallasTemperature._ras(this.dt);
    },

    // Search the wire for devices with active alarms.
    // Returns true then it has enumerated the next device.
    // Returns false if there are no devices or an operaiton failed.
    // If a new device is found then its address is copied to param #2.
    // Use resetAlarmSearch to start over.
    alarmSearch: function(addr) {
      return DallasTemperature._as(this.dt, addr);
    },

    // Returns true if device address might have an alarm condition
    // (only an alarm search can verify this).
    // Return always false if an operaiton failed.
    hasAlarm: function(addr) {
      return DallasTemperature._ha(this.dt, addr);
    },

    // Returns true if any device is reporting an alarm on the bus
    // Return always false if an operaiton failed.
    hasAlarms: function() {
      return DallasTemperature._has(this.dt);
    },

    // Get device address in hex format
    toHexStr: function(addr) {
      let res = '';
      for (let i = 0; i < addr.length; i++) {
        res += DallasTemperature._byte2hex(addr.charCodeAt(i));
      }
      return res;
    },
  },

  create: function(ow) {
    let obj = Object.create(DallasTemperature._proto);
    // Initialize DallasTemperature library.
    // Return value: handle opaque pointer.
    obj.dt = DallasTemperature._create(ow.ow);
    return obj;
  },
};
