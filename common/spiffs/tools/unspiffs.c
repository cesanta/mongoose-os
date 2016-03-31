#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mem_spiffs.h"

static void show_usage(char *argv[]) {
  fprintf(stderr, "usage: %s [-l] [-d extdir] <filename>\n", argv[0]);
  exit(1);
}

int main(int argc, char **argv) {
  FILE *fp;
  const char *filename = NULL;
  int i;
  int list = 0, vis = 0;
  const char *extDir = ".";

  for (i = 1; i < argc && argv[i][0] == '-'; i++) {
    if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
      extDir = argv[i + 1];
      i++;
    } else if (strcmp(argv[i], "-v") == 0) {
      vis = 1;
    } else if (strcmp(argv[i], "-l") == 0) {
      list = 1;
    } else if (strcmp(argv[i], "-h") == 0) {
      show_usage(argv);
    }
  }

  if (argc - i < 1) {
    show_usage(argv);
  }

  filename = argv[i];
  fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "unable to open %s, err: %d\n", filename, errno);
    return 1;
  }

  fseek(fp, 0, SEEK_END);
  image_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  image = (char *) malloc(image_size);
  if (fread(image, image_size, 1, fp) < 1) {
    fprintf(stderr, "cannot read %s, err: %d\n", filename, errno);
    return 1;
  }

  mem_spiffs_mount();

  if (vis) SPIFFS_vis(&fs);

  {
    spiffs_DIR d;
    struct spiffs_dirent de;
    SPIFFS_opendir(&fs, ".", &d);

    while (SPIFFS_readdir(&d, &de) != NULL) {
      if (list) {
        printf("%s\n", de.name);
      } else {
        char target[1024];
        char *buf = NULL;
        FILE *out;
        spiffs_file in;

        sprintf(target, "%s/%s", extDir, de.name);

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
  fclose(fp);

  return 0;
}
