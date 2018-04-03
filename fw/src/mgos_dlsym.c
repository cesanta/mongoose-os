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

#include <string.h>

#include "mgos_dlsym.h"

extern struct mgos_ffi_export ffi_exports[];
extern int ffi_exports_cnt;

void *mgos_dlsym(void *handle, const char *name) {
  (void) handle;
  int i;
  for (i = 0; i < ffi_exports_cnt; i++) {
    if (strcmp(name, ffi_exports[i].name) == 0) return ffi_exports[i].addr;
  }
  return NULL;
}
