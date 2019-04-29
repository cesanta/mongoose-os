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

#include <string.h>

#include "common/cs_time.h"
#include "common/cs_varint.h"
#include "common/mg_str.h"
#include "common/str_util.h"
#include "common/test_main.h"
#include "common/test_util.h"

static const char *check_assert_ptrne(void) {
  int a = 0;
  ASSERT_PTRNE(&a, &a);
  return NULL;
}

static const char *check_assert_streq(void) {
  ASSERT_STREQ("foo", "bar");
  return NULL;
}

static const char *check_assert_eq(void) {
  ASSERT_EQ(1, 2);
  return NULL;
}
static const char *check_assert_eq_precision(void) {
  ASSERT_EQ((2ULL << 51) + 1, (2ULL << 51) + 1);
  return NULL;
}
static const char *check_assert_eq64(void) {
  ASSERT_EQ64(0xffffffffffffffff, 0xfffffffffffffffe);
  return NULL;
}

static const char *check_assert_ne(void) {
  ASSERT_NE(1, 1);
  return NULL;
}
static const char *check_assert_ne_precision(void) {
  ASSERT_NE((2ULL << 51) + 1, (2ULL << 51) + 2);
  return NULL;
}
static const char *check_assert_ne64(void) {
  ASSERT_NE64(0xffffffffffffffff, 0xffffffffffffffff);
  return NULL;
}

static const char *test_testutil(void) {
  if (check_assert_ptrne() == NULL) return "ASSERT_PTRNE does not work";

  ASSERT_STREQ("foo", "foo");
  ASSERT_PTRNE(check_assert_streq(), NULL);

  ASSERT_EQ(0, 0);
  ASSERT_EQ(-1, -1);
  ASSERT_EQ((long) -1, (long) -1);

  ASSERT_EQ(1, 1);
  ASSERT_EQ((2ULL << 51), (2ULL << 51));
  ASSERT_PTRNE(check_assert_eq(), NULL);
  ASSERT_STREQ(check_assert_eq_precision(),
               "loss of precision, use ASSERT_EQ64");
  ASSERT_EQ64(0xffffffffffffffff, 0xffffffffffffffff);
  ASSERT_PTRNE(check_assert_eq64(), NULL);

  ASSERT_NE((2ULL << 51) - 1, (2ULL << 51) - 2);
  ASSERT_PTRNE(check_assert_ne(), NULL);
  ASSERT_STREQ(check_assert_ne_precision(),
               "loss of precision, use ASSERT_NE64");
  ASSERT_NE64(0xffffffffffffffff, 0xfffffffffffffffe);
  ASSERT_PTRNE(check_assert_ne64(), NULL);
  printf("^^^ failure messages above are normal, ignore ^^^\n");
  return NULL;
}

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
  uint8_t buf[100] = {0};
  size_t llen_enc, llen_dec;
  uint64_t num;

  memset(buf, 'X', sizeof(buf));
  ASSERT_EQ((llen_enc = cs_varint_encode(127, buf, 0)), 1);
  ASSERT_EQ(buf[0], 'X');
  ASSERT_EQ((llen_enc = cs_varint_encode(127, buf, sizeof(buf))), 1);
  ASSERT_EQ(llen_enc, 1);
  ASSERT_EQ(buf[1], 'X');
  ASSERT(!cs_varint_decode(buf, 0, &num, &llen_dec));
  ASSERT(cs_varint_decode(buf, 1, &num, &llen_dec));
  ASSERT_EQ(num, 127);
  ASSERT_EQ(llen_dec, llen_enc);

  memset(buf, 'X', sizeof(buf));
  ASSERT_EQ((llen_enc = cs_varint_encode(128, buf, 1)), 2);
  ASSERT_EQ(buf[1], 'X');
  ASSERT_EQ((llen_enc = cs_varint_encode(128, buf, sizeof(buf))), 2);
  ASSERT(!cs_varint_decode(buf, 0, &num, &llen_dec));
  num = 123;
  llen_dec = 456;
  ASSERT(!cs_varint_decode(buf, 1, &num, &llen_dec));
  ASSERT_EQ(num, 123);
  ASSERT_EQ(llen_dec, 456);
  ASSERT(cs_varint_decode(buf, sizeof(buf), &num, &llen_dec));
  ASSERT_EQ(num, 128);
  ASSERT_EQ(llen_dec, llen_enc);

  ASSERT_EQ((llen_enc = cs_varint_encode(0xfffffff, buf, sizeof(buf))), 4);
  ASSERT(cs_varint_decode(buf, sizeof(buf), &num, &llen_dec));
  ASSERT_EQ(num, 0xfffffff);
  ASSERT_EQ(llen_dec, llen_enc);

  ASSERT_EQ((llen_enc = cs_varint_encode(0x7fffffff, buf, sizeof(buf))), 5);
  ASSERT(cs_varint_decode(buf, sizeof(buf), &num, &llen_dec));
  ASSERT_EQ(num, 0x7fffffff);
  ASSERT_EQ(llen_dec, llen_enc);

  ASSERT_EQ((llen_enc = cs_varint_encode(0xffffffff, buf, sizeof(buf))), 5);
  ASSERT(cs_varint_decode(buf, sizeof(buf), &num, &llen_dec));
  ASSERT_EQ(num, 0xffffffff);
  ASSERT_EQ(llen_dec, llen_enc);

  ASSERT_EQ((llen_enc = cs_varint_encode(0xfffffffff, buf, sizeof(buf))), 6);
  ASSERT(cs_varint_decode(buf, sizeof(buf), &num, &llen_dec));
  ASSERT_EQ(num, 0xfffffffff);
  ASSERT_EQ(llen_dec, llen_enc);

  ASSERT_EQ((llen_enc = cs_varint_encode(0xfffffffffffffff, buf, sizeof(buf))),
            9);
  ASSERT(cs_varint_decode(buf, sizeof(buf), &num, &llen_dec));
  ASSERT_EQ64(num, 0xfffffffffffffff);
  ASSERT_EQ(llen_dec, llen_enc);

  ASSERT_EQ((llen_enc = cs_varint_encode(0xffffffffffffffff, buf, sizeof(buf))),
            10);
  ASSERT(cs_varint_decode(buf, sizeof(buf), &num, &llen_dec));
  ASSERT_EQ64(num, 0xffffffffffffffff);
  ASSERT_EQ(llen_dec, llen_enc);

  /* Overflow */
  memset(buf, 0xff, sizeof(buf));
  num = 0;
  ASSERT(cs_varint_decode(buf, sizeof(buf), &num, &llen_dec));
  ASSERT_EQ64(num, 0xffffffffffffffff);
  ASSERT_EQ(llen_dec, 10);

  return NULL;
}

static const char *test_cs_timegm(void) {
  struct tm t;
  time_t now = time(NULL);
  gmtime_r(&now, &t);
  ASSERT_EQ((double) timegm(&t), cs_timegm(&t));

  return NULL;
}

static const char *test_mg_match_prefix(void) {
  const struct mg_str null = MG_NULL_STR;
  ASSERT_EQ(mg_match_prefix_n(null, mg_mk_str("")), 0);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str(""), null), 0);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("x"), null), 0);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str(""), mg_mk_str("")), 0);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("|"), mg_mk_str("")), 0);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("xy|"), mg_mk_str("")), 0);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("xy|"), mg_mk_str("xy")), 2);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("*"), mg_mk_str("")), 0);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a"), mg_mk_str("a")), 1);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a"), mg_mk_str("xyz")), 0);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("abc"), mg_mk_str("abc")), 3);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("abc"), mg_mk_str("abcdef")), 3);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("abc*"), mg_mk_str("abcdef")), 6);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a*f"), mg_mk_str("abcdef")), 6);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a*f"), mg_mk_str("abcdefZZ")), 6);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a*f"), mg_mk_str("abcdeZZ")), 0);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a*f|de*"), mg_mk_str("abcdef")), 6);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a*f|de*|xy"), mg_mk_str("defgh")), 5);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a*f,de*"), mg_mk_str("abcdef")), 6);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a*f|de*,xy"), mg_mk_str("defgh")), 5);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a*f,de*,xy"), mg_mk_str("defgh")), 5);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("/a/b/*/d"), mg_mk_str("/a/b/c/d")), 8);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("?"), mg_mk_str("a")), 1);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("?"), mg_mk_str("abc")), 1);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("?*"), mg_mk_str("abc")), 3);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("?*"), mg_mk_str("abcdef")), 6);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("?*"), mg_mk_str("")), 0);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a?c"), mg_mk_str("abc")), 3);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a?c"), mg_mk_str("adc")), 3);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a?c"), mg_mk_str("ab")), 0);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a?c"), mg_mk_str("a")), 0);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a$"), mg_mk_str("a")), 1);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("*a$"), mg_mk_str("a")), 1);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("*b$"), mg_mk_str("ab")), 2);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("**a$|**b$"), mg_mk_str("xa")), 2);
  ASSERT_EQ(mg_match_prefix_n(mg_mk_str("a.*$"), mg_mk_str("a.txt")), 5);
  return NULL;
}

static const char *test_mg_mk_str(void) {
  const char *foo = "foo";
  struct mg_str s0 = MG_NULL_STR;
  ASSERT_EQ(s0.len, 0);
  ASSERT(s0.p == NULL);
  struct mg_str s0a = mg_mk_str(NULL);
  ASSERT_EQ(s0a.len, 0);
  ASSERT(s0a.p == NULL);
  struct mg_str s1 = MG_MK_STR("foo");
  ASSERT_EQ(s1.len, 3);
  ASSERT(s1.p == foo);
  struct mg_str s2 = mg_mk_str("foo");
  ASSERT_EQ(s2.len, 3);
  ASSERT(s2.p == foo);
  struct mg_str s3 = mg_mk_str_n("foo", 2);
  ASSERT_EQ(s3.len, 2);
  ASSERT(s3.p == foo);
  struct mg_str s4 = mg_mk_str_n("foo", 0);
  ASSERT_EQ(s4.len, 0);
  ASSERT(s4.p == foo);
  return NULL;
}

static const char *test_mg_strdup(void) {
  struct mg_str s0 = MG_NULL_STR;
  struct mg_str s1 = MG_MK_STR("foo");

  struct mg_str s2 = mg_strdup(s1);
  ASSERT_EQ(s1.len, s2.len);
  ASSERT(s1.p != s2.p);
  ASSERT_EQ(memcmp(s2.p, s1.p, s1.len), 0);

  struct mg_str s3 = mg_strdup(s0);
  ASSERT_EQ(s3.len, 0);
  ASSERT(s3.p == NULL);

  struct mg_str s4 = mg_mk_str_n("foo", 0);
  struct mg_str s5 = mg_strdup(s4);
  ASSERT_EQ(s5.len, 0);
  ASSERT(s5.p == NULL);

  free((void *) s2.p);
  return NULL;
}

static const char *test_mg_strchr(void) {
  struct mg_str s0 = MG_NULL_STR;
  struct mg_str s1 = mg_mk_str_n("foox", 3);
  struct mg_str s2 = mg_mk_str_n("foox\0yz", 7);

  ASSERT(mg_strchr(s0, 'f') == NULL);

  ASSERT(mg_strchr(s1, 'f') == s1.p);
  ASSERT(mg_strchr(s1, 'o') == s1.p + 1);
  ASSERT(mg_strchr(s1, 'x') == NULL);

  ASSERT(mg_strchr(s2, 'y') == s2.p + 5);
  ASSERT(mg_strchr(s2, 'z') == s2.p + 6);

  return NULL;
}

static const char *test_mg_strstr(void) {
  struct mg_str s0 = MG_NULL_STR;
  struct mg_str s1 = MG_MK_STR("foobario");
  struct mg_str s2 = s1;
  s2.len -= 2;

  ASSERT(mg_strstr(s0, s0) == NULL);
  ASSERT(mg_strstr(s1, s0) == s1.p);
  ASSERT(mg_strstr(s0, s1) == NULL);
  ASSERT(mg_strstr(s1, s1) == s1.p);
  ASSERT(mg_strstr(s1, mg_mk_str("foo")) == s1.p);
  ASSERT(mg_strstr(s1, mg_mk_str("oo")) == s1.p + 1);
  ASSERT(mg_strstr(s1, mg_mk_str("bar")) == s1.p + 3);
  ASSERT(mg_strstr(s1, mg_mk_str("bario")) == s1.p + 3);
  ASSERT(mg_strstr(s2, mg_mk_str("bar")) == s1.p + 3);
  ASSERT(mg_strstr(s2, mg_mk_str("bario")) == NULL);

  return NULL;
}

static const char *test_mg_strstrip(void) {
  struct mg_str s0 = MG_NULL_STR;

  ASSERT_EQ(mg_strstrip(s0).len, 0);
  ASSERT_MG_STREQ(mg_strstrip(mg_mk_str("foo")), "foo");
  ASSERT_MG_STREQ(mg_strstrip(mg_mk_str(" foo")), "foo");
  ASSERT_MG_STREQ(mg_strstrip(mg_mk_str("foo ")), "foo");
  ASSERT_MG_STREQ(mg_strstrip(mg_mk_str(" foo ")), "foo");
  ASSERT_MG_STREQ(mg_strstrip(mg_mk_str(" \t\r\n foo \r\n\t ")), "foo");
  ASSERT_MG_STREQ(mg_strstrip(mg_mk_str(" \t\r\n foo \r\n\t bar")),
                  "foo \r\n\t bar");

  return NULL;
}

static const char *test_mg_str_starts_with(void) {
  struct mg_str s0 = MG_NULL_STR;

  ASSERT(mg_str_starts_with(s0, s0));
  ASSERT(!mg_str_starts_with(s0, mg_mk_str("foo")));
  ASSERT(mg_str_starts_with(mg_mk_str("foo"), s0));
  ASSERT(mg_str_starts_with(mg_mk_str("foobar"), mg_mk_str("foo")));
  ASSERT(!mg_str_starts_with(mg_mk_str("foobar"), mg_mk_str("bar")));
  ASSERT(!mg_str_starts_with(mg_mk_str("foo"), mg_mk_str("foobar")));

  return NULL;
}

void tests_setup(void) {
}

const char *tests_run(const char *filter) {
  RUN_TEST(test_testutil);
  RUN_TEST(test_c_snprintf);
  RUN_TEST(test_cs_varint);
  RUN_TEST(test_cs_timegm);
  RUN_TEST(test_mg_match_prefix);
  RUN_TEST(test_mg_mk_str);
  RUN_TEST(test_mg_strdup);
  RUN_TEST(test_mg_strchr);
  RUN_TEST(test_mg_strstr);
  RUN_TEST(test_mg_strstrip);
  RUN_TEST(test_mg_str_starts_with);
  return NULL;
}

void tests_teardown(void) {
}
