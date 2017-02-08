/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "/usr/include/dirent.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mem_spiffs.h"

bool copy(char *src, char *dst) {
  bool result = false;
  int size;
  u8_t *buf = NULL;
  struct stat st;
  spiffs_file sfd;
  int ifd = -1;

  fprintf(stderr, "     Adding %s: ", dst);

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

  if ((sfd = SPIFFS_open(&fs, dst, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR,
                         0)) == -1) {
    fprintf(stderr, "SPIFFS_open failed: %d\n", SPIFFS_errno(&fs));
    goto cleanup;
  }

  if (SPIFFS_write(&fs, sfd, (uint8_t *) buf, size) == -1) {
    fprintf(stderr, "SPIFFS_write failed: %d\n", SPIFFS_errno(&fs));
    goto spiffs_cleanup;
  }

  fprintf(stderr, "%d\n", (int) size);
  result = true;

spiffs_cleanup:
  SPIFFS_close(&fs, sfd);
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

int main(int argc, char **argv) {
  const char *root_dir;
  DIR *dir;

  if (argc < 3) {
    fprintf(stderr, "usage: %s <size> <root_dir> [image_file]\n", argv[0]);
    return 1;
  }

  image_size = (size_t) strtol(argv[1], NULL, 0);
  if (image_size == 0) {
    fprintf(stderr, "invalid size '%s'\n", argv[1]);
    return 1;
  }

  root_dir = argv[2];

  image = malloc(image_size);
  if (image == NULL) {
    fprintf(stderr, "cannot allocate %lu bytes\n", image_size);
    return 1;
  }

  mem_spiffs_erase(NULL, 0, image_size);
  mem_spiffs_mount();  // Will fail but is required.
  SPIFFS_format(&fs);
  if (mem_spiffs_mount() != SPIFFS_OK) {
    fprintf(stderr, "SPIFFS_mount failed: %d\n", SPIFFS_errno(&fs));
    return 1;
  }

  if ((dir = opendir(root_dir)) == NULL) {
    fprintf(stderr, "unable to open directory %s\n", root_dir);
    return 1;
  } else {
    if (!read_dir(dir, root_dir)) {
      return 1;
    }
  }

  u32_t total, used;
  SPIFFS_info(&fs, &total, &used);
  fprintf(stderr, "     Image stats: size=%u, space: total=%u, used=%u, free=%u\n",
          (unsigned int) image_size, total, used, total - used);

  FILE *out = stdout;
  if (argc == 4) {
    out = fopen(argv[3], "w");
    if (out == NULL) {
      fprintf(stderr, "failed to open %s for writing\n", argv[3]);
      return 1;
    }
  }

  fwrite(image, image_size, 1, out);

  if (out != stdout) fclose(out);

  return 0;
}
