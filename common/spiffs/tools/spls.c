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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mem_spiffs.h"

int remove_f(const char *fname) {
  fprintf(stderr, "remove %s\n", fname);
  return SPIFFS_remove(&fs, fname);
}

int main(int argc, char **argv) {
  uint32_t expected_crc = 0;
  int opt, bs = FS_BLOCK_SIZE, ps = FS_PAGE_SIZE, es = FS_ERASE_SIZE;

  while ((opt = getopt(argc, argv, "b:e:p:c:")) != -1) {
    switch (opt) {
      case 'b': {
        bs = (size_t) strtol(optarg, NULL, 0);
        if (bs == 0) {
          fprintf(stderr, "invalid fs block size '%s'\n", optarg);
          return 1;
        }
        break;
      }
      case 'e': {
        es = (size_t) strtol(optarg, NULL, 0);
        if (es == 0) {
          fprintf(stderr, "invalid fs erase size '%s'\n", optarg);
          return 1;
        }
        break;
      }
      case 'p': {
        ps = (size_t) strtol(optarg, NULL, 0);
        if (ps == 0) {
          fprintf(stderr, "invalid fs page size '%s'\n", optarg);
          return 1;
        }
        break;
      }
      case 'c': {
        expected_crc = (uint32_t) strtoul(optarg, NULL, 0);
        if (ps == 0) {
          fprintf(stderr, "invalid crc value '%s'\n", optarg);
          return 1;
        }
        break;
      }
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "Image file name required\n");
    return 1;
  }

  const char *image_file = argv[optind];

  if (mem_spiffs_mount_file(image_file, bs, ps, es) != SPIFFS_OK) {
    fprintf(stderr, "Failed to mount FS image file\n");
    return 1;
  }

  SPIFFS_vis(&fs);

  uint32_t crc32;
  crc32 = list_files();
  if (crc32 == 0) {
    return 3;
  }

  if (expected_crc != 0 && crc32 != expected_crc) {
    fprintf(stderr, "CRC mismatch!\n");
    return 123;
  }

  return 0;
}
