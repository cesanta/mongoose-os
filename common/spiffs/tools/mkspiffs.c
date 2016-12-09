#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

#include "mem_spiffs.h"

void copy(char *path, char *fname) {
  int size;
  u8_t *buf;
  struct stat st;
  spiffs_file sfd;
  int ifd;

  ifd = open(path, O_RDONLY);
  if (ifd == -1) {
    fprintf(stderr, "cannot open %s\n", path);
    perror("cannot open");
    return;
  }

  if (fstat(ifd, &st) == -1) {
    fprintf(stderr, "cannot stat %s\n", path);
    return;
  }
  size = st.st_size;

  if (size == -1) {
    fprintf(stderr, "skipping %s\n", path);
    return;
  }

  buf = malloc(size);

  if (read(ifd, buf, size) != size) {
    fprintf(stderr, "unable to read file %s\n", fname);
    return;
  }

  if ((sfd = SPIFFS_open(&fs, fname, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR,
                         0)) == -1) {
    fprintf(stderr, "SPIFFS_open %s failed: %d\n", fname, SPIFFS_errno(&fs));
    goto cleanup;
  }

  if (SPIFFS_write(&fs, sfd, (uint8_t *) buf, size) == -1) {
    fprintf(stderr, "SPIFFS_write %s failed: %d\n", fname, SPIFFS_errno(&fs));
    goto spifs_cleanup;
  }

  fprintf(stderr, "a %s\n", fname);

spifs_cleanup:
  SPIFFS_close(&fs, sfd);
cleanup:
  free(buf);
  close(ifd);
}

void read_dir(DIR *dir, const char *dir_path) {
  char path[512];
  struct dirent *ent;

  while ((ent = readdir(dir)) != NULL) {
    if (ent->d_name[0] == '.') { /* Excludes ".", ".." and hidden files. */
      continue;
    }
    sprintf(path, "%s/%s", dir_path, ent->d_name);
    copy(path, ent->d_name);
  }
  closedir(dir);
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

  fprintf(stderr, "adding files in directory %s\n", root_dir);
  if ((dir = opendir(root_dir)) == NULL) {
    fprintf(stderr, "unable to open directory %s\n", root_dir);
    return 1;
  } else {
    read_dir(dir, root_dir);
  }

  u32_t total, used;
  SPIFFS_info(&fs, &total, &used);
  fprintf(stderr, "Image stats: size=%u, space: total=%u, used=%u, free=%u\n",
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
