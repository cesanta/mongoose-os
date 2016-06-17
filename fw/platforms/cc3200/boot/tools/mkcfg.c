#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/boot.h"

int main(int argc, char **argv) {
  struct boot_cfg cfg;
  if (argc < 3) return 1;
  memset(&cfg, 0, sizeof(cfg));
  strcpy(cfg.image_file, argv[1]);
  cfg.base_address = strtol(argv[2], NULL, 0);
  if (argc == 4) {
    cfg.seq = strtoll(argv[3], NULL, 0);
  } else {
    cfg.seq = BOOT_CFG_INITIAL_SEQ;
  }
  fwrite(&cfg, sizeof(cfg), 1, stdout);
  return 0;
}
