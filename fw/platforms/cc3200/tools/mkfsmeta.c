#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/cc3200_fs_spiffs_container_meta.h"

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
