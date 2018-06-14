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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cc3200_vfs_dev_slfs_container_meta.h"

int main(int argc, char **argv) {
  if (argc < 5) return 1;
  union fs_container_meta meta;
  memset(&meta, 0, sizeof(meta));
  meta.info.fs_size = strtol(argv[1], NULL, 0);
  meta.info.fs_block_size = strtol(argv[2], NULL, 0);
  meta.info.fs_page_size = strtol(argv[3], NULL, 0);
  meta.info.fs_erase_size = strtol(argv[4], NULL, 0);
  if (argc == 6) {
    meta.info.seq = strtoull(argv[5], NULL, 0);
  } else {
    meta.info.seq = FS_INITIAL_SEQ;
  }
  fwrite(&meta, sizeof(meta), 1, stdout);
  return 0;
}
