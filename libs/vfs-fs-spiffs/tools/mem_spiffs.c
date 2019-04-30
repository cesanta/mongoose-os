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

#include "mem_spiffs.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include <spiffs.h>

#define STRINGIFY(a) STRINGIFY2(a)
#define STRINGIFY2(a) #a

spiffs fs;
u8_t *spiffs_work_buf;
u8_t spiffs_fds[32 * 4];

char *image; /* in memory flash image */
bool log_reads = false, log_writes = false, log_erases = false;
int opr, opw, ope;
int wfail = -1;

uint32_t cs_crc32(uint32_t crc32, const void *data, uint32_t len);

s32_t mem_spiffs_read(spiffs *fs, u32_t addr, u32_t size, u8_t *dst) {
  if (log_reads) {
    fprintf(stderr, "R   #%04d %d @ %d\n", opr, (int) size, (int) addr);
  }
  memcpy(dst, image + addr, size);
  opr++;
  return SPIFFS_OK;
}

s32_t mem_spiffs_write(spiffs *fs, u32_t addr, u32_t size, u8_t *src) {
  if (log_writes) {
    fprintf(stderr, " W  #%04d %d @ %d\n", opw, (int) size, (int) addr);
  }
  if (opw == wfail) {
    fprintf(stderr, "=== BOOM!\n");
    mem_spiffs_dump("boom.bin");
    exit(0);
  }
  memcpy(image + addr, src, size);
  opw++;
  return SPIFFS_OK;
}

s32_t mem_spiffs_erase(spiffs *fs, u32_t addr, u32_t size) {
  if (log_erases) {
    fprintf(stderr, "  E #%04d %d @ %d\n", ope, (int) size, (int) addr);
  }
  memset(image + addr, 0xff, size);
  ope++;
  return SPIFFS_OK;
}

int mem_spiffs_mount(int fs_size, int bs, int ps, int es) {
  spiffs_config cfg;
  memset(&cfg, 0, sizeof(cfg));

  spiffs_work_buf = (u8_t *) calloc(2, ps);

  cfg.phys_size = fs_size;
  cfg.phys_addr = 0;

  cfg.phys_erase_block = es;
  cfg.log_block_size = bs;
  cfg.log_page_size = ps;

  cfg.hal_read_f = mem_spiffs_read;
  cfg.hal_write_f = mem_spiffs_write;
  cfg.hal_erase_f = mem_spiffs_erase;

  return SPIFFS_mount(&fs, &cfg, spiffs_work_buf, spiffs_fds,
                      sizeof(spiffs_fds), 0, 0, 0);
}

int mem_spiffs_mount_file(const char *fname, int bs, int ps, int es) {
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
  fprintf(stderr, "Mounting SPIFFS from %s (fs %d bs %d ps %d es %d)\n", fname,
          fs, bs, ps, es);
  return mem_spiffs_mount(fs, bs, ps, es);
}

bool mem_spiffs_dump(const char *fname) {
  FILE *out = stdout;
  if (fname != NULL) {
    out = fopen(fname, "w");
    if (out == NULL) {
      fprintf(stderr, "failed to open %s for writing\n", fname);
      return false;
    }
  }
  if (fwrite(image, fs.cfg.phys_size, 1, out) != 1) {
    fprintf(stderr, "write failed\n");
    return false;
  }
  if (out != stdout) fclose(out);
  return true;
}

u32_t list_files(void) {
  spiffs_DIR d;
  struct spiffs_dirent de;
  if (SPIFFS_opendir(&fs, ".", &d) == NULL) {
    fprintf(stderr, "opendir error: %d\n", SPIFFS_errno(&fs));
    return 0;
  }

  fprintf(stderr, "\n-- files:\n");
  uint32_t overall_size = 0, overall_crc32 = 0;
  while (SPIFFS_readdir(&d, &de) != NULL) {
    char *buf = NULL;
    spiffs_file in;

    in = SPIFFS_open_by_dirent(&fs, &de, SPIFFS_RDONLY, 0);
    if (in < 0) {
      fprintf(stderr, "cannot open spiffs file %s, err: %d\n", de.name,
              SPIFFS_errno(&fs));
      return 0;
    }

    buf = malloc(de.size);
    if (SPIFFS_read(&fs, in, buf, de.size) != de.size) {
      fprintf(stderr, "cannot read %s, err: %d\n", de.name, SPIFFS_errno(&fs));
      return 0;
    }
    uint32_t crc32 = cs_crc32(0, buf, de.size);
    char fmt[32];
    sprintf(fmt, "%%-%ds %%5d 0x%%08x", SPIFFS_OBJ_NAME_LEN);
    fprintf(stderr, fmt, de.name, (int) de.size, crc32);
#if SPIFFS_OBJ_META_LEN > 0
    fprintf(stderr, " meta:");
    for (int i = 0; i < SPIFFS_OBJ_META_LEN; i++) {
      fprintf(stderr, " %02x", de.meta[i]);
    }
#endif
    fprintf(stderr, "\n");
    free(buf);
    SPIFFS_close(&fs, in);
    overall_size += de.size;
    overall_crc32 ^= crc32;
  }

  SPIFFS_closedir(&d);
  fprintf(stderr, "-- overall size: %u, crc32: 0x%08x\n", overall_size,
          overall_crc32);
  return overall_crc32;
}

/*
 * Karl Malbrain's compact CRC-32.
 * See "A compact CCITT crc16 and crc32 C implementation that balances processor
 * cache usage against speed".
 */
uint32_t cs_crc32(uint32_t crc32, const void *data, uint32_t len) {
  static const uint32_t s_crc32[16] = {
      0,          0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4,
      0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
      0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c};
  const uint8_t *p = (const uint8_t *) data;
  crc32 = ~crc32;
  while (len--) {
    uint8_t b = *p++;
    crc32 = (crc32 >> 4) ^ s_crc32[(crc32 & 0xF) ^ (b & 0xF)];
    crc32 = (crc32 >> 4) ^ s_crc32[(crc32 & 0xF) ^ (b >> 4)];
  }
  return ~crc32;
}
