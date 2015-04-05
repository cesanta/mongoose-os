/*
 * Copyright (c) 2013-2015 Cesanta Software Limited
 * All rights reserved
 *
 * This library is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http://www.gnu.org/licenses/>.
 *
 * You are free to use this library under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this library under a commercial
 * license, as set out in <http://cesanta.com/products.html>.
 */

#include "fossa.h"
#include "v7.h"

#include <math.h>

#define FAIL(str, line)                           \
  do {                                            \
    printf("Fail on line %d: [%s]\n", line, str); \
    exit(1);                                      \
    return str;                                   \
  } while (0)

#define ASSERT(expr)                    \
  do {                                  \
    static_num_tests++;                 \
    if (!(expr)) FAIL(#expr, __LINE__); \
  } while (0)

#define RUN_TEST(test)                       \
  do {                                       \
    const char *msg = NULL;                  \
    if (strstr(#test, filter)) msg = test(); \
    if (msg) return msg;                     \
  } while (0)

static int static_num_tests = 0;

static char *read_file(const char *path, size_t *size) {
  FILE *fp;
  struct stat st;
  char *data = NULL;
  if ((fp = fopen(path, "rb")) != NULL && !fstat(fileno(fp), &st)) {
    *size = st.st_size;
    data = (char *) malloc(*size + 1);
    if (data != NULL) {
      if (fread(data, 1, *size, fp) < *size) {
        if (ferror(fp) == 0) return NULL;
      }
      data[*size] = '\0';
    }
    fclose(fp);
  }
  return data;
}

extern void init_smartjs(struct v7 *v7);

static struct v7 *new_v7(void) {
  struct v7 *v7 = v7_create();
  init_smartjs(v7);
  return v7;
}

static int check_value(struct v7 *v7, v7_val_t v, const char *str) {
  char buf[2048];
  v7_to_json(v7, v, buf, sizeof(buf));
  if (strncmp(buf, str, sizeof(buf)) != 0) {
    printf("want %s got %s\n", str, buf);
    return 0;
  }
  return 1;
}

static int check_num(v7_val_t v, double num) {
  int ret = isnan(num) ? isnan(v7_to_double(v)) : v7_to_double(v) == num;
  if (!ret) {
    printf("Num: want %f got %f\n", num, v7_to_double(v));
  }

  return ret;
}

static int check_num_not(v7_val_t v, double num) {
  int ret = isnan(num) ? isnan(v7_to_double(v)) : v7_to_double(v) != num;
  if (!ret) {
    printf("Num: want %f got %f\n", num, v7_to_double(v));
  }

  return ret;
}

static int check_str(struct v7 *v7, v7_val_t v, const char *str) {
  size_t n1, n2 = strlen(str);
  const char *s = v7_to_string(v7, &v, &n1);
  int ret = (n1 == n2 && memcmp(s, str, n1) == 0);
  if (!ret) {
    printf("Str: want %s got %s\n", str, s);
  }
  return ret;
}

static const char *test_file(void) {
  v7_val_t v;
  size_t file_len, string_len;
  char *file_data = read_file("test/unit_test.c", &file_len);
  const char *s;
  struct v7 *v7 = new_v7();
  char buf[100];

  /* Read file in C and Javascript, then compare respective strings */
  ASSERT(v7_exec(v7, &v, "var fd = OS.open('test/unit_test.c')") == V7_OK);
  ASSERT(v7_exec(v7, &v,
                 "var a = '', b; while ((b = OS.read(fd)) != '') "
                 "{ a += b; }; a") == V7_OK);
  s = v7_to_string(v7, &v, &string_len);
  ASSERT(string_len == file_len);
  ASSERT(memcmp(s, file_data, string_len) == 0);
  free(file_data);
  ASSERT(v7_exec(v7, &v, "OS.close(fd)") == V7_OK);
  ASSERT(check_value(v7, v, "0"));

  /* Create file, write into it, rename it, then remove it. */
  snprintf(buf, sizeof(buf), "fd = OS.open('foo.txt', %d, %d);",
           O_RDWR | O_CREAT, 0644);
  ASSERT(v7_exec(v7, &v, buf) == V7_OK);
  ASSERT(v7_exec(v7, &v, "OS.write(fd, 'hi there');") == V7_OK);
  ASSERT(check_value(v7, v, "0"));
  ASSERT(v7_exec(v7, &v, "OS.close(fd)") == V7_OK);
  ASSERT(check_value(v7, v, "0"));
  ASSERT((file_data = read_file("foo.txt", &file_len)) != NULL);
  ASSERT(file_len == 8);
  ASSERT(memcmp(file_data, "hi there", 8) == 0);
  free(file_data);
  ASSERT(v7_exec(v7, &v, "OS.rename('foo.txt', 'bar.txt')") == V7_OK);
  ASSERT(check_value(v7, v, "0"));
  ASSERT(v7_exec(v7, &v, "OS.remove('bar.txt')") == V7_OK);
  ASSERT(check_value(v7, v, "0"));
  ASSERT(fopen("foo.txt", "r") == NULL);
  ASSERT(fopen("bar.txt", "r") == NULL);

  v7_destroy(v7);
  return NULL;
}

static const char *test_crypto(void) {
  v7_val_t v;
  struct v7 *v7 = new_v7();

  ASSERT(v7_exec(v7, &v, "Crypto.md5('hello')") == V7_OK);
  ASSERT(check_str(
      v7, v,
      "\x5d\x41\x40\x2a\xbc\x4b\x2a\x76\xb9\x71\x9d\x91\x10\x17\xc5\x92"));
  ASSERT(v7_exec(v7, &v, "Crypto.md5_hex('hello')") == V7_OK);
  ASSERT(check_str(v7, v, "5d41402abc4b2a76b9719d911017c592"));
  ASSERT(v7_exec(v7, &v, "Crypto.sha1('hello')") == V7_OK);
  ASSERT(check_str(v7, v,
                   "\xaa\xf4\xc6\x1d\xdc\xc5\xe8\xa2\xda\xbe\xde\x0f\x3b\x48"
                   "\x2c\xd9\xae\xa9\x43\x4d"));
  ASSERT(v7_exec(v7, &v, "Crypto.sha1_hex('hello')") == V7_OK);
  ASSERT(check_str(v7, v, "aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d"));

  ASSERT(v7_exec(v7, &v, "Crypto.md5()") == V7_OK);
  ASSERT(check_value(v7, v, "null"));
  ASSERT(v7_exec(v7, &v, "Crypto.md5_hex()") == V7_OK);
  ASSERT(check_value(v7, v, "null"));
  ASSERT(v7_exec(v7, &v, "Crypto.sha1()") == V7_OK);
  ASSERT(check_value(v7, v, "null"));
  ASSERT(v7_exec(v7, &v, "Crypto.sha1_hex()") == V7_OK);
  ASSERT(check_value(v7, v, "null"));

  ASSERT(v7_exec(v7, &v, "Crypto.md5(123)") == V7_OK);
  ASSERT(check_value(v7, v, "null"));
  ASSERT(v7_exec(v7, &v, "Crypto.md5_hex([123])") == V7_OK);
  ASSERT(check_value(v7, v, "null"));
  ASSERT(v7_exec(v7, &v, "Crypto.sha1(null)") == V7_OK);
  ASSERT(check_value(v7, v, "null"));
  ASSERT(v7_exec(v7, &v, "Crypto.sha1_hex({})") == V7_OK);
  ASSERT(check_value(v7, v, "null"));

  v7_destroy(v7);
  return NULL;
}

#ifndef V7_DISABLE_SOCKETS
static const char *test_socket(void) {
  struct v7 *v7 = new_v7();
  ASSERT(v7_exec(v7, &v, "var d = new Socket()") == V7_OK);
  ASSERT(v7_exec(v7, &v, "d.close()") == V7_OK);

  ASSERT(v7_exec(v7, &v, "var d = Socket()") != V7_OK);

  ASSERT(v7_exec(v7, &v, "var d = new Socket(Socket.AF_INET)") == V7_OK);
  ASSERT(v7_exec(v7, &v, "d.close()") == V7_OK);

#ifdef V7_ENABLE_IPV6
  ASSERT(v7_exec(v7, &v, "var d = new Socket(Socket.AF_INET6)") == V7_OK);
  ASSERT(v7_exec(v7, &v, "d.close()") == V7_OK);
#endif

  ASSERT(v7_exec(v7, &v,
                 "var d = new Socket(Socket.AF_INET,"
                 "Socket.SOCK_STREAM)") == V7_OK);
  ASSERT(v7_exec(v7, &v, "d.close()") == V7_OK);

  ASSERT(v7_exec(v7, &v,
                 "var d = new Socket(Socket.AF_INET,"
                 "Socket.SOCK_DGRAM)") == V7_OK);
  ASSERT(v7_exec(v7, &v, "d.close()") == V7_OK);

  ASSERT(v7_exec(v7, &v,
                 "var d = new Socket(Socket.AF_INET,"
                 "Socket.SOCK_STREAM, Socket.RECV_STRING)") == V7_OK);
  ASSERT(v7_exec(v7, &v, "d.close()") == V7_OK);

  ASSERT(v7_exec(v7, &v,
                 "var d = new Socket(Socket.AF_INET,"
                 "Socket.SOCK_STREAM, Socket.RECV_RAW)") == V7_OK);
  ASSERT(v7_exec(v7, &v, "d.close()") == V7_OK);

  ASSERT(v7_exec(v7, &v, "var s = new Socket()") == V7_OK);
  ASSERT(v7_exec(v7, &v, "s.close()") == V7_OK);
  ASSERT(v7_exec(v7, &v, "var s = new Socket()") == V7_OK);
  ASSERT(v7_exec(v7, &v, "s.bind(60000)") == V7_OK);
  ASSERT(check_num(v, 0));
  ASSERT(v7_exec(v7, &v, "s.localPort") == V7_OK);
  ASSERT(check_num(v, 60000));
  ASSERT(v7_exec(v7, &v, "s.bind(60000)") == V7_OK);
  ASSERT(check_num_not(v, 0));
  ASSERT(v7_exec(v7, &v, "s.close()") == V7_OK);

  ASSERT(v7_exec(v7, &v, "var t = new Socket()") == V7_OK);
  ASSERT(v7_exec(v7, &v, "t.bind(\"non_number\")") == V7_OK);
  ASSERT(check_num_not(v, 0));
  ASSERT(v7_exec(v7, &v, "t.errno") == V7_OK);
  ASSERT(check_num(v, 22));
  ASSERT(v7_exec(v7, &v, "t.bind(12345.25)") == V7_OK);
  ASSERT(check_num(v, 0));
  ASSERT(v7_exec(v7, &v, "t.errno") == V7_OK);
  ASSERT(check_num(v, 0));
  ASSERT(v7_exec(v7, &v, "t.close()") == V7_OK);

  ASSERT(v7_exec(v7, &v, "var s = new Socket()") == V7_OK);
  ASSERT(v7_exec(v7, &v, "s.bind(12345)") == V7_OK);
  ASSERT(check_num(v, 0));
  ASSERT(v7_exec(v7, &v, "s.errno") == V7_OK);
  ASSERT(check_num(v, 0));
  ASSERT(v7_exec(v7, &v, "s.listen()") == V7_OK);
  ASSERT(check_num(v, 0));
  ASSERT(v7_exec(v7, &v, "s.errno") == V7_OK);
  ASSERT(check_num(v, 0));
  ASSERT(v7_exec(v7, &v, "s.close()") == V7_OK);

  ASSERT(v7_exec(v7, &v, "var s = new Socket()") == V7_OK);
  ASSERT(v7_exec(v7, &v, "s.bind(12345, \"127.0.0.1\")") == V7_OK);
  ASSERT(check_num(v, 0));
  ASSERT(v7_exec(v7, &v, "s.errno") == V7_OK);
  ASSERT(check_num(v, 0));
  ASSERT(v7_exec(v7, &v, "s.listen()") == V7_OK);
  ASSERT(check_num(v, 0));
  ASSERT(v7_exec(v7, &v, "s.errno") == V7_OK);
  ASSERT(check_num(v, 0));
  ASSERT(v7_exec(v7, &v, "s.close()") == V7_OK);

  ASSERT(v7_exec(v7, &v, "var s = new Socket()") == V7_OK);
  ASSERT(v7_exec(v7, &v, "s.bind(600000)") == V7_OK);
  ASSERT(check_num_not(v, 0));
  ASSERT(v7_exec(v7, &v, "s.errno") == V7_OK);
  ASSERT(check_num(v, 22));
  ASSERT(v7_exec(v7, &v, "s.close()") == V7_OK);
  v7_destroy(v7);
  return NULL;
}
#endif

static const char *run_all_tests(const char *filter) {
  RUN_TEST(test_file);
#ifndef V7_DISABLE_SOCKETS
  RUN_TEST(test_socket);
#endif
  RUN_TEST(test_crypto);
  return NULL;
}

int main(int argc, char *argv[]) {
  const char *filter = argc > 1 ? argv[1] : "";
  const char *fail_msg = run_all_tests(filter);
  printf("%s, tests run: %d\n", fail_msg ? "FAIL" : "PASS", static_num_tests);
  return fail_msg == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}
