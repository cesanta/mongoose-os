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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "frozen.h"

#include "mem_lfs.h"

bool copy(char *src, char *dst) {
  bool result = false;
  int size;
  uint8_t *buf = NULL;
  struct stat st;
  lfs_file_t f;
  int ifd = -1;

  fprintf(stderr, "       Adding %s: ", dst);

  ifd = open(src, O_RDONLY);
  if (ifd < 0) {
    fprintf(stderr, "cannot open %s\n", src);
    perror("cannot open");
    goto cleanup;
  }

  if (fstat(ifd, &st) < 0) {
    fprintf(stderr, "cannot stat %s\n", src);
    goto cleanup;
  }
  size = st.st_size;

  if (size < 0) {
    fprintf(stderr, "skipping %s\n", src);
    goto cleanup;
  }

  buf = malloc(size);

  if (read(ifd, buf, size) != size) {
    fprintf(stderr, "failed to read source file\n");
    goto cleanup;
  }

  int r;
  lfs_t *fs = mem_lfs_get();
  if ((r = lfs_file_open(fs, &f, dst, LFS_O_CREAT | LFS_O_TRUNC | LFS_O_RDWR)) <
      0) {
    fprintf(stderr, "lfs_file_open failed: %d\n", r);
    goto cleanup;
  }

  if ((r = lfs_file_write(fs, &f, buf, size)) != size) {
    fprintf(stderr, "lfs_file_write failed: %d\n", r);
    if (r == LFS_ERR_NOSPC) {
      fprintf(stderr, "*** Out of space, tried to write %d bytes ***\n",
              (int) size);
    }
    goto spiffs_cleanup;
  }

  fprintf(stderr, "%d\n", (int) size);
  result = true;

spiffs_cleanup:
  lfs_file_close(fs, &f);
cleanup:
  free(buf);
  if (ifd >= 0) close(ifd);
  return result;
}

bool read_dir(DIR *dir, const char *dir_path) {
  bool result = false;
  char path[512];
  struct dirent *ent;

  while ((ent = readdir(dir)) != NULL) {
    if (ent->d_name[0] == '.') { /* Excludes ".", ".." and hidden files. */
      continue;
    }
    sprintf(path, "%s/%s", dir_path, ent->d_name);
    if (!copy(path, ent->d_name)) {
      goto cleanup;
    }
  }
  result = true;
cleanup:
  closedir(dir);
  return result;
}

void show_usage(char *argv[], const char *err_msg) {
  if (err_msg != NULL) fprintf(stderr, "Error: %s\r\n", err_msg);
  fprintf(stderr,
          "usage: %s [-u] [-d] [-s fs_size] [-b block_size] "
          "[-f image_file] [-o json_opts] <root_dir>\n",
          argv[0]);
  exit(1);
}

static int count_used(void *arg, lfs_block_t block) {
  (void) block;
  (*((uint32_t *) arg))++;
  return LFS_ERR_OK;
}

int main(int argc, char **argv) {
  int opt, r;
  const char *root_dir = NULL;
  const char *image_file = NULL;
  DIR *dir;
  bool update = false;
  int fs_size = -1, bs = 4096;

  while ((opt = getopt(argc, argv, "b:df:o:s:u")) != -1) {
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
        log_reads = log_writes = log_erases = true;
        break;
      }
      case 'f': {
        image_file = optarg;
        break;
      }
      case 'o': {
        json_scanf(optarg, strlen(optarg), "{size: %u, bs: %u}", &fs_size, &bs);
        break;
      }
      case 's': {
        fs_size = (size_t) strtol(optarg, NULL, 0);
        if (fs_size == 0) {
          fprintf(stderr, "invalid fs size '%s'\n", optarg);
          return 1;
        }
        break;
      }
      case 'u': {
        update = true;
        break;
      }
    }
  }

  if (update) {
    if (image_file == NULL) show_usage(argv, "Image file is required with -u");
    if (mem_lfs_mount_file(image_file, bs) != LFS_ERR_OK) {
      return 1;
    }
  } else {
    if (fs_size == -1) {
      show_usage(argv, "-s is required when creating an image");
    }
    if ((r = mem_lfs_format(fs_size, bs)) != LFS_ERR_OK) {
      fprintf(stderr, "lfs_format failed: %d. wrong parameters?\n", r);
      return 1;
    }
  }

  root_dir = argv[optind++];
  if ((dir = opendir(root_dir)) == NULL) {
    fprintf(stderr, "unable to open directory %s\n", root_dir);
    return 1;
  } else {
    fprintf(stderr, "     FS params: size=%u, bs=%u\n", fs_size, bs);
    if (!read_dir(dir, root_dir)) {
      return 1;
    }
  }

  uint32_t total = fs_size, used = 0;
  lfs_traverse(mem_lfs_get(), count_used, &used);
  used *= bs;
  fprintf(stderr, "     FS stats : space total=%u, used=%u, free=%u\n", total,
          used, total - used);

  return mem_lfs_dump(image_file) ? 0 : 2;
}
