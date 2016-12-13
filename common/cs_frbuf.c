/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * File-backed ring buffer of fixed size. When full, will drop old records.
 * File structure looks like this:
 *
 * |file header|len|data|len|data|...
 *
 */

#include "common/cs_frbuf.h"
#include "common/cs_dbg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define MAGIC 0x3142 /* B1 */
#define FILE_HDR_SIZE sizeof(struct cs_frbuf_file_hdr)
#define REC_HDR_SIZE sizeof(struct cs_frbuf_rec_hdr)

struct cs_frbuf_file_hdr {
  uint16_t magic;
  uint16_t size, used;
  uint16_t head, tail;
};

struct cs_frbuf_rec_hdr {
  uint16_t len;
};

struct cs_frbuf {
  FILE *fp;
  struct cs_frbuf_file_hdr hdr;
};

static size_t cs_pread(struct cs_frbuf *b, size_t offset, size_t size,
                       void *buf) {
  fseek(b->fp, offset, SEEK_SET);
  return fread(buf, 1, size, b->fp);
}

static size_t cs_pwrite(struct cs_frbuf *b, size_t offset, size_t size,
                        const void *buf) {
  fseek(b->fp, offset, SEEK_SET);
  return fwrite(buf, 1, size, b->fp);
}

static size_t write_hdr(struct cs_frbuf *b) {
  if (b->hdr.used == 0) {
    b->hdr.head = b->hdr.tail = 0;
  }
  return cs_pwrite(b, 0, FILE_HDR_SIZE, &b->hdr);
}

struct cs_frbuf *cs_frbuf_init(const char *fname, uint16_t size) {
  struct cs_frbuf *b = malloc(sizeof(*b));
  if (b == NULL) return NULL;
  b->fp = fopen(fname, "r+");
  b->hdr.size = 0;
  if (b->fp != NULL) {
    fseek(b->fp, 0, SEEK_END);
    long fsize = ftell(b->fp);
    if (fsize >= (long) FILE_HDR_SIZE) {
      fseek(b->fp, 0, SEEK_SET);
      size_t nr = fread(&b->hdr, FILE_HDR_SIZE, 1, b->fp);
      if (nr != 1 || b->hdr.magic != MAGIC ||
          (fsize > (long) FILE_HDR_SIZE && b->hdr.used == 0)) {
        /* Truncate the empty or invalid buffer */
        b->hdr.size = 0;
        fclose(b->fp);
        b->fp = NULL;
      }
    }
  }
  if (b->hdr.size == 0) {
    if (b->fp == NULL) {
      b->fp = fopen(fname, "w+");
      if (b->fp == NULL) return false;
    }
    b->hdr.magic = MAGIC;
    b->hdr.size = size - FILE_HDR_SIZE;
    b->hdr.used = 0;
    b->hdr.head = b->hdr.tail = 0;
    if (write_hdr(b) != FILE_HDR_SIZE) {
      cs_frbuf_deinit(b);
      b = NULL;
    }
  }
  if (b != NULL) fflush(b->fp);
  return b;
}

void cs_frbuf_deinit(struct cs_frbuf *b) {
  if (b->fp != NULL) fclose(b->fp);
  memset(b, 0, sizeof(*b));
  free(b);
}

static size_t dpwrite(struct cs_frbuf *b, size_t offset, size_t size,
                      const void *buf) {
  /* If the region to be written overwrites current head record, throw away
   * until it doesn't. */
  while (b->hdr.used > 0 && offset <= b->hdr.head &&
         (offset + size > b->hdr.head)) {
    int len = cs_frbuf_get(b, NULL);
    if (len <= 0) return 0;
  }
  return cs_pwrite(b, offset + FILE_HDR_SIZE, size, buf);
}

bool cs_frbuf_append(struct cs_frbuf *b, const void *data, uint16_t len) {
  if (len == 0) return false;
  len = MIN(len, b->hdr.size - REC_HDR_SIZE);
  if (b->hdr.size - b->hdr.tail < (uint16_t) REC_HDR_SIZE) b->hdr.tail = 0;
  struct cs_frbuf_rec_hdr rhdr = {.len = len};
  if (dpwrite(b, b->hdr.tail, REC_HDR_SIZE, &rhdr) != REC_HDR_SIZE) {
    return false;
  }
  uint16_t to_write1 = MIN(len, b->hdr.size - b->hdr.tail - REC_HDR_SIZE);
  if (to_write1 > 0) {
    if (dpwrite(b, b->hdr.tail + REC_HDR_SIZE, to_write1, data) != to_write1) {
      return false;
    }
  }
  if (to_write1 < len) {
    uint16_t to_write2 = len - to_write1;
    if (dpwrite(b, 0, to_write2, ((char *) data) + to_write1) != to_write2) {
      return false;
    }
    b->hdr.tail = to_write2;
  } else {
    b->hdr.tail += (REC_HDR_SIZE + to_write1);
  }
  b->hdr.used += (REC_HDR_SIZE + len);
  if (write_hdr(b) != FILE_HDR_SIZE) return false;
  fflush(b->fp);
  return true;
}

static size_t dpread(struct cs_frbuf *b, size_t offset, size_t size,
                     void *buf) {
  return cs_pread(b, offset + FILE_HDR_SIZE, size, buf);
}

int cs_frbuf_get(struct cs_frbuf *b, char **data) {
  if (b->hdr.used == 0) return 0;
  if (b->hdr.size - b->hdr.head < (uint16_t) REC_HDR_SIZE) b->hdr.head = 0;
  struct cs_frbuf_rec_hdr rhdr;
  if (dpread(b, b->hdr.head, REC_HDR_SIZE, &rhdr) != REC_HDR_SIZE) {
    return -1;
  }
  if (data != NULL) {
    *data = malloc(rhdr.len);
    if (*data == NULL) return -2;
  }
  uint16_t to_read1 = MIN(rhdr.len, b->hdr.size - b->hdr.head - REC_HDR_SIZE);
  if (to_read1 > 0 && data != NULL) {
    if (dpread(b, b->hdr.head + REC_HDR_SIZE, to_read1, *data) != to_read1) {
      return -3;
    }
  }
  if (to_read1 < rhdr.len) {
    uint16_t to_read2 = rhdr.len - to_read1;
    if (data != NULL) {
      if (dpread(b, 0, to_read2, *data + to_read1) != to_read2) return -4;
    }
    b->hdr.head = to_read2;
  } else {
    b->hdr.head += (REC_HDR_SIZE + to_read1);
  }
  b->hdr.used -= (REC_HDR_SIZE + rhdr.len);
  if (write_hdr(b) != FILE_HDR_SIZE) return -5;
  fflush(b->fp);
  return rhdr.len;
}
