let TMP006 = {
  CONV_4: 0,
  CONV_2: 1,
  CONV_1: 2,
  CONV_1_2: 3,
  CONV_1_4: 4,
  setup: ffi('bool mgos_tmp006_setup(void *, int, int, bool)'),
  getVoltage: ffi('double mgos_tmp006_get_voltage(void *, int)'),
  getDieTemp: ffi('double mgos_tmp006_get_die_temp(void *, int)'),
};
