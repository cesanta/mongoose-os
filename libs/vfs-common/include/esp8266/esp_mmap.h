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

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP8266_MMAP_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP8266_MMAP_H_

#include "esp_fs.h"

#ifdef CS_MMAP

#ifdef __cplusplus
extern "C" {
#endif

/*
 * See common/platforms/esp/src/esp_mmap.h for details of these values
 */

#define MMAP_BASE ((void *) 0x10000000)
#define MMAP_END ((void *) 0x20000000)

#define MMAP_ADDR_BITS 20
#define MMAP_NUM_BITS 8

#define DUMMY_MMAP_BUFFER_START ((u8_t *) 0x70000000)
#define DUMMY_MMAP_BUFFER_END ((u8_t *) 0x70100000)

#define FLASH_READ_BYTE(addr) (*(addr))

#define FLASH_BASE(fs) 0x40200000

#ifdef __cplusplus
}
#endif

#endif /* CS_MMAP */
#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP8266_MMAP_H_ */
