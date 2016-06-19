#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/boot_meta.h"

int main(int argc, char **argv) {
  if (argc < 4) return 1;
  union boot_cfg_meta meta;
  memset(&meta, 0, sizeof(meta));
  meta.cfg.flags = 0;
  strcpy(meta.cfg.app_image_file, argv[1]);
  meta.cfg.app_load_addr = strtol(argv[2], NULL, 0);
  strcpy(meta.cfg.fs_container_prefix, argv[3]);
  if (argc == 5) {
    meta.cfg.seq = strtoull(argv[4], NULL, 0);
  } else {
    meta.cfg.seq = BOOT_CFG_INITIAL_SEQ;
  }
  fwrite(&meta, sizeof(meta), 1, stdout);
  return 0;
}
