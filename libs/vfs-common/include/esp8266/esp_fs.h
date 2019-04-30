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

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP_FS_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP_FS_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_SECTOR_SIZE 0x1000
#define FLASH_ERASE_BLOCK_SIZE 0x10000

bool esp_fs_mount(uint32_t addr, uint32_t size, const char *dev_name,
                  const char *path);

/*
 * Translate file descriptor returned by open() to the one suitable for use
 * by mgos_vfs_*
 */
#define ARCH_FD_TO_VFS_FD(fd) (fd)

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP_FS_H_ */
