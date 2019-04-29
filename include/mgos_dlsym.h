/*
 * Copyright (c) 2018 Cesanta Software Limited
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

#ifndef CS_FW_INCLUDE_MGOS_DLSYM_H_
#define CS_FW_INCLUDE_MGOS_DLSYM_H_

#include "common/mg_str.h"

#include "mgos_features.h"

struct mgos_ffi_export {
  const char *name;
  void *addr;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void *mgos_dlsym(void *handle, const char *name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_INCLUDE_MGOS_DLSYM_H_ */
