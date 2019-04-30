/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

let TouchPad = {
  // ## **`TouchPad.GPIO`**
  // Handy map of GPIO to touch sensor number. Touch sensor number is a number
  // from 0 to 9.
  GPIO: {"4": 0, "0": 1, "2": 2, "27": 7, "15": 3, "13": 4, "12": 5, "14": 6, "33": 8, "32": 9},

  // ## **`TouchPad.init()`**
  // Initialize touch pad module.
  // Return value: 0 in case of success, non-zero otherwise.
  init: ffi('int touch_pad_init()'),

  // ## **`TouchPad.deinit()`**
  // Uninstall touch pad driver.
  // Return value: 0 in case of success, non-zero otherwise.
  deinit: ffi('int touch_pad_deinit()'),

  // ## **`TouchPad.config(touch_num, threshold)`**
  // Configure touch pad interrupt threshold.
  // `touch_num` is a touchpad index (a number from 0 to 9), `threshold` is an interrupt threshold
  // Return value: 0 in case of success, non-zero otherwise.
  config: ffi('int touch_pad_config(int, int)'),

  // ## **`TouchPad.read(touch_num)`**
  // Return touch sensor counter value or -1 in case of a failure. `touch_num`
  // is a touchpad index (a number from 0 to 9)
  //
  // Each touch sensor has a counter to count the number of charge/discharge
  // cycles.  When the pad is not 'touched', we can get a number of the
  // counter.  When the pad is 'touched', the value in counter will get smaller
  // because of the larger equivalent capacitance.  User can use this function
  // to determine the interrupt trigger threshold.
  read: ffi('int esp32_touch_pad_read(int)'),

  // ## **`TouchPad.readFiltered(touch_num)`**
  // Get filtered touch sensor counter value by IIR filter, or -1 in case of a
  // failure. NOTE: `TouchPad.filterStart()` has to be called before this
  // function.  `touch_num` is a touchpad index (a number from 0 to 9).
  readFiltered: ffi('int esp32_touch_pad_read_filtered(int)'),

  // ## **`TouchPad.isrRegister(handler, userdata)`**
  // Register touchpad ISR. The handler will be attached to the same CPU core
  // that this function is running on. Handler is a function like
  // `function(status, userdata){ /* ... */ }`, `status` is a number
  // representing which pads are "touched".
  // Return value: 0 in case of success, non-zero otherwise.
  isrRegister: ffi('int esp32_touch_pad_isr_register(void (*)(int, userdata), userdata)'),

  // ## **`TouchPad.isrDeregister()`**
  // Deregister touchpad ISR previously registered with
  // `TouchPad.isrRegister()`.  Return value: 0 in case of success, non-zero
  // otherwise.
  isrDeregister: ffi('int esp32_touch_pad_isr_register()'),

  // ## **`TouchPad.intrEnable()`**
  // Enable touchpad interrupt. Return value: 0.
  intrEnable: ffi('int touch_pad_intr_enable()'),

  // ## **`TouchPad.intrDisable()`**
  // Disable touchpad interrupt. Return value: 0.
  intrDisable: ffi('int touch_pad_intr_disable()'),

  // ## **`TouchPad.setMeasTime(sleep_cycle, meas_cycle)`**
  // Set touch sensor measurement and sleep time.
  // The touch sensor will sleep after each measurement.  `sleep_cycle` determines
  // the interval between each measurement:  `t_sleep = sleep_cycle / (RTC_SLOW_CLK frequency)`.
  // `meas_cycle` is the duration of the touch sensor measurement.
  // `t_meas = meas_cycle / 8M`, the maximum measure time is `0xffff / 8M = 8.19 ms`
  // Return value: 0 in case of success, non-zero otherwise.
  setMeasTime: ffi('int touch_pad_set_meas_time(int, int)'),

  // ## **`TouchPad.getMeasTimeSleepCycle()`**
  // Return sleep cycle (previously set with `TouchPad.setMeasTime()`), or -1 in
  // case of a failure.
  getMeasTimeSleepCycle: ffi('int esp32_touch_pad_get_meas_time_sleep_cycle()'),

  // ## **`TouchPad.getMeasTimeMeasCycle()`**
  // Return duration of the touch sensor measurement (previously set with
  // `TouchPad.setMeasTime()`), or -1 in case of a failure.
  getMeasTimeMeasCycle: ffi('int esp32_touch_pad_get_meas_time_meas_cycle()'),

  HVOLT_KEEP:  -1, HVOLT_2V4: 0, HVOLT_2V5: 1, HVOLT_2V6: 2, HVOLT_2V7: 3,
  LVOLT_KEEP:  -1, LVOLT_0V5: 0, LVOLT_0V6: 1, LVOLT_0V7: 2, LVOLT_0V8: 3,
  HVOLT_ATTEN_KEEP: -1, HVOLT_ATTEN_1V5: 0, HVOLT_ATTEN_1V: 1, HVOLT_ATTEN_0V5: 2, HVOLT_ATTEN_0V: 3,

  // ## **`TouchPad.setVoltage(refh, refl, atten)`**
  // Set touch sensor reference voltage, if the voltage gap between high and
  // low reference voltage get less, the charging and discharging time would be
  // faster; accordingly, the counter value would be larger.  In the case of
  // detecting very slight change of capacitance, we can narrow down the gap so
  // as to increase the sensitivity. On the other hand, narrow voltage gap
  // would also introduce more noise, but we can use a software filter to
  // pre-process the counter value.
  //
  // `refh` is the value of `DREFH`, one of the following:
  // - `TouchPad.HVOLT_KEEP`
  // - `TouchPad.HVOLT_2V4`
  // - `TouchPad.HVOLT_2V5`
  // - `TouchPad.HVOLT_2V6`
  // - `TouchPad.HVOLT_2V7`
  //
  // `refl` is the value os `DREFL`, one of the following:
  // - `TouchPad.LVOLT_KEEP`
  // - `TouchPad.LVOLT_0V5`
  // - `TouchPad.LVOLT_0V6`
  // - `TouchPad.LVOLT_0V7`
  // - `TouchPad.LVOLT_0V8`
  //
  // `atten` is the attenuation of `DREFH`, one of the following:
  // - `TouchPad.HVOLT_ATTEN_KEEP`
  // - `TouchPad.HVOLT_ATTEN_1V5`
  // - `TouchPad.HVOLT_ATTEN_1V`
  // - `TouchPad.HVOLT_ATTEN_0V5`
  // - `TouchPad.HVOLT_ATTEN_0V`
  //
  // Return value: 0 in case of success, non-zero otherwise.
  setVoltage: ffi('int touch_pad_set_voltage(int, int, int)'),

  // ## **`TouchPad.getVoltageRefH()`**
  // Get touch sensor reference voltage `refh` (previously set with
  // `TouchPad.setVoltage()`), or -1 in case of a failure.
  getVoltageRefH: ffi('int esp32_touch_pad_get_voltage_refh()'),

  // ## **`TouchPad.getVoltageRefL()`**
  // Get touch sensor reference voltage `refl` (previously set with
  // `TouchPad.setVoltage()`), or -1 in case of a failure.
  getVoltageRefL: ffi('int esp32_touch_pad_get_voltage_refl()'),

  // ## **`TouchPad.getVoltageAtten()`**
  // Get touch sensor attenuation of `DREFH` (previously set with
  // `TouchPad.setVoltage()`), or -1 in case of a failure.
  getVoltageAtten: ffi('int esp32_touch_pad_get_voltage_atten()'),

  PAD_TIE_OPT_LOW: 0, PAD_TIE_OPT_HIGH: 1,

  // ## **`TouchPad.setCntMode(touch_num, slope, opt)`**
  // Set touch sensor charge/discharge speed for each pad.
  //
  // `touch_num` is a touchpad index (a number from 0 to 9), `slope` is a
  // charge/discharge speed, `opt` is the initial voltage, one of the following:
  // - `TouchPad.PAD_TIE_OPT_LOW`
  // - `TouchPad.PAD_TIE_OPT_HIGH`
  //
  // If `slope` is 0, the counter would always be zero.
  // If `slope` is 1, the charging and discharging would be slow, accordingly,
  // the counter value would be small.
  // If `slope` is set 7, which is the maximum value, the charging and
  // discharging would be fast, accordingly, the counter value would be larger.
  // Return value: 0 in case of success, non-zero otherwise.
  setCntMode: ffi('int touch_pad_set_cnt_mode(int, int, int)'),

  // ## **`TouchPad.getCntModeSlope(touch_num)`**
  // Get "slope": a charge/discharge speed previously set with
  // `TouchPad.setCntMode()`, or -1 in case of a failure.
  getCntModeSlope: ffi('int esp32_touch_pad_get_cnt_mode_slope(int)'),

  // ## **`TouchPad.getCntModeOpt(touch_num)`**
  // Get initial voltage previously set with `TouchPad.setCntMode()`, or -1 in
  // case of a failure.
  // `touch_num` is a touchpad index (a number from 0 to 9).
  getCntModeOpt: ffi('int esp32_touch_pad_get_cnt_mode_opt(int)'),

  // ## **`TouchPad.ioInit(touch_num)`**
  // Initialize touch pad GPIO.
  // `touch_num` is a touchpad index (a number from 0 to 9).
  // Return value: 0 in case of success, non-zero otherwise.
  ioInit: ffi('int touch_pad_io_init(int)'),

  FSM_MODE_TIMER: 0, FSM_MODE_SW: 1,

  // ## **`TouchPad.setFSMMode(mode)`**
  // Set touch sensor FSM mode, the test action can be triggered by the timer,
  // as well as by the software. `mode` can be one of the following:
  // - `TouchPad.FSM_MODE_TIMER`
  // - `TouchPad.FSM_MODE_SW`
  // Return value: 0 in case of success, non-zero otherwise.
  setFSMMode: ffi('int touch_pad_set_fsm_mode(int)'),

  // ## **`TouchPad.getFSMMode()`**
  // Get FSM mode previously set with `TouchPad.setFSMMode()`, or -1 in case of
  // a failure.
  getFSMMode: ffi('int esp32_touch_pad_get_fsm_mode()'),

  // ## **`TouchPad.swStart()`**
  // Trigger a touch sensor measurement, only support in `FSM_MODE_SW` mode of
  // FSM.
  // Return value: 0 in case of success, non-zero otherwise.
  swStart: ffi('int touch_pad_sw_start()'),

  // ## **`TouchPad.setThresh(touch_num, threshold)`**
  // Set touch sensor interrupt threshold.
  // `touch_num` is a touchpad index (a number from 0 to 9), `threshold` is a
  // threshold of touchpad count; refer to `TouchPad.setTriggerMode()` to see
  // how to set trigger mode.
  // Return value: 0 in case of success, non-zero otherwise.
  setThresh: ffi('int touch_pad_set_thresh(int, int)'),

  // ## **`TouchPad.getThresh(touch_num)`**
  // Get touch sensor interrupt threshold previously set with
  // `TouchPad.setThresh()`, or -1 in case of a failure.
  // `touch_num` is a touchpad index (a number from 0 to 9).
  // Return value: 0 in case of success, non-zero otherwise.
  getThresh: ffi('int esp32_touch_pad_get_thresh(int)'),

  TRIGGER_BELOW: 0, TTRIGGER_ABOVE: 1,

  // ## **`TouchPad.setTriggerMode(mode)`**
  // Set touch sensor interrupt trigger mode, one of the following:
  // - `TouchPad.TRIGGER_BELOW`
  // - `TouchPad.TRIGGER_ABOVE`
  //
  // Interrupt can be triggered either when counter result is less than
  // threshold, or when counter result is more than threshold.
  //
  // Return value: 0 in case of success, non-zero otherwise.
  setTriggerMode: ffi('int touch_pad_set_trigger_mode(int)'),

  // ## **`TouchPad.getTriggerMode()`**
  // Get touch sensor interrupt trigger mode previously set with
  // `TouchPad.setTriggerMode()`, or -1 in case of a failure.
  // Return value: 0 in case of success, non-zero otherwise.
  getTriggerMode: ffi('int esp32_touch_pad_get_trigger_mode()'),

  TRIGGER_SOURCE_BOTH: 0, TRIGGER_SOURCE_SET1: 1,

  // ## **`TouchPad.setTriggerSource(src)`**
  // Set touch sensor interrupt trigger source `src`, one of the following:
  // - `TouchPad.TRIGGER_SOURCE_BOTH`
  // - `TouchPad.TRIGGER_SOURCE_SET1`
  //
  // There are two sets of touch signals.  Set1 and set2 can be mapped to
  // several touch signals. Either set will be triggered if at least one of its
  // touch signal is 'touched'. The interrupt can be configured to be generated
  // if set1 is triggered, or only if both sets are triggered.
  //
  // Return value: 0 in case of success, non-zero otherwise.
  setTriggerSource: ffi('int touch_pad_set_trigger_source(int)'),

  // ## **`TouchPad.getTriggerSource()`**
  // Get touch sensor interrupt trigger source previously set with
  // `TouchPad.setTriggerSource()`, or -1 in case of a failure.
  // Return value: 0 in case of success, non-zero otherwise.
  getTriggerSource: ffi('int esp32_touch_pad_get_trigger_source()'),

  // ## **`TouchPad.setGroupMask(set1_mask, set2_mask, en_mask)`**
  // Set touch sensor group mask.  Touch pad module has two sets of signals,
  // 'Touched' signal is triggered only if at least one of touch pad in this
  // group is "touched".  This function will set the register bits according to
  // the given bitmask.
  //
  // `set1_mask` is a bitmask of touch sensor signal group1, it's a 10-bit value.
  // `set2_mask` is a bitmask of touch sensor signal group2, it's a 10-bit value.
  // `en_mask` is a bitmask of touch sensor work enable, it's a 10-bit value.
  //
  // Return value: 0 in case of success, non-zero otherwise.
  setGroupMask: ffi('int touch_pad_set_group_mask(int, int, int)'),

  // ## **`TouchPad.getGroupMaskSet1()`**
  // Get set1 mask previously set with `TouchPad.setGroupMask()`, or -1 in case
  // of a failure.
  getGroupMaskSet1: ffi('int esp32_touch_pad_get_group_mask_set1()'),

  // ## **`TouchPad.getGroupMaskSet2()`**
  // Get set2 mask previously set with `TouchPad.setGroupMask()`, or -1 in case
  // of a failure.
  getGroupMaskSet2: ffi('int esp32_touch_pad_get_group_mask_set2()'),

  // ## **`TouchPad.getGroupMaskEn()`**
  // Get mask of enabled sensors previously set with `TouchPad.setGroupMask()`,
  // or -1 in case of a failure.
  getGroupMaskEn: ffi('int esp32_touch_pad_get_group_mask_en()'),

  // ## **`TouchPad.clearStatus()`**
  // Clear touch status register.
  // Return value: 0 in case of success, non-zero otherwise.
  clearStatus: ffi('int touch_pad_clear_status()'),

  // ## **`TouchPad.getStatus()`**
  // Return status: a number representing which pads are "touched".
  getStatus: ffi('int touch_pad_get_status()'),

  // ## **`TouchPad.setFilterPeriod(period_ms)`**
  // Set touch pad filter calibration period, in ms.  Need to call
  // `TouchPad.filterStart()` before all touch filter APIs.
  // Return value: 0 in case of success, non-zero otherwise.
  setFilterPeriod: ffi('int touch_pad_set_filter_period(int)'),

  // ## **`TouchPad.getFilterPeriod(period_ms)`**
  // Get touch pad filter calibration period in ms previously set with
  // `TouchPad.setFilterPeriod()`, or -1 in case of a failure.
  getFilterPeriod: ffi('int esp32_touch_pad_get_filter_period()'),

  // ## **`TouchPad.filterStart(filter_period_ms)`**
  // Start a filter to process the noise in order to prevent false triggering
  // when detecting slight change of capacitance. This function must be called
  // before any other filter API functions.
  //
  // If filter is not initialized, this function will initialize the filter
  // with given period.  If filter is already initialized, it will update the
  // filter period.
  //
  // Return value: 0 in case of success, non-zero otherwise.
  filterStart: ffi('int touch_pad_filter_start(int)'),

  // ## **`TouchPad.filterStop()`**
  // Stop touch pad filter, started before with `TouchPad.filterStart()`.
  // Return value: 0 in case of success, non-zero otherwise.
  filterStop: ffi('int touch_pad_filter_stop()'),

  // ## **`TouchPad.filterDelete()`**
  // Delete touch pad filter driver (activated before with
  // `TouchPad.filterStart()`) and release the memory.
  // Return value: 0 in case of success, non-zero otherwise.
  filterDelete: ffi('int touch_pad_filter_delete()'),
};
