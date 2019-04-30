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

#include "mem_lfs.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <lfs.h>

#define STRINGIFY(a) STRINGIFY2(a)
#define STRINGIFY2(a) #a

static lfs_t s_lfs;
static struct lfs_config s_cfg;

char *image; /* in memory flash image */
bool log_reads = false, log_writes = false, log_erases = false;
int opr, opw, ope;
int wfail = -1;

lfs_t *mem_lfs_get(void) {
  return &s_lfs;
}

static int mem_lfs_read(const struct lfs_config *c, lfs_block_t block,
                        lfs_off_t off, void *buffer, lfs_size_t size) {
  if (log_reads) {
    fprintf(stderr, "R   #%04d %d @ %d+%d\n", opr, (int) size, (int) block,
            (int) off);
  }
  memcpy(buffer, image + (block * c->block_size) + off, size);
  opr++;
  return LFS_ERR_OK;
}

static int mem_lfs_prog(const struct lfs_config *c, lfs_block_t block,
                        lfs_off_t off, const void *buffer, lfs_size_t size) {
  if (log_writes) {
    fprintf(stderr, " W  #%04d %d @ %d+%d\n", opw, (int) size, (int) block,
            (int) off);
  }
  memcpy(image + block * c->block_size + off, buffer, size);
  opw++;
  return LFS_ERR_OK;
}

static int mem_lfs_erase(const struct lfs_config *c, lfs_block_t block) {
  if (log_erases) {
    fprintf(stderr, "  E #%04d %d @ %d\n", ope, (int) c->block_size,
            (int) block);
  }
  memset(image + block * c->block_size, 0xff, c->block_size);
  ope++;
  return LFS_ERR_OK;
}

static int mem_lfs_sync(const struct lfs_config *c) {
  (void) c;
  return LFS_ERR_OK;
}

int mem_lfs_mount(int fs_size, int bs) {
  memset(&s_lfs, 0, sizeof(s_lfs));
  memset(&s_cfg, 0, sizeof(s_cfg));

  s_cfg.read = mem_lfs_read;
  s_cfg.prog = mem_lfs_prog;
  s_cfg.erase = mem_lfs_erase;
  s_cfg.sync = mem_lfs_sync;

  s_cfg.read_size = 64;
  s_cfg.prog_size = 64;
  s_cfg.block_size = bs;
  s_cfg.lookahead = 1024;
  s_cfg.block_count = fs_size / bs;

  return lfs_mount(&s_lfs, &s_cfg);
}

int mem_lfs_format(int fs_size, int bs) {
  memset(&s_lfs, 0, sizeof(s_lfs));
  memset(&s_cfg, 0, sizeof(s_cfg));

  s_cfg.read = mem_lfs_read;
  s_cfg.prog = mem_lfs_prog;
  s_cfg.erase = mem_lfs_erase;
  s_cfg.sync = mem_lfs_sync;

  s_cfg.read_size = 64;
  s_cfg.prog_size = 64;
  s_cfg.block_size = bs;
  s_cfg.lookahead = 1024;
  s_cfg.block_count = fs_size / bs;

  image = (char *) malloc(fs_size);
  memset(image, 0xff, fs_size);

  fprintf(stderr, "Formatting LFS (fs %d bs %d)\n", fs_size, bs);
  int r = lfs_format(&s_lfs, &s_cfg);
  if (r != LFS_ERR_OK) return r;
  return lfs_mount(&s_lfs, &s_cfg);
}

int mem_lfs_mount_file(const char *fname, int bs) {
  FILE *in = fopen(fname, "r");
  if (in == NULL) {
    fprintf(stderr, "failed to open %s\n", fname);
    return -1;
  }
  fseek(in, 0, SEEK_END);
  int fs = ftell(in);
  fseek(in, 0, SEEK_SET);
  image = malloc(fs);
  int nr = fread(image, fs, 1, in);
  fclose(in);
  if (nr != 1) {
    fprintf(stderr, "Image %s exists but cannot be read\n", fname);
    return -1;
  }
  fprintf(stderr, "Mounting LFS from %s (fs %d bs %d)\n", fname, fs, bs);
  return mem_lfs_mount(fs, bs);
}

bool mem_lfs_dump(const char *fname) {
  FILE *out = stdout;
  if (fname != NULL) {
    out = fopen(fname, "w");
    if (out == NULL) {
      fprintf(stderr, "failed to open %s for writing\n", fname);
      return false;
    }
  }
  if (fwrite(image, s_lfs.cfg->block_count * s_lfs.cfg->block_size, 1, out) !=
      1) {
    fprintf(stderr, "write failed\n");
    return false;
  }
  if (out != stdout) fclose(out);
  return true;
}
