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

#ifndef CS_MOS_LIBS_ONEWIRE_SRC_MGOS_ONEWIRE_H_
#define CS_MOS_LIBS_ONEWIRE_SRC_MGOS_ONEWIRE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mgos_onewire;

/*
 * Calculate CRC8 of the given chunk of memory.
 */
uint8_t mgos_onewire_crc8(const uint8_t *rom, int len);

/*
 * Create onewire instance on a given pin.
 */
struct mgos_onewire *mgos_onewire_create(int pin);

/*
 * Reset onewire bus. Usually this is needed before communicating with any
 * device.
 */
bool mgos_onewire_reset(struct mgos_onewire *ow);

/*
 * Setup the search to find the device type 'family_code' on the next call
 * mgos_onewire_next() if it is present Note if no devices of the desired
 * family are currently on the 1-Wire, then another type will be found.
 */
void mgos_onewire_target_setup(struct mgos_onewire *ow,
                               const uint8_t family_code);
/*
 * Search for the next device. The given `rom` should point to a chunk of at
 * least 8 bytes; result will be written there. `mode` is as follows:
 * 0 - normal search, 1 - conditional search.
 */
bool mgos_onewire_next(struct mgos_onewire *ow, uint8_t *rom, int mode);

/*
 * Select a device based on is address `rom`, which is a 8-byte string. After
 * a reset, this is needed to choose which device you will use, and then all
 * communication will be with that device, until another reset.
 */
void mgos_onewire_select(struct mgos_onewire *ow, const uint8_t *rom);

/*
 * Skip the device selection. This only works if you have a single device, but
 * you can avoid searching and use this to immediately access your device.
 */
void mgos_onewire_skip(struct mgos_onewire *ow);

/*
 * Reset a search. Next use of `mgos_onewire_next` will begin at the first
 * device.
 */
void mgos_onewire_search_clean(struct mgos_onewire *ow);

/*
 * Read a single bit from the onewire bus. Returned value is `true` for 1, or
 * `false` for 0.
 */
bool mgos_onewire_read_bit(struct mgos_onewire *ow);

/*
 * Read a byte from the onewire bus.
 */
uint8_t mgos_onewire_read(struct mgos_onewire *ow);

/*
 * Read `len` bytes from the onewire bus to the buffer `buf`.
 */
void mgos_onewire_read_bytes(struct mgos_onewire *ow, uint8_t *buf, int len);

/*
 * Write a single bit to the onewire bus; given `bit` should be either `0` or
 * `1`.
 */
void mgos_onewire_write_bit(struct mgos_onewire *ow, int bit);

/*
 * Write a byte to the onewire bus.
 */
void mgos_onewire_write(struct mgos_onewire *ow, const uint8_t data);

/*
 * Write `len` bytes to the onewire bus from `buf`.
 */
void mgos_onewire_write_bytes(struct mgos_onewire *ow, const uint8_t *buf,
                              int len);

/*
 * Close onewire instance and free the occupied memory.
 */
void mgos_onewire_close(struct mgos_onewire *ow);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_ONEWIRE_SRC_MGOS_ONEWIRE_H_ */
