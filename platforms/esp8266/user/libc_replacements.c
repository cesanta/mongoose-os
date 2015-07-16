/*
* Copyright (c) 2015 Cesanta Software Limited
* All rights reserved
*/

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"

#include "v7.h"
#include "v7_esp.h"

/*
 * strerror provided by libc consumes 2kb RAM
 * Moreover, V7 uses strerror mostly for
 * file operation, so returns of strerror
 * are undefined, because spiffs uses its own
 * error codes and doesn't provide
 * error descriptions
 */
char* strerror(int errnum) {
  static char buf[15];
  snprintf(buf, sizeof(buf), "err: %d", errnum);
  buf[sizeof(buf) - 1] = 0;
  return buf;
}

void* malloc(size_t size) {
  void* res = (void*) pvPortMalloc(size);
  if (res == NULL) {
    v7_gc(v7, 1);
    res = (void*) pvPortMalloc(size);
  }
  return res;
}

void free(void* ptr) {
  vPortFree(ptr);
}

void* realloc(void* ptr, size_t size) {
  void* res = (void*) pvPortRealloc(ptr, size);
  if (res == NULL) {
    v7_gc(v7, 1);
    res = (void*) pvPortRealloc(ptr, size);
  }
  return res;
}

void* calloc(size_t num, size_t size) {
  void* res = (void*) pvPortZalloc(num * size);
  if (res == NULL) {
    v7_gc(v7, 1);
    res = (void*) pvPortZalloc(num * size);
  }
  return res;
}

int puts(const char* str) {
  return os_printf("%s", str);
}

int sprintf(char* buffer, const char* format, ...) {
  int ret;
  va_list arglist;
  va_start(arglist, format);
  ret = c_vsnprintf(buffer, ~0, format, arglist);
  va_end(arglist);
  return ret;
}

int snprintf(char* buffer, size_t size, const char* format, ...) {
  int ret;
  va_list arglist;
  va_start(arglist, format);
  ret = c_vsnprintf(buffer, size, format, arglist);
  va_end(arglist);
  return ret;
}

int vsnprintf(char* buffer, size_t size, const char* format, va_list arg) {
  return c_vsnprintf(buffer, size, format, arg);
}

int memcmp(const void* s1, const void* s2, size_t n) {
  return ets_memcmp(s1, s2, n);
}

int strcmp(const char* s1, const char* s2) {
  return ets_strcmp(s1, s2);
}

char* strcpy(char* dest, const char* src) {
  return (char*) ets_strcpy(dest, src);
}

int strncmp(const char* s1, const char* s2, size_t len) {
  return ets_strncmp(s1, s2, len);
}

char* strncpy(char* dest, const char* src, size_t n) {
  return (char*) ets_strncpy(dest, src, n);
}

size_t strnlen(const char* s, size_t len) {
  // there is no ets_strnlen
  const char* cp;
  for (cp = s; len != 0 && *cp != '\0'; cp++, len--)
    ;
  return (size_t)(cp - s);
}

char* strstr(const char* haystack, const char* needle) {
  return (char*) ets_strstr(haystack, needle);
}

char* strchr(const char* str, int character) {
  while (1) {
    if (*str == 0x00) {
      return NULL;
    }
    if (*str == (char) character) {
      return (char*) str;
    }
    str++;
  }
}

char* strrchr(const char* str, int character) {
  char* ret = NULL;
  while (1) {
    if (*str == 0x00) {
      return ret;
    }
    if (*str == (char) character) {
      ret = (char*) str;
    }
    str++;
  }
}

char* strcat(char* dest, const char* src) {
  return strncat(dest, src, strlen(src));
}

char* strncat(char* dest, const char* src, size_t n) {
  uint32_t offset = strlen(dest);
  uint32_t i;
  for (i = 0; i < n; i++) {
    *(dest + i + offset) = *(src + i);
    if (*(src + i) == 0x00) {
      break;
    }
  }
  return dest;
}

char* strtok_r(char* str, const char* delimiters, char** temp) {
  static char* ret = NULL;
  char* start = NULL;
  char* end = NULL;
  uint32_t size = 0;

  if (str == NULL) {
    start = *temp;
  } else {
    start = str;
  }

  if (start == NULL) {
    return NULL;
  }

  end = start;

  while (1) {
    uint16_t i;
    for (i = 0; i < strlen(delimiters); i++) {
      if (*end == *(delimiters + i)) {
        break;
      }
    }
    end++;
    if (*end == 0x00) {
      break;
    }
  }

  *temp = end;

  if (ret != NULL) {
    free(ret);
  }

  size = (end - start);
  ret = (char*) malloc(size);
  strncpy(ret, start, size);
  return ret;
}

char* strtok(char* str, const char* delimiters) {
  return strtok_r(str, delimiters, NULL);
}

int strcasecmp(const char* str1, const char* str2) {
  int d = 0;
  while (1) {
    int c1 = tolower(*str1++);
    int c2 = tolower(*str2++);
    if (((d = c1 - c2) != 0) || (c2 == '\0')) {
      break;
    }
  }
  return d;
}

char* strdup(const char* str) {
  size_t len = strlen(str) + 1;
  char* cstr = malloc(len);
  if (cstr) {
    memcpy(cstr, str, len);
  }
  return cstr;
}

// based on Source:
// https://github.com/anakod/Sming/blob/master/Sming/system/stringconversion.cpp#L93
double strtod(const char* str, char** endptr) {
  double result = 0.0;
  double factor = 1.0;
  bool decimals = false;
  char c;

  while (isspace(*str)) {
    str++;
  }

  if (*str == 0x00) {
    // only space in str?
    if (endptr != NULL) {
      *endptr = (char*) str;
    }
    return result;
  }

  if (*str == '-') {
    factor = -1;
    str++;
  } else if (*str == '+') {
    str++;
  } else if (*str == '0') {
    str++;
    if (*str == 'x') { /* base 16 */
      str++;
      while ((c = tolower(*str))) {
        int d;
        if (c >= '0' && c <= '9') {
          d = c - '0';
        } else if (c >= 'a' && c <= 'f') {
          d = 10 + (c - 'a');
        } else {
          break;
        }
        result = 16 * result + d;
        str++;
      }
    } else if (*str == 'b') { /* base 2 */
      str++;
      while ((c = *str)) {
        int d = c - '0';
        if (c != '0' && c != '1') break;
        result = 2 * result + d;
        str++;
      }
    } else { /* base 8 */
      while ((c = *str)) {
        int d = c - '0';
        if (c < '0' || c > '7') break;
        result = 8 * result + d;
        str++;
      }
    }
  } else { /* base 10 */
    while ((c = *str)) {
      if (c == '.') {
        decimals = true;
        str++;
        continue;
      }

      int d = c - '0';
      if (d < 0 || d > 9) {
        break;
      }

      result = 10 * result + d;
      if (decimals) {
        factor *= 0.1;
      }

      str++;
    }
  }
  if (endptr != NULL) {
    *endptr = (char*) str;
  }
  return result * factor;
}

// ##########################################################################
//                             ctype functions
// ##########################################################################

int isalnum(int c) {
  if (isalpha(c) || isdigit(c)) {
    return 1;
  }
  return 0;
}

int isalpha(int c) {
  if (islower(c) || isupper(c)) {
    return 1;
  }
  return 0;
}

int iscntrl(int c) {
  if (c <= 0x1F || c == 0x7F) {
    return 1;
  }
  return 0;
}

int isdigit(int c) {
  if (c >= '0' && c <= '9') {
    return 1;
  }
  return 0;
}

int isgraph(int c) {
  if (isprint(c) && c != ' ') {
    return 1;
  }
  return 0;
}

int islower(int c) {
  if (c >= 'a' && c <= 'z') {
    return 1;
  }
  return 0;
}

int isprint(int c) {
  if (!iscntrl(c)) {
    return 1;
  }
  return 0;
}

int ispunct(int c) {
  if (isgraph(c) && !isalnum(c)) {
    return 1;
  }
  return 0;
}

int isspace(int c) {
  switch (c) {
    case 0x20:  // ' '
    case 0x09:  // '\t'
    case 0x0a:  // '\n'
    case 0x0b:  // '\v'
    case 0x0c:  // '\f'
    case 0x0d:  // '\r'
      return 1;
  }
  return 0;
}

int isupper(int c) {
  if (c >= 'A' && c <= 'Z') {
    return 1;
  }
  return 0;
}

int isxdigit(int c) {
  if (c >= 'A' && c <= 'F') {
    return 1;
  }
  if (c >= 'a' && c <= 'f') {
    return 1;
  }
  if (isdigit(c)) {
    return 1;
  }
  return 0;
}

int tolower(int c) {
  if (isupper(c)) {
    c += 0x20;
  }
  return c;
}

int toupper(int c) {
  if (islower(c)) {
    c -= 0x20;
  }
  return c;
}

int isblank(int c) {
  switch (c) {
    case 0x20:  // ' '
    case 0x09:  // '\t'
      return 1;
  }
  return 0;
}

// ##########################################################################

static int errno_var = 0;

int* __errno(void) {
  return &errno_var;
}

/*
 * Reinventing pow to avoid usage of native pow
 * becouse pow goes to iram0 segment
 */
static double flash_pow10int(int n) {
  if (n == 0) {
    return 1;
  } else if (n == 1) {
    return 10;
  } else if (n > 0) {
    return round(exp(n * log(10)));
  } else {
    return 1 / round(exp(-n * log(10)));
  }
}

/*
 * Using ln to get lg in order
 * to avoid usage of native log10
 * since it goes to iram segment
 */

static double flash_log10(double x) {
  return log(x) / log(10);
}

/*
 * Attempt to reproduce sprintf's %g
 */
int double_to_str(char* buf, double val, int prec) {
  if (isnan(val)) {
    strcpy(buf, "nan");
    return 3;
  } else if (isinf(val)) {
    strcpy(buf, "inf");
    return 3;
  } else if (val == 0) {
    strcpy(buf, "0");
    return 1;
  }
  /*
   * It is possible to use itol, in case of integer
   * could be kinda optimization
   */
  double precision = flash_pow10int(-prec);

  int mag1, mag2;
  char* ptr = buf;
  int neg = (val < 0);

  if (neg != 0) {
    /* no fabs() */
    val = -val;
  }

  mag1 = flash_log10(val);

  int use_e =
      (mag1 >= prec || (neg && mag1 >= prec - 3) || mag1 <= -(prec - 3));

  if (neg) {
    *ptr = '-';
    ptr++;
  }

  if (use_e) {
    if (mag1 < 0) {
      mag1 -= 1.0;
    }

    val = val / flash_pow10int(mag1);
    mag2 = mag1;
    mag1 = 0;
  }

  if (mag1 < 1.0) {
    mag1 = 0;
  }

  while (val > precision || mag1 >= 0) {
    double pos = flash_pow10int(mag1);

    if (pos > 0 && !isinf(pos)) {
      int num = floor(val / pos);
      val -= (num * pos);
      *ptr = '0' + num;
      ptr++;
    }
    if (mag1 == 0 && val > 0) {
      *ptr = '.';
      ptr++;
    }
    mag1--;
  }
  if (use_e != 0) {
    int i, j;
    *ptr = 'e';
    ptr++;
    if (mag2 > 0) {
      *ptr = '+';
    } else {
      *ptr = '-';
      mag2 = -mag2;
    }
    ptr++;
    mag1 = 0;
    while (mag2 > 0) {
      *ptr = '0' + mag2 % 10;
      ptr++;
      mag2 /= 10;
      mag1++;
    }
    ptr -= mag1;
    for (i = 0, j = mag1 - 1; i < j; i++, j--) {
      double tmp = ptr[i];
      ptr[i] = ptr[j];
      ptr[j] = tmp;
    }
    ptr += mag1;
  }
  *ptr = '\0';

  return ptr - buf;
}

void abort(void) {
  /* cause an unaligned access exception, that will drop you into gdb */
  *(int*) 1 = 1;
  while (1)
    ; /* avoid gcc warning because stdlib abort() has noreturn attribute */
}
