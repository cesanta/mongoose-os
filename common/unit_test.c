/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

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

static const char *run_tests(const char *filter, double *total_elapsed) {
  RUN_TEST(test_c_snprintf);
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
