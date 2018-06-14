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

#ifndef CS_FW_PLATFORMS_CC3220_SRC_MG_LOCALS_H_
#define CS_FW_PLATFORMS_CC3220_SRC_MG_LOCALS_H_

#include <dirent.h>

#include "mgos_debug.h"
#define MG_UART_WRITE(fd, buf, len) mgos_debug_write(fd, buf, len)

#ifdef __cplusplus
extern "C" {
#endif

void fs_slfs_set_new_file_size(const char *name, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC3220_SRC_MG_LOCALS_H_ */
