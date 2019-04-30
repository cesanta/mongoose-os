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

#ifndef CS_FW_PLATFORMS_ESP32_SRC_ESP32_FS_H_
#define CS_FW_PLATFORMS_ESP32_SRC_ESP32_FS_H_

#include <esp_vfs.h> /* for esp_vfs_translate_fd */
#include <stdbool.h>

#include "esp_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

const esp_partition_t *esp32_find_fs_for_app_slot(int app_slot);

bool esp32_fs_crypt_init(void);

bool esp32_fs_mount_part(const char *label, const char *path,
                         const char *fs_type, const char *fs_opts);

#define SUBTYPE_TO_SLOT(st) ((st) -ESP_PARTITION_SUBTYPE_OTA(0))
#define NUM_OTA_SLOTS \
  (ESP_PARTITION_SUBTYPE_APP_OTA_MAX - ESP_PARTITION_SUBTYPE_APP_OTA_MIN)
int esp32_get_boot_slot();

/*
 * Translate file descriptor returned by open() to the one suitable for use
 * by mgos_vfs_*
 */
#define ARCH_FD_TO_VFS_FD(fd) esp_vfs_translate_fd(fd, NULL)

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_ESP32_SRC_ESP32_FS_H_ */
