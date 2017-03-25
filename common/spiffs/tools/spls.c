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

int remove_f(const char *fname) {
  fprintf(stderr, "remove %s\n", fname);
  return SPIFFS_remove(&fs, fname);
}

int main(int argc, char **argv) {
  const char *image_file = argv[1];
  uint32_t expected_crc = 0;

  struct stat st;
  if (stat(image_file, &st) != 0) {
    fprintf(stderr, "failed to stat %s: %d\n", image_file, errno);
    return 1;
  }

  if (argc > 2) {
    expected_crc = strtoul(argv[2], NULL, 0);
    fprintf(stderr, "expected CRC: 0x%08x\n", expected_crc);
  }

  image_size = st.st_size;
  fprintf(stderr, "image size: %d\n", (int) image_size);

  image = malloc(image_size);
  if (image == NULL) {
    fprintf(stderr, "cannot allocate %lu bytes\n", image_size);
    return 1;
  }

  FILE *in = fopen(image_file, "r");
  if (in == NULL) {
    fprintf(stderr, "cannot open %s: %d\n", image_file, errno);
    return 1;
  }

  if (fread(image, 1, image_size, in) != image_size) {
    fprintf(stderr, "failed to read image data\n");
    return 1;
  }

  fclose(in);

  if (mem_spiffs_mount() != SPIFFS_OK) {
    fprintf(stderr, "SPIFFS_mount failed: %d\n", SPIFFS_errno(&fs));
    return 2;
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
