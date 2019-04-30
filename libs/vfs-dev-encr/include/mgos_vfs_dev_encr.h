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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "mgos_vfs_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_VFS_DEV_TYPE_ENCR "encr"

enum mgos_vfs_dev_err encr_dev_init(struct mgos_vfs_dev *dev,
                                    struct mgos_vfs_dev *io_dev,
                                    const uint8_t *key,
                                    struct mgos_vfs_dev *key_dev, int key_len,
                                    bool testing);

#ifdef __cplusplus
}
#endif
