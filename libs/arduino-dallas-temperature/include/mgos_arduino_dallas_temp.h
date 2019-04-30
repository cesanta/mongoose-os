/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _MGOS_ARDUINO_DALLAS_TEMP_H_
#define _MGOS_ARDUINO_DALLAS_TEMP_H_

#include "mgos_arduino_onewire.h"
#ifdef __cplusplus
#include "DallasTemperature.h"
#else
typedef struct DallasTemperatureTag DallasTemperature;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Initialize DallasTemperature driver.
// Return value: handle opaque pointer.
DallasTemperature *mgos_arduino_dt_create(OneWire *ow);

// Close DallasTemperature handle. Return value: none.
void mgos_arduino_dt_close(DallasTemperature *dt);

// Initialise 1-Wire bus.
void mgos_arduino_dt_begin(DallasTemperature *dt);

// Returns the number of devices found on the bus.
// Return always 0 if an operaiton failed.
int mgos_arduino_dt_get_device_count(DallasTemperature *dt);

// Returns true if address is valid.
// Return always false if an operaiton failed.
bool mgos_arduino_dt_valid_address(DallasTemperature *dt, const char *addr);

// Returns true if address is of the family of sensors the lib supports.
// Return always false if an operaiton failed.
bool mgos_arduino_dt_valid_family(DallasTemperature *dt, const char *addr);

// Finds an address at a given index on the bus.
// Return false if the device was not found or an operaiton failed.
// Returns true otherwise.
bool mgos_arduino_dt_get_address(DallasTemperature *dt, char *addr, int idx);

// Attempt to determine if the device at the given address is connected to the bus.
// Return false if the device is not connected or an operaiton failed.
// Returns true otherwise.
bool mgos_arduino_dt_is_connected(DallasTemperature *dt, const char *addr);

// Attempt to determine if the device at the given address is connected to the bus.
// Also allows for updating the read scratchpad.
// Return false if the device is not connected or an operaiton failed.
// Returns true otherwise.
bool mgos_arduino_dt_is_connected_sp(DallasTemperature *dt, const char *addr, char *sp);

// Read device's scratchpad.
// Return false if an operaiton failed.
// Returns true otherwise.
bool mgos_arduino_dt_read_scratch_pad(DallasTemperature *dt, const char *addr, char *sp);

// Write device's scratchpad.
void mgos_arduino_dt_write_scratch_pad(DallasTemperature *dt, const char *addr, const char *sp);

// Read device's power requirements.
// Return true if device needs parasite power.
// Return always false if an operaiton failed.
bool mgos_arduino_dt_read_power_supply(DallasTemperature *dt, const char *addr);

// Get global resolution.
int mgos_arduino_dt_get_global_resolution(DallasTemperature *dt);

// Set global resolution to 9, 10, 11, or 12 bits.
void mgos_arduino_dt_set_global_resolution(DallasTemperature *dt, int res);

// Returns the device resolution: 9, 10, 11, or 12 bits.
// Returns 0 if device not found or if an operaiton failed.
int mgos_arduino_dt_get_resolution(DallasTemperature *dt, const char *addr);

// Set resolution of a device to 9, 10, 11, or 12 bits.
// If new resolution is out of range, 9 bits is used.
// Return true if a new value was stored.
// Returns false otherwise.
bool mgos_arduino_dt_set_resolution(DallasTemperature *dt, const char *addr, int res, bool skip_global_calc);

// Sets the waitForConversion flag.
void mgos_arduino_dt_set_wait_for_conversion(DallasTemperature *dt, bool f);

// Gets the value of the waitForConversion flag.
// Return always false if an operaiton failed.
bool mgos_arduino_dt_get_wait_for_conversion(DallasTemperature *dt);

// Sets the checkForConversion flag.
void mgos_arduino_dt_set_check_for_conversion(DallasTemperature *dt, bool f);

// Gets the value of the waitForConversion flag.
// Return always false if an operaiton failed.
bool mgos_arduino_dt_get_check_for_conversion(DallasTemperature *dt);

// Sends command for all devices on the bus to perform a temperature conversion.
// Returns false if a device is disconnected or if an operaiton failed.
// Returns true otherwise.
void mgos_arduino_dt_request_temperatures(DallasTemperature *dt);

// Sends command for one device to perform a temperature conversion by address.
// Returns false if a device is disconnected or if an operaiton failed.
// Returns true otherwise.
bool mgos_arduino_dt_request_temperatures_by_address(DallasTemperature *dt, const char *addr);

// Sends command for one device to perform a temperature conversion by index.
// Returns false if a device is disconnected or if an operaiton failed.
// Returns true otherwise.
bool mgos_arduino_dt_request_temperatures_by_index(DallasTemperature *dt, int idx);

// Returns temperature raw value (12 bit integer of 1/128 degrees C)
// or DEVICE_DISCONNECTED_RAW if an operaiton failed.
int16_t mgos_arduino_dt_get_temp(DallasTemperature *dt, const char *addr);

// Returns temperature in degrees C * 100
// or DEVICE_DISCONNECTED_C if an operaiton failed.
int mgos_arduino_dt_get_tempc(DallasTemperature *dt, const char *addr);

// Returns temperature in degrees F * 100
// or DEVICE_DISCONNECTED_F if an operaiton failed.
int mgos_arduino_dt_get_tempf(DallasTemperature *dt, const char *addr);

// Returns temperature for device index in degrees C * 100 (slow)
// or DEVICE_DISCONNECTED_C if an operaiton failed.
int mgos_arduino_dt_get_tempc_by_index(DallasTemperature *dt, int idx);

// Returns temperature for device index in degrees F * 100 (slow)
// or DEVICE_DISCONNECTED_F if an operaiton failed.
int mgos_arduino_dt_get_tempf_by_index(DallasTemperature *dt, int idx);

// Returns true if the bus requires parasite power.
// Returns always false if an operaiton failed.
bool mgos_arduino_dt_is_parasite_power_mode(DallasTemperature *dt);

// Is a conversion complete on the wire?
// Return always false if an operaiton failed.
bool mgos_arduino_dt_is_conversion_complete(DallasTemperature *dt);

// Returns number of milliseconds to wait till conversion is complete (based on IC datasheet)
// or 0 if an operaiton failed.
int16_t mgos_arduino_dt_millis_to_wait_for_conversion(DallasTemperature *dt, int res);

// Sets the high alarm temperature for a device.
// Accepts a char. Valid range is -55C - 125C.
void mgos_arduino_dt_set_high_alarm_temp(DallasTemperature *dt, const char *addr, char gradc);

// Sets the low alarm temperature for a device.
// Accepts a char. Valid range is -55C - 125C.
void mgos_arduino_dt_set_low_alarm_temp(DallasTemperature *dt, const char *addr, char gradc);

// Returns a signed char with the current high alarm temperature for a device
// in the range -55C - 125C or DEVICE_DISCONNECTED_C if an operaiton failed
char mgos_arduino_dt_get_high_alarm_temp(DallasTemperature *dt, const char *addr);

// Returns a signed char with the current low alarm temperature for a device
// in the range -55C - 125C or DEVICE_DISCONNECTED_C if an operaiton failed
char mgos_arduino_dt_get_low_alarm_temp(DallasTemperature *dt, const char *addr);

// Resets internal variables used for the alarm search.
void mgos_arduino_dt_reset_alarm_search(DallasTemperature *dt);

// Search the wire for devices with active alarms.
// Returns true then it has enumerated the next device.
// Returns false if there are no devices or an operaiton failed.
// If a new device is found then its address is copied to param #2.
// Use mgos_arduino_dt_reset_alarm_search() to start over.
bool mgos_arduino_dt_alarm_search(DallasTemperature *dt, char *new_addr);

// Returns true if device address might have an alarm condition
// (only an alarm search can verify this).
// Return always false if an operaiton failed.
bool mgos_arduino_dt_has_alarm(DallasTemperature *dt, const char *addr);

// Returns true if any device is reporting an alarm on the bus
// Return always false if an operaiton failed.
bool mgos_arduino_dt_has_alarms(DallasTemperature *dt);

#ifdef __cplusplus
}
#endif

#endif /* _MGOS_ARDUINO_DALLAS_TEMP_H_ */
