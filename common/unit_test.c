/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#include "common/cs_varint.h"
#include "common/test_util.h"
#include "common/str_util.h"

int num_tests;

static const char *test_c_snprintf(void) {
  char buf[100];

  memset(buf, 'x', sizeof(buf));
  ASSERT_EQ(c_snprintf(buf, sizeof(buf), "boo"), 3);
  ASSERT_STREQ(buf, "boo");

  memset(buf, 'x', sizeof(buf));
  ASSERT_EQ(c_snprintf(buf, 0, "boo"), 3);
  ASSERT_EQ(buf[0], 'x');

  memset(buf, 'x', sizeof(buf));
  ASSERT_EQ(
      c_snprintf(buf, sizeof(buf), "/%s%c [%.*s]", "foo", 'q', 3, "123456789"),
      11);
  ASSERT_STREQ(buf, "/fooq [123]");

  memset(buf, 'x', sizeof(buf));
  ASSERT(c_snprintf(buf, sizeof(buf), "%d %x %04x %lu %ld", -2194, 0xabcdef01,
                    11, (unsigned long) 354523323, (long) 17) > 0);
  ASSERT_STREQ(buf, "-2194 abcdef01 000b 354523323 17");

  memset(buf, 'x', sizeof(buf));
  /*  ASSERT_EQ(c_snprintf(buf, sizeof(buf), "%*sfoo", 3, ""), 3); */
  c_snprintf(buf, sizeof(buf), "%*sfoo", 3, "");
  ASSERT_STREQ(buf, "   foo");

  return NULL;
}

static const char *test_cs_varint(void) {
  uint8_t buf[100];

  int llen_enc;
  int llen_dec;
  ASSERT_EQ((llen_enc = cs_varint_encode(127, buf)), 1);
  ASSERT_EQ(cs_varint_decode(buf, &llen_dec), 127);
  ASSERT_EQ(llen_dec, llen_enc);

  ASSERT_EQ((llen_enc = cs_varint_encode(128, buf)), 2);
  ASSERT_EQ(cs_varint_decode(buf, &llen_dec), 128);
  ASSERT_EQ(llen_dec, llen_enc);

  ASSERT_EQ((llen_enc = cs_varint_encode(0xfffffff, buf)), 4);
  ASSERT_EQ(cs_varint_decode(buf, &llen_dec), 0xfffffff);
  ASSERT_EQ(llen_dec, llen_enc);

  ASSERT_EQ((llen_enc = cs_varint_encode(0x7fffffff, buf)), 5);
  ASSERT_EQ(cs_varint_decode(buf, &llen_dec), 0x7fffffff);
  ASSERT_EQ(llen_dec, llen_enc);

  ASSERT_EQ((llen_enc = cs_varint_encode(0xffffffff, buf)), 5);
  ASSERT_EQ(cs_varint_decode(buf, &llen_dec), 0xffffffff);
  ASSERT_EQ(llen_dec, llen_enc);

  ASSERT_EQ((llen_enc = cs_varint_encode(0xfffffffff, buf)), 6);
  ASSERT_EQ(cs_varint_decode(buf, &llen_dec), 0xfffffffff);
  ASSERT_EQ(llen_dec, llen_enc);

  ASSERT_EQ((llen_enc = cs_varint_encode(0xfffffffffffffff, buf)), 9);
  ASSERT_EQ(cs_varint_decode(buf, &llen_dec), 0xfffffffffffffff);
  ASSERT_EQ(llen_dec, llen_enc);

  ASSERT_EQ((llen_enc = cs_varint_encode(0xffffffffffffffff, buf)), 10);
  ASSERT_EQ(cs_varint_decode(buf, &llen_dec), 0xffffffffffffffff);
  ASSERT_EQ(llen_dec, llen_enc);

  return NULL;
}

static const char *run_tests(const char *filter, double *total_elapsed) {
  RUN_TEST(test_c_snprintf);
  RUN_TEST(test_cs_varint);
  return NULL;
}

int main(int argc, char *argv[]) {
  const char *fail_msg;
  const char *filter = argc > 1 ? argv[1] : "";
  double total_elapsed = 0.0;

  fail_msg = run_tests(filter, &total_elapsed);
  printf("%s, tests run: %d\n", fail_msg ? "FAIL" : "PASS", num_tests);

  return fail_msg == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}
