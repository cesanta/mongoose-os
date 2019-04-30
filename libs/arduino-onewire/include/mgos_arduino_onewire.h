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

#ifndef _MGOS_ARDUINO_ONEWIRE_H_
#define _MGOS_ARDUINO_ONEWIRE_H_

#ifdef __cplusplus
#include "OneWire.h"
#else
typedef struct OneWireTag OneWire;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Create a OneWire object instance. Return value: an object with the methods
 * described below.
 */
OneWire *mgos_arduino_onewire_create(uint8_t pin);

/*
 * Destroy an `ow` instance.
 */
void mgos_arduino_onewire_close(OneWire *ow);

/*
 * Reset the 1-wire bus. Usually this is needed before communicating with any
 * device.
 */
uint8_t mgos_arduino_onewire_reset(OneWire *ow);

/*
 * Select a device based on its address `addr`, which is a 8-byte array.
 * After a reset, this is needed to choose which device you will use, and then
 * all communication will be with that device, until another reset.
 *
 * Example:
 * ```c
 * uint8_t addr[] = {0x28, 0xff, 0x2b, 0x45, 0x4c, 0x04, 0x00, 0x10};
 * mgos_arduino_onewire_select(myow, addr);
 * ```
 */
void mgos_arduino_onewire_select(OneWire *ow, const uint8_t *addr);

/*
 * Skip the device selection. This only works if you have a single device, but
 * you can avoid searching and use this to immediately access your device.
 */
void mgos_arduino_onewire_skip(OneWire *ow);

/*
 * Write a byte to the onewire bus.
 *
 * Example:
 * ```c
 * // Write 0x12 to the onewire bus
 * mgos_arduino_onewire_write(myow, 0x12);
 * ```
 */
void mgos_arduino_onewire_write(OneWire *ow, uint8_t v);

/*
 * Write `count` bytes from the buffer `buf`. Example:
 *
 * ```c
 * // Write [0x55, 0x66, 0x77] to the onewire bus
 * const uint8_t buf[] = {0xff, 0x66, 0x77};
 * mgos_arduino_onewire_write_bytes(myow, buf, sizeof(buf));
 * ```
 */
void mgos_arduino_onewire_write_bytes(OneWire *ow, const uint8_t *buf,
                                      uint16_t count);

/*
 * Read a byte from the onewire bus.
 */
uint8_t mgos_arduino_onewire_read(OneWire *ow);

/*
 * Read multiple bytes from the onewire bus and write them to `buf`. The given
 * buffer should be at least `count` bytes long.
 */
void mgos_arduino_onewire_read_bytes(OneWire *ow, uint8_t *buf, uint16_t count);

/*
 * Write a single bit to the onewire bus. Given `v` should be either 0 or 1.
 */
void mgos_arduino_onewire_write_bit(OneWire *ow, uint8_t v);

/*
 * Read a single bit from the onewire bus. Returned value is either 0 or 1.
 */
uint8_t mgos_arduino_onewire_read_bit(OneWire *ow);

/*
 * Not implemented yet.
 */
void mgos_arduino_onewire_depower(OneWire *ow);

/*
 * Search for the next device. The given `addr` should point to the buffer with
 * at least 8 bytes. If a device is found, `addr` is
 * filled with the device's address and 1 is returned. If no more
 * devices are found, 0 is returned.
 * `mode` is an integer: 0 means normal search, 1 means conditional search.
 * Example:
 * ```c
 * uint8_t addr[8];
 * uint8_t res = mgos_arduino_onewire_search(addr, 0);
 * if (res) {
 *   // addr contains the next device's address
 *   printf("Found!");
 * } else {
 *   printf("Not found");
 * }
 * ```
 */
uint8_t mgos_arduino_onewire_search(OneWire *ow, uint8_t *newAddr,
                                    uint8_t search_mode);

/*
 * Reset a search. Next use of `mgos_arduino_onewire_search(....)` will begin
 * at the first device.
 */
void mgos_arduino_onewire_reset_search(OneWire *ow);

/*
 * Setup the search to find the device type 'fc' (family code) on the next call
 * to `mgos_arduino_onewire_search()` if it is present.
 *
 * If no devices of the desired family are currently on the bus, then
 * device of some another type will be found by `search()`.
 */
void mgos_arduino_onewire_target_search(OneWire *ow, uint8_t family_code);

/*
 * Calculate crc8 for the given buffer `addr`, `len`.
 */
uint8_t mgos_arduino_onewire_crc8(OneWire *ow, const uint8_t *addr,
                                  uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* _MGOS_ARDUINO_ONEWIRE_H_ */
