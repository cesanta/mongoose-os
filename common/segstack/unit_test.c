/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 * This library is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http: *www.gnu.org/licenses/>.
 *
 * You are free to use this library under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this library under a commercial
 * license, as set out in <http://cesanta.com/products.html>.
 */

/*
 * To unit test on Mac system, do
 *
 * g++ unit_test.c -o unit_test -W -Wall -g -O0 -fprofile-arcs -ftest-coverage
 * clang unit_test.c -o unit_test -W -Wall -g -O0 -fprofile-arcs -ftest-coverage
 * ./unit_test
 * gcov -a unit_test.c
 */

#include "segstack.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FAIL(str, line)                           \
  do {                                            \
    printf("Fail on line %d: [%s]\n", line, str); \
    return str;                                   \
  } while (0)

#define ASSERT(expr)                    \
  do {                                  \
    static_num_tests++;                 \
    if (!(expr)) FAIL(#expr, __LINE__); \
  } while (0)

/* VC6 doesn't know how to cast an unsigned 64-bit int to double */
#if (defined(_MSC_VER) && _MSC_VER <= 1200)
#define AS_DOUBLE(d) (double)(int64_t)(d)
#else
#define AS_DOUBLE(d) (double)(d)
#endif

/*
 * Numeric equality assertion. Comparison is made in native types but for
 * printing both are convetrted to double.
 */
#define ASSERT_EQ(actual, expected)                                 \
  do {                                                              \
    static_num_tests++;                                             \
    if (!((actual) == (expected))) {                                \
      printf("%f != %f\n", AS_DOUBLE(actual), AS_DOUBLE(expected)); \
      FAIL(#actual " == " #expected, __LINE__);                     \
    }                                                               \
  } while (0)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define RUN_TEST(test)        \
  do {                        \
    const char *msg = test(); \
    if (msg) return msg;      \
  } while (0)

static int static_num_tests = 0;

static int num_segs(struct segstack *ss) {
  int ret = 0;
  struct segstack_seg *bp = ss->last_seg;
  while (bp != NULL) {
    ret++;
    bp = bp->prev;
  }
  return ret;
}

static int64_t f(int i) {
  return i * 2 + 100;
}

static const char *test_pop(struct segstack *ss, int i, int seg_size,
                            int max_segs_num, int stash_size) {
  /* Check segs number */
  int expected_segs_num = (i / seg_size) + 1 + stash_size;
  if (expected_segs_num > max_segs_num) {
    expected_segs_num = max_segs_num;
  }
  ASSERT_EQ(num_segs(ss), expected_segs_num);

  /* Check TOS */
  int32_t val = *((int32_t *) segstack_tos(ss));
  ASSERT_EQ(val, f(i));

  /* Check popped value (should be the same as the TOS value above) */
  int32_t val2 = 0;
  segstack_pop(ss, &val2);
  ASSERT_EQ(val2, f(i));

  return NULL;
}

static const char *test_general(void) {
  uint8_t seg_size;
  uint8_t stash_size;

  /* Try various stash sizes... */
  for (stash_size = 0; stash_size <= 3; stash_size++) {
    /* And various seg sizes */
    for (seg_size = 1; seg_size <= 16; seg_size <<= 1) {
      struct segstack ss;
      struct segstack_opt opt = {
          .cell_size = 4, .seg_size = seg_size, .stash_segs = stash_size,
      };
      const int maxsize = 500;
      const int maxback = 50;
      const int dropsize = 123;
      int max_segs_num = 0;
      segstack_init(&ss, &opt);

      /* Fill the stack with values */
      int32_t i;
      for (i = 0; i <= maxsize; i++) {
        int32_t val = f(i);
        segstack_push(&ss, &val);
        ASSERT_EQ(num_segs(&ss), (i / seg_size + 1));
      }

      max_segs_num = num_segs(&ss);

      ASSERT(segstack_at(&ss, maxsize + 1) == NULL);
      ASSERT(segstack_at(&ss, -maxsize - 2) == NULL);

      /* Check each element of the stack */
      for (i = 0; i <= maxsize; i++) {
        ASSERT_EQ(*(int32_t *) segstack_at(&ss, i), f(i));
        ASSERT_EQ(num_segs(&ss), (maxsize / seg_size + 1));
      }

      /* Pop various amounts of cells from the stack, and push them back */
      int back;
      for (back = 1; back < maxback; back++) {
        for (i = maxsize; i >= (maxsize - back); i--) {
          const char *t = test_pop(&ss, i, seg_size, max_segs_num, stash_size);
          if (t != NULL) {
            return t;
          }
          int32_t val = 0xa5a5a5a5;
          segstack_push(&ss, &val);
          segstack_pop(&ss, NULL);
        }

        for (i = (maxsize - back); i <= maxsize; i++) {
          int32_t val = f(i);
          segstack_push(&ss, &val);
        }
      }

      /* Drop stack size by dropsize */
      segstack_set_size(&ss, maxsize - dropsize + 1);

      /* Pop all elements from the stack */
      for (i = maxsize - dropsize; i >= 0; i--) {
        const char *t = test_pop(&ss, i, seg_size, max_segs_num, stash_size);
        if (t != NULL) {
          return t;
        }
      }

      /* There should be no TOS, and len is 0 */
      ASSERT_EQ(segstack_size(&ss), 0);
      ASSERT(ss.tos_seg == NULL);

      /* In case of no stash, last_seg should also be NULL */
      if (stash_size == 0) {
        ASSERT(ss.last_seg == NULL);
      }

      segstack_free(&ss);
    }
  }
  return NULL;
}

static const char *run_all_tests(void) {
  RUN_TEST(test_general);
  return NULL;
}

int main(void) {
  const char *fail_msg = run_all_tests();
  printf("%s, tests run: %d\n", fail_msg ? "FAIL" : "PASS", static_num_tests);
  return fail_msg == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}
