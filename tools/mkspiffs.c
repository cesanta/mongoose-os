#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../src/spiffs/spiffs.h"

#define LOG_PAGE_SIZE 256
#define FLASH_BLOCK_SIZE (4 * 1024)

spiffs fs;
u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
u8_t spiffs_fds[32 * 4];

char *image; /* in memory flash image */
size_t image_size;

s32_t mem_spiffs_read(u32_t addr, u32_t size, u8_t *dst) {
  memcpy(dst, image + addr, size);
  return SPIFFS_OK;
}

s32_t mem_spiffs_write(u32_t addr, u32_t size, u8_t *src) {
  memcpy(image + addr, src, size);
  return SPIFFS_OK;
}

s32_t mem_spiffs_erase(u32_t addr, u32_t size) {
  memset(image + addr, 0xff, size);
  return SPIFFS_OK;
}

void mem_spiffs_mount() {
  spiffs_config cfg;

  cfg.phys_size = image_size;
  cfg.phys_addr = 0;

  cfg.phys_erase_block = FLASH_BLOCK_SIZE;
  cfg.log_block_size = FLASH_BLOCK_SIZE;
  cfg.log_page_size = LOG_PAGE_SIZE;

  cfg.hal_read_f = mem_spiffs_read;
  cfg.hal_write_f = mem_spiffs_write;
  cfg.hal_erase_f = mem_spiffs_erase;

  if (SPIFFS_mount(&fs, &cfg, spiffs_work_buf, spiffs_fds, sizeof(spiffs_fds),
                   0, 0, 0) == -1) {
    fprintf(stderr, "SPIFFS_mount failed: %d\n", SPIFFS_errno(&fs));
  }
}

void copy(char *path, char *fname) {
  int size;
  u8_t *buf;
  struct stat st;
  spiffs_file sfd;
  int ifd;

  ifd = open(path, O_RDONLY);
  if (ifd == -1) {
    fprintf(stderr, "cannot open %s\n", path);
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
    fprintf(stderr, "usage: %s <size> <root_dir>\n", argv[0]);
    return 1;
  }

  image_size = atoi(argv[1]);
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

  mem_spiffs_erase(0, image_size);
  mem_spiffs_mount();

  fprintf(stderr, "adding files in directory %s\n", root_dir);
  if ((dir = opendir(root_dir)) == NULL) {
    fprintf(stderr, "unable to open directory %s\n", root_dir);
    return 1;
  } else {
    read_dir(dir, root_dir);
  }

  fwrite(image, image_size, 1, stdout);
  return 0;
}
