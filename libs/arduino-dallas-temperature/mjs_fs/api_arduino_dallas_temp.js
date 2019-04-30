// Arduino DallasTemperature library API. Source C API is defined at:
// [mgos_arduino_dallas_temp.h](https://github.com/cesanta/mongoose-os/libs/arduino-dallas-temperature/blob/master/src/mgos_arduino_dallas_temp.h)

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
  _shat: ffi('void mgos_arduino_dt_set_high_alarm_temp(void *, int)'),
  _slat: ffi('void mgos_arduino_dt_set_low_alarm_temp(void *, int)'),
  _ghat: ffi('int mgos_arduino_dt_get_high_alarm_temp(void *)'),
  _glat: ffi('int mgos_arduino_dt_get_low_alarm_temp(void *)'),
  _ras: ffi('void mgos_arduino_dt_reset_alarm_search(void *)'),
  _as: ffi('int mgos_arduino_dt_alarm_search(void *, void *)'),
  _ha: ffi('int mgos_arduino_dt_has_alarm(void *, void *)'),
  _has: ffi('int mgos_arduino_dt_has_alarms(void *)'),

  _byte2hex: function(byte) {
    let hex_char = '0123456789abcdef';
    return hex_char[(byte >> 4) & 0x0F] + hex_char[byte & 0x0F];
  },

  // ## **`DallasTemperature.create(ow)`**
  // Create and return an instance of the dallas temperature: an object with
  // methods described below. `ow` is an OneWire instance.
  //
  // Example:
  // ```javascript
  // let ow = OneWire(12 /* onewire pin number */);
  // let myDT = DallasTemperature.create(ow);
  // ```
  create: function(ow) {
    let obj = Object.create(DallasTemperature._proto);
    // Initialize DallasTemperature library.
    // Return value: handle opaque pointer.
    obj.dt = DallasTemperature._create(ow.ow);
    return obj;
  },

  _proto: {
    // ## **`myDT.close()`**
    // Close DallasTemperature handle. Return value: none.
    close: function() {
      return DallasTemperature._close(this.dt);
    },

    // ## **`myDT.begin()`**
    // Initialise the sensor. Return value: none.
    begin: function() {
      return DallasTemperature._begin(this.dt);
    },

    // ## **`myDT.getDeviceCount()`**
    // Return the number of devices found on the bus.
    // If an operaiton is failed, 0 is returned.
    getDeviceCount: function() {
      return DallasTemperature._gdc(this.dt);
    },

    // ## **`myDT.validAddress(addr)`**
    // Check if given onewire `addr` (8-byte string) is valid; returns 1 if it
    // is, or 0 otherwise.
    validAddress: function(addr) {
      return DallasTemperature._va(this.dt, addr);
    },

    // ## **`myDT.validFamily(addr)`**
    // Return 1 if onewire address `addr` (8-byte string) is of the family of
    // sensors the lib supports.  Return always 0 if an operaiton failed.
    validFamily: function(addr) {
      return DallasTemperature._vf(this.dt, addr);
    },

    // ## **`myDT.getAddress(addr, idx)`**
    // Find an onewire address at a given index `idx` on the bus. Resulting
    // address is written into the provided string buffer `addr`, which should
    // be 8 bytes lont.
    // Return value: 1 in case of success, 0 otherwise.
    // Example:
    // ```javascript
    // load("api_sys.js");
    // load("api_arduino_dallas_temp.js");
    //
    // let addr = Sys._sbuf(8);
    // let res = myDT.getAddress(addr, 0);
    // if (res === 1) {
    //   print("found:", addr);
    // } else {
    //   print("not found");
    // }
    // ```
    getAddress: function(addr, idx) {
      return DallasTemperature._ga(this.dt, addr, idx);
    },

    // ## **`myDT.isConnected(addr)`**
    // Determine if the device at the given onewire address (8-byte string) is
    // connected to the bus.
    // Return value: 1 if device is connected, 0 otherwise.
    isConnected: function(addr) {
      return DallasTemperature._isc(this.dt, addr);
    },

    // ## **`myDT.isConnectedWithScratchPad(addr, sp)`**
    // Determine if the device at the given onewire address (8-byte string) is
    // connected to the bus, and if so, read the scratch pad to the provided
    // buffer (9-byte string).
    // Return value: 1 if device is connected (and a scratchpad is read), 0
    // otherwise.
    // Example:
    // ```javascript
    // load("api_sys.js");
    // load("api_arduino_dallas_temp.js");
    //
    // let sp = Sys._sbuf(9);
    // let res = myDT.isConnectedWithScratchPad("\x28\xff\x2b\x45\x4c\x04\x00\x10", sp);
    // if (res === 1) {
    //   print("connected, scratchpad:", sp);
    // } else {
    //   print("not connected");
    // }
    // ```
    isConnectedWithScratchPad: function(addr, sp) {
      return DallasTemperature._iscsp(this.dt, addr, sp);
    },

    // ## **`myDT.readScratchPad(addr, sp)`**
    // Read device's scratchpad.
    // `sp` is a string buffer (minimum 9 bytes length) to read scratchpad
    // into.
    // Return 1 in case of success, 0 otherwise.
    // Example:
    // ```javascript
    // load("api_sys.js");
    // load("api_arduino_dallas_temp.js");
    //
    // let sp = Sys._sbuf(9);
    // let res = myDT.readScratchPad("\x28\xff\x2b\x45\x4c\x04\x00\x10", sp);
    // if (res === 1) {
    //   print("scratchpad:", sp);
    // } else {
    //   print("failed to read scratchpad");
    // }
    // ```
    readScratchPad: function(addr, sp) {
      return DallasTemperature._rsp(this.dt, addr, sp);
    },

    // ## **`myDT.writeScratchPad(addr, sp)`**
    // Write device's scratchpad `sp` (which should be a 9-byte string) by
    // the provided onewire address `addr` (a 8-byte string).
    // Return value: none.
    writeScratchPad: function(addr, sp) {
      return DallasTemperature._wsp(this.dt, addr, sp);
    },

    // ## **`myDT.readPowerSupply()`**
    // Read device's power requirements.
    // Return 1 if device needs parasite power.
    // Return always 0 if an operaiton failed.
    readPowerSupply: function(addr) {
      return DallasTemperature._rps(this.dt, addr);
    },

    // ## **`myDT.getGlobalResolution()`**
    // Get global resolution in bits. Return value: 9, 10, 11 or 12.
    // In case of a failure, returns 0.
    getGlobalResolution: function() {
      return DallasTemperature._ggr(this.dt);
    },

    // ## **`myDT.setGlobalResolution(res)`**
    // Set global resolution `res` in bits, which can be either 9, 10, 11, or
    // 12. If given resolution is out of range, 9 bits is used.
    // Return value: none.
    setGlobalResolution: function(res) {
      return DallasTemperature._sgr(this.dt, res);
    },
    
    // ## **`myDT.getResolution(addr)`**
    // Get device's resolution in bits. Return value: 9, 10, 11 or 12.
    // In case of a failure, returns 0.
    getResolution: function(addr) {
      return DallasTemperature._gr(this.dt, addr);
    },

    // ## **`myDT.setResolution(addr, res, skip)`**
    // Set resolution of a device with onewire address `addr` to 9, 10, 11, or
    // 12 bits.  If given resolution is out of range, 9 bits is used.
    // Return 1 in case of success, 0 otherwise.
    setResolution: function(addr, res, skip) {
      return DallasTemperature._sr(this.dt, addr, res, skip);
    },

    // ## **`myDT.setWaitForConversion(waitForConversion)`**
    // Set/clear the waitForConversion flag.
    // Return value: none.
    setWaitForConversion: function(f) {
      return DallasTemperature._swfc(this.dt, f);
    },

    // ## **`myDT.getWaitForConversion()`**
    // Get the value of the waitForConversion flag: either 1 or 0. In case
    // of a failure, return 0.
    getWaitForConversion: function() {
      return DallasTemperature._gwfc(this.dt);
    },

    // ## **`myDT.setCheckForConversion(checkForConversion)`**
    // Set/clear the `checkForConversion` flag.
    setCheckForConversion: function(f) {
      return DallasTemperature._scfc(this.dt, f);
    },

    // ## **`myDT.getCheckForConversion()`**
    // Get the value of the `checkForConversion` flag: either 1 or 0. In case
    // of a failure, return 0.
    getCheckForConversion: function() {
      return DallasTemperature._gcfc(this.dt);
    },

    // ## **`myDT.requestTemperatures()`**
    // Send command for all devices on the bus to perform a temperature
    // conversion.
    //
    // Return value: 1 in case of success, 0 otherwise.
    requestTemperatures: function() {
      return DallasTemperature._rts(this.dt);
    },

    // ## **`myDT.requestTemperaturesByAddress(addr)`**
    // Send command to a device with the given onewire address `addr` to
    // perform a temperature conversion.
    //
    // Return value: 1 in case of success, 0 otherwise.
    requestTemperaturesByAddress: function(addr) {
      return DallasTemperature._rtsba(this.dt, addr);
    },

    // ## **`myDT.requestTemperaturesByIndex(idx)`**
    // Send command to a device with the given index `idx` to perform a
    // temperature conversion.
    //
    // Return value: 1 in case of success, 0 otherwise.
    requestTemperaturesByIndex: function(idx) {
      return DallasTemperature._rtsbi(this.dt, idx);
    },

    // ## **`myDT.getTemp(addr)`**
    // Return raw temperature value (12 bit integer of 1/128 degrees C)
    // or `DallasTemperature.DEVICE_DISCONNECTED_RAW` in case of a failure.
    getTemp: function(addr) {
      // C-functions output value of “1234” equals 12.34 Deg.
      return DallasTemperature._gt(this.dt, addr) / 100.0;
    },

    // ## **`myDT.getTempC(addr)`**
    // Returns temperature in degrees C or
    // `DallasTemperature.DEVICE_DISCONNECTED_C` in case of a failure.
    getTempC: function(addr) {
      // C-functions output value of “1234” equals 12.34 Deg.
      return DallasTemperature._gtc(this.dt, addr) / 100.0;
    },

    // ## **`myDT.getTempF(addr)`**
    // Returns temperature in degrees F or
    // `DallasTemperature.DEVICE_DISCONNECTED_F` in case of a failure.
    getTempF: function(addr) {
      // C-functions output value of “1234” equals 12.34 Deg.
      return DallasTemperature._gtf(this.dt, addr) / 100.0;
    },

    // ## **`myDT.getTempCByIndex(idx)`**
    // Get temperature from the device with the given index `idx` in degrees C,
    // or `DallasTemperature.DEVICE_DISCONNECTED_C` in case of a failure.
    getTempCByIndex: function(idx) {
      // C-functions output value of “1234” equals 12.34 Deg.
      return DallasTemperature._gtcbi(this.dt, idx) / 100.0;
    },

    // ## **`myDT.getTempFByIndex(idx)`**
    // Get temperature from the device with the given index `idx` in degrees F,
    // or `DallasTemperature.DEVICE_DISCONNECTED_F` in case of a failure.
    getTempFByIndex: function(idx) {
      // C-functions output value of “1234” equals 12.34 Deg.
      return DallasTemperature._gtfbi(this.dt, idx) / 100.0;
    },

    // ## **`myDT.isParasitePowerMode()`**
    // Return 1 if the bus requires parasite power, 0 otherwise. In case of a
    // failure return 0.
    isParasitePowerMode: function() {
      return DallasTemperature._isppm(this.dt);
    },
    
    // ## **`myDT.isConversionComplete()`**
    // Return whether a conversion is completed.
    isConversionComplete: function() {
      return DallasTemperature._iscc(this.dt);
    },

    // ## **`myDT.millisToWaitForConversion(res)`**
    // Return number of milliseconds to wait until the conversion is completed
    // for the given resolution `res` in bits (9, 10, 11 or 12).
    // In case of a failure, return 0.
    millisToWaitForConversion: function(res) {
      return DallasTemperature._mtwfc(this.dt, res);
    },

    // ## **`myDT.setHighAlarmTemp(grc)`**
    // Set the upper alarm temperature (in degrees C) for a device; valid range
    // for `grc` is from -55 to 125.
    // Return value: none.
    setHighAlarmTemp: function(grc) {
      return DallasTemperature._shat(this.dt, grc);
    },

    // ## **`myDT.setLowAlarmTemp()`**
    // Set the lower alarm temperature (in degrees C) for a device; valid range
    // for `grc` is from -55 to 125.
    // Return value: none.
    setLowAlarmTemp: function(grc) {
      return DallasTemperature._slat(this.dt, grc);
    },

    // ## **`myDT.getHighAlarmTemp()`**
    // Return upper alarm temperature in degrees C (from -55 to 125), or
    // `DallasTemperature.DEVICE_DISCONNECTED_C` in case of a failure.
    getHighAlarmTemp: function() {
      return DallasTemperature._ghat(this.dt);
    },

    // ## **`myDT.getHighAlarmTemp()`**
    // Return lower alarm temperature in degrees C (from -55 to 125), or
    // `DallasTemperature.DEVICE_DISCONNECTED_C` in case of a failure.
    getLowAlarmTemp: function() {
      return DallasTemperature._glat(this.dt);
    },

    // ## **`myDT.alarmSearch(addr)`**
    // Search the wire for devices with active alarms.
    //
    // `addr` should be a string buffer of at least 8 bytes.
    //
    // If the next device is found, 1 is returned and the device's address
    // is written to `addr`; otherwise 0 is returned.
    //
    // Use `myDT.resetAlarmSearch()` to start over.
    // Example:
    // ```javascript
    // load("api_sys.js");
    // load("api_arduino_dallas_temp.js");
    //
    // print("Looking for devices with active alarms...");
    // let addr = Sys._sbuf(8);
    // while (myDT.alarmSearch(addr) === 1) {
    //   print("Found:", addr);
    // }
    // print("Done.");
    // ```
    alarmSearch: function(addr) {
      return DallasTemperature._as(this.dt, addr);
    },

    // ## **`myDT.resetAlarmSearch()`**
    // Reset alarm search.
    // Return value: none.
    resetAlarmSearch: function() {
      return DallasTemperature._ras(this.dt);
    },

    // ## **`myDT.hasAlarm(addr)`**
    // Return 1 if device with the given onewire address has active alarm;
    // 0 otherwise. In case of a failure, 0 is returned.
    hasAlarm: function(addr) {
      return DallasTemperature._ha(this.dt, addr);
    },

    // ## **`myDT.hasAlarms()`**
    // Return 1 if any device on the bus has active alarm; 0 otherwise.
    // In case of a failure, 0 is returned.
    hasAlarms: function() {
      return DallasTemperature._has(this.dt);
    },

    // ## **`myDT.toHexStr(addr)`**
    // Return device address `addr` in the hex format.
    toHexStr: function(addr) {
      let res = '';
      for (let i = 0; i < addr.length; i++) {
        res += DallasTemperature._byte2hex(addr.charCodeAt(i));
      }
      return res;
    },
  },
};
