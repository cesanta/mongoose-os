/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 * Arduino DallasTemperature library API wrapper
 */

#include "mgos_arduino_dallas_temp.h"

DallasTemperature *mgos_arduino_dt_create(OneWire *ow) {
  if (ow == nullptr) return nullptr;
  return new DallasTemperature(ow);
}

void mgos_arduino_dt_close(DallasTemperature *dt) {
  if (dt != nullptr) {
    delete dt;
    dt = nullptr;
  }
}

void mgos_arduino_dt_begin(DallasTemperature *dt) {
  if (dt == nullptr) return;
  dt->begin();
}

int mgos_arduino_dt_get_device_count(DallasTemperature *dt) {
  if (dt == nullptr) return 0;
  return dt->getDeviceCount();
}

bool mgos_arduino_dt_valid_address(DallasTemperature *dt, const char *addr) {
  if (dt == nullptr) return false;
  return dt->validAddress((uint8_t *)addr);
}

bool mgos_arduino_dt_valid_family(DallasTemperature *dt, const char* addr) {
  if (dt == nullptr) return false;
  return dt->validFamily((uint8_t *)addr);
}

bool mgos_arduino_dt_get_address(DallasTemperature *dt, char *addr, int idx) {
  if (dt == nullptr) return false;
  return dt->getAddress((uint8_t *)addr, idx);
}

bool mgos_arduino_dt_is_connected(DallasTemperature *dt, const char *addr) {
  if (dt == nullptr) return false;
  return dt->isConnected((uint8_t *)addr);
}

bool mgos_arduino_dt_is_connected_sp(DallasTemperature *dt, const char *addr, char *sp) {
  if (dt == nullptr) return false;
  return dt->isConnected((uint8_t *)addr, (uint8_t *)sp);
}

bool mgos_arduino_dt_read_scratch_pad(DallasTemperature *dt, const char *addr, char *sp) {
  if (dt == nullptr) return false;
  return dt->readScratchPad((uint8_t *)addr, (uint8_t *)sp);
}

void mgos_arduino_dt_write_scratch_pad(DallasTemperature *dt, const char *addr, const char *sp) {
  if (dt == nullptr) return;
  dt->writeScratchPad((uint8_t *)addr, (uint8_t *)sp);
}

bool mgos_arduino_dt_read_power_supply(DallasTemperature *dt, const char *addr) {
  if (dt == nullptr) return false;
  return dt->readPowerSupply((uint8_t *)addr);
}

int mgos_arduino_dt_get_global_resolution(DallasTemperature *dt) {
  if (dt == nullptr) return 0;
  return dt->getResolution();
}

void mgos_arduino_dt_set_global_resolution(DallasTemperature *dt, int res) {
  if (dt == nullptr) return;
  dt->setResolution(res);
}

int mgos_arduino_dt_get_resolution(DallasTemperature *dt, const char *addr) {
  if (dt == nullptr) return 0;
  return dt->getResolution((uint8_t *)addr);
}

bool mgos_arduino_dt_set_resolution(DallasTemperature *dt, const char *addr, int res, bool skip_global_calc) {
  if (dt == nullptr) return false;
  return dt->setResolution((uint8_t *)addr, res, skip_global_calc);
}

void mgos_arduino_dt_set_wait_for_conversion(DallasTemperature *dt, bool f) {
  if (dt == nullptr) return;
  dt->setWaitForConversion(f);
}

bool mgos_arduino_dt_get_wait_for_conversion(DallasTemperature *dt) {
  if (dt == nullptr) return false;
  return dt->getWaitForConversion();
}

void mgos_arduino_dt_set_check_for_conversion(DallasTemperature *dt, bool f) {
  if (dt == nullptr) return;
  dt->setCheckForConversion(f);
}

bool mgos_arduino_dt_get_check_for_conversion(DallasTemperature *dt) {
  if (dt == nullptr) return false;
  return dt->getCheckForConversion();
}

void mgos_arduino_dt_request_temperatures(DallasTemperature *dt) {
  if (dt == nullptr) return;
  dt->requestTemperatures();
}

bool mgos_arduino_dt_request_temperatures_by_address(DallasTemperature *dt, const char *addr) {
  if (dt == nullptr) return false;
  return dt->requestTemperaturesByAddress((uint8_t *)addr);
}

bool mgos_arduino_dt_request_temperatures_by_index(DallasTemperature *dt, int idx) {
  if (dt == nullptr) return false;
  return dt->requestTemperaturesByIndex(idx);
}

int16_t mgos_arduino_dt_get_temp(DallasTemperature *dt, const char *addr) {
  if (dt == nullptr) return DEVICE_DISCONNECTED_RAW;
  return dt->getTemp((uint8_t *)addr);
}

int mgos_arduino_dt_get_tempc(DallasTemperature *dt, const char *addr) {
  if (dt == nullptr) return DEVICE_DISCONNECTED_C;
  return round(dt->getTempC((uint8_t *)addr) * 100.0);
}

int mgos_arduino_dt_get_tempf(DallasTemperature *dt, const char *addr) {
  if (dt == nullptr) return DEVICE_DISCONNECTED_F;
  return round(dt->getTempF((uint8_t *)addr) * 100.0);
}

int mgos_arduino_dt_get_tempc_by_index(DallasTemperature *dt, int idx) {
  if (dt == nullptr) return DEVICE_DISCONNECTED_C;
  return round(dt->getTempCByIndex(idx) * 100.0);
}

int mgos_arduino_dt_get_tempf_by_index(DallasTemperature *dt, int idx) {
  if (dt == nullptr) return DEVICE_DISCONNECTED_F;
  return round(dt->getTempFByIndex(idx) * 100.0);
}

bool mgos_arduino_dt_is_parasite_power_mode(DallasTemperature *dt) {
  if (dt == nullptr) return false;
  return dt->isParasitePowerMode();
}

bool mgos_arduino_dt_is_conversion_complete(DallasTemperature *dt) {
  if (dt == nullptr) return false;
  return dt->isConversionComplete();
}

int16_t mgos_arduino_dt_millis_to_wait_for_conversion(DallasTemperature *dt, int res) {
  if (dt == nullptr) return 0;
  return dt->millisToWaitForConversion(res);
}

void mgos_arduino_dt_set_high_alarm_temp(DallasTemperature *dt, const char *addr, char gradc) {
  if (dt == nullptr) return;
  dt->setHighAlarmTemp((uint8_t *)addr, gradc);
}

void mgos_arduino_dt_set_low_alarm_temp(DallasTemperature *dt, const char *addr, char gradc) {
  if (dt == nullptr) return;
  dt->setLowAlarmTemp((uint8_t *)addr, gradc);
}

char mgos_arduino_dt_get_high_alarm_temp(DallasTemperature *dt, const char *addr) {
  if (dt == nullptr) return DEVICE_DISCONNECTED_C;
  return dt->getHighAlarmTemp((uint8_t *)addr);
}

char mgos_arduino_dt_get_low_alarm_temp(DallasTemperature *dt, const char *addr) {
  if (dt == nullptr) return DEVICE_DISCONNECTED_C;
  return dt->getLowAlarmTemp((uint8_t *)addr);
}

void mgos_arduino_dt_reset_alarm_search(DallasTemperature *dt) {
  if (dt == nullptr) return;
  dt->resetAlarmSearch();
}

bool mgos_arduino_dt_alarm_search(DallasTemperature *dt, char *new_addr) {
  if (dt == nullptr) return false;
  return dt->alarmSearch((uint8_t *)new_addr);
}

bool mgos_arduino_dt_has_alarm(DallasTemperature *dt, const char *addr) {
  if (dt == nullptr) return false;
  return dt->hasAlarm((uint8_t *)addr);
}

bool mgos_arduino_dt_has_alarms(DallasTemperature *dt) {
  if (dt == nullptr) return false;
  return dt->hasAlarm();
}
