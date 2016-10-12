#include "cs_frbuf.h"

#include <stdio.h>
#include <stdlib.h>

#include "cs_dbg.h"
#include "test_util.h"

#define TEST_FILE "cs_frbuf_test.dat"

void bin2hex(const char *src, int src_len, char *dst) {
  int i = 0;
  for (i = 0; i < src_len; i++) {
    sprintf(dst, "%02x", (int) *src);
    dst += 2;
    src += 1;
  }
}

/* Private definitions from cs_frbuf.c duplicated here for testing. */
#define MAGIC 0x3142 /* B1 */
struct cs_frbuf_file_hdr {
  uint16_t magic;
  uint16_t size, used;
  uint16_t head, tail;
};

#define ASSERT_FILE_EQ(expected_header, expected_data)                \
  do {                                                                \
    FILE *fp = fopen(TEST_FILE, "r");                                 \
    if (fp == NULL) FAIL("unable to open file", __LINE__);            \
    fseek(fp, 0, SEEK_END);                                           \
    long fsize = ftell(fp);                                           \
    fseek(fp, 0, SEEK_SET);                                           \
    char *buf = malloc(fsize);                                        \
    ASSERT_EQ(fread(buf, 1, fsize, fp), fsize);                       \
    struct cs_frbuf_file_hdr *h = (struct cs_frbuf_file_hdr *) buf;   \
    ASSERT_EQ(h->magic, MAGIC);                                       \
    char *hbuf;                                                       \
    asprintf(&hbuf, "s:%u u:%u h:%u t:%u", h->size, h->used, h->head, \
             h->tail);                                                \
    ASSERT_STREQ(hbuf, expected_header);                              \
    char *dbuf = calloc(fsize * 2 + 1, 1);                            \
    bin2hex(buf + sizeof(struct cs_frbuf_file_hdr),                   \
            fsize - sizeof(struct cs_frbuf_file_hdr), dbuf);          \
    ASSERT_STREQ(dbuf, expected_data);                                \
    free(hbuf);                                                       \
    free(dbuf);                                                       \
    free(buf);                                                        \
    fclose(fp);                                                       \
  } while (0)

#define ASSERT_FRBUF_GET(b, expected_data)  \
  do {                                      \
    char *data;                             \
    size_t len = cs_frbuf_get(b, &data);    \
    if (expected_data == NULL) {            \
      ASSERT_EQ(len, 0);                    \
    } else {                                \
      ASSERT_GT(len, 0);                    \
      ASSERT(data != NULL);                 \
      ASSERT_STREQ_NZ(data, expected_data); \
      free(data);                           \
    }                                       \
  } while (0)

static const char *test_frbuf_init_clean(void) {
  struct cs_frbuf *b = cs_frbuf_init(TEST_FILE, 100);
  ASSERT_FILE_EQ("s:90 u:0 h:0 t:0", "");
  cs_frbuf_deinit(b);
  ASSERT_FILE_EQ("s:90 u:0 h:0 t:0", "");
  return NULL;
}

static const char *test_frbuf_simple(void) {
  {
    struct cs_frbuf *b = cs_frbuf_init(TEST_FILE, 100);
    ASSERT_FILE_EQ("s:90 u:0 h:0 t:0", "");
    ASSERT(cs_frbuf_append(b, "AAAAA", 5));
    ASSERT_FILE_EQ("s:90 u:7 h:0 t:7", "05004141414141");
    cs_frbuf_deinit(b);
  }
  {
    struct cs_frbuf *b = cs_frbuf_init(TEST_FILE, 100);
    /* Previous state has been restored. */
    ASSERT_FILE_EQ("s:90 u:7 h:0 t:7", "05004141414141");
    ASSERT_FRBUF_GET(b, "AAAAA");
    /* Empty buffer is rewound to the beginning */
    ASSERT_FILE_EQ("s:90 u:0 h:0 t:0", "05004141414141");
    ASSERT(cs_frbuf_append(b, "BBB", 3));
    ASSERT_FILE_EQ("s:90 u:5 h:0 t:5", "03004242424141");
    ASSERT_FRBUF_GET(b, "BBB");
    ASSERT_FILE_EQ("s:90 u:0 h:0 t:0", "03004242424141");
    ASSERT_FRBUF_GET(b, NULL);
    cs_frbuf_deinit(b);
  }
  {
    struct cs_frbuf *b = cs_frbuf_init(TEST_FILE, 100);
    /* Empty buffer is truncated on next init. */
    ASSERT_FILE_EQ("s:90 u:0 h:0 t:0", "");
    cs_frbuf_deinit(b);
  }
  return NULL;
}

static const char *test_frbuf_wrap(void) {
  { /* Last record ends exactly at the end of the buffer */
    struct cs_frbuf *b = cs_frbuf_init(TEST_FILE, 22);
    ASSERT(cs_frbuf_append(b, "AAAAA", 5));
    ASSERT(cs_frbuf_append(b, "BBB", 3));
    ASSERT(cs_frbuf_append(b, "CC", 2)); /* AAAAA is discarded to make room. */
    ASSERT_FILE_EQ("s:12 u:9 h:7 t:4", "020043434141410300424242");
    ASSERT_FRBUF_GET(b, "BBB");
    ASSERT_FRBUF_GET(b, "CC");
    cs_frbuf_deinit(b);
  }
  remove(TEST_FILE);
  { /* Only one byte is available at the end, record header doesn't fit. */
    struct cs_frbuf *b = cs_frbuf_init(TEST_FILE, 22);
    ASSERT(cs_frbuf_append(b, "AAAAA", 5));
    ASSERT(cs_frbuf_append(b, "BB", 2));
    ASSERT(cs_frbuf_append(b, "CC", 2));
    ASSERT_FILE_EQ("s:12 u:8 h:7 t:4", "0200434341414102004242");
    cs_frbuf_deinit(b);
  }
  remove(TEST_FILE);
  { /* Header fits at the end, data is wrapped around. */
    struct cs_frbuf *b = cs_frbuf_init(TEST_FILE, 22);
    ASSERT(cs_frbuf_append(b, "AAAAA", 5));
    ASSERT(cs_frbuf_append(b, "B", 1));
    ASSERT(cs_frbuf_append(b, "CC", 2));
    ASSERT_FILE_EQ("s:12 u:7 h:7 t:2", "434341414141410100420200");
    ASSERT_FRBUF_GET(b, "B");
    ASSERT_FRBUF_GET(b, "CC");
    cs_frbuf_deinit(b);
  }
  remove(TEST_FILE);
  { /* Header and some data fit at the end, the rest is wrapped around. */
    struct cs_frbuf *b = cs_frbuf_init(TEST_FILE, 22);
    ASSERT(cs_frbuf_append(b, "AAAA", 4));
    ASSERT(cs_frbuf_append(b, "B", 1));
    ASSERT(cs_frbuf_append(b, "CC", 2));
    ASSERT_FILE_EQ("s:12 u:7 h:6 t:1", "430041414141010042020043");
    ASSERT_FRBUF_GET(b, "B");
    ASSERT_FRBUF_GET(b, "CC");
    cs_frbuf_deinit(b);
  }
  remove(TEST_FILE);
  return NULL;
}

static const char *run_tests(const char *filter, double *total_elapsed) {
  remove(TEST_FILE);
  RUN_TEST(test_frbuf_init_clean);
  remove(TEST_FILE);
  RUN_TEST(test_frbuf_simple);
  remove(TEST_FILE);
  RUN_TEST(test_frbuf_wrap);
  return NULL;
}

int main(int argc, char *argv[]) {
  const char *fail_msg;
  const char *filter = argc > 1 ? argv[1] : "";
  double total_elapsed = 0.0;

  cs_log_set_level(LL_DEBUG);

  fail_msg = run_tests(filter, &total_elapsed);
  printf("%s, run %d in %.3lfs\n", fail_msg ? "FAIL" : "PASS", num_tests,
         total_elapsed);
  return fail_msg == NULL ? 0 : 1;
}
