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
#include <errno.h>
#include <unistd.h>

#include "mem_spiffs.h"

static void show_usage(char *argv[]) {
  fprintf(stderr, "usage: %s [-l] [-d extdir] [-v] <filename>\n", argv[0]);
  exit(1);
}

int main(int argc, char **argv) {
  int opt;
  int list = 0, vis = 0;
  const char *ext_dir = NULL;

  int bs = FS_BLOCK_SIZE, ps = FS_PAGE_SIZE, es = FS_ERASE_SIZE;

  while ((opt = getopt(argc, argv, "b:d:e:lp:v")) != -1) {
    switch (opt) {
      case 'b': {
        bs = (size_t) strtol(optarg, NULL, 0);
        if (bs == 0) {
          fprintf(stderr, "invalid fs block size '%s'\n", optarg);
          return 1;
        }
        break;
      }
      case 'd': {
        ext_dir = optarg;
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
      case 'l': {
        list = 1;
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
      case 'v': {
        vis = 1;
        break;
      }
    }
  }

  if (argc - optind < 1) {
    show_usage(argv);
  }

  if (mem_spiffs_mount_file(argv[optind], bs, ps, es) != SPIFFS_OK) {
    return 1;
  }

  if (vis) SPIFFS_vis(&fs);

  {
    spiffs_DIR d;
    struct spiffs_dirent de;
    SPIFFS_opendir(&fs, ".", &d);

    while (SPIFFS_readdir(&d, &de) != NULL) {
      if (list) {
        printf("%s %d\n", de.name, de.size);
      } else if (ext_dir != NULL) {
        char target[1024];
        char *buf = NULL;
        FILE *out;
        spiffs_file in;

        sprintf(target, "%s/%s", ext_dir, de.name);

        fprintf(stderr, "extracting %s\n", de.name);
        out = fopen(target, "w");
        if (out == NULL) {
          fprintf(stderr, "cannot write %s, err: %d\n", target, errno);
          return 1;
        }

        in = SPIFFS_open_by_dirent(&fs, &de, SPIFFS_RDONLY, 0);
        if (in < 0) {
          fprintf(stderr, "cannot open spiffs file %s, err: %d\n", de.name,
                  SPIFFS_errno(&fs));
          return 1;
        }

        buf = malloc(de.size);
        if (SPIFFS_read(&fs, in, buf, de.size) != de.size) {
          fprintf(stderr, "cannot read %s, err: %d\n", de.name,
                  SPIFFS_errno(&fs));
          return 1;
        }

        SPIFFS_close(&fs, in);
        fwrite(buf, de.size, 1, out);
        free(buf);
        fclose(out);
      }
    }

    SPIFFS_closedir(&d);
  }

  free(image);

  return 0;
}
