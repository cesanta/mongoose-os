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

/* There are no includes here because once stdbool is included, "true" is
 * stringized to "1". */

#define STRINGIZE_LIT(...) #__VA_ARGS__
#define STRINGIZE(x) STRINGIZE_LIT(x)

#ifdef MGOS_ROOT_FS_TYPE
const char *mgos_vfs_get_root_fs_type(void) {
  return STRINGIZE(MGOS_ROOT_FS_TYPE);
}
#endif

#ifdef MGOS_ROOT_FS_TYPE
const char *mgos_vfs_get_root_fs_opts(void) {
  return STRINGIZE(MGOS_ROOT_FS_OPTS);
}
#endif
