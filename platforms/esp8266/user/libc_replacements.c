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
#include <sys/stat.h>
#include <sys/time.h>

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
 * Returns _required_ number of symbols
 */

#define APPEND_CHAR(ch)              \
  {                                  \
    if (count < buf_size) *ptr = ch; \
    count++;                         \
  }

int double_to_str(char* buf, size_t buf_size, double val, int prec) {
  if (isnan(val)) {
    strncpy(buf, "nan", buf_size);
    return 3;
  } else if (isinf(val)) {
    strncpy(buf, "inf", buf_size);
    return 3;
  } else if (val == 0) {
    strncpy(buf, "0", buf_size);
    return 1;
  }
  /*
   * It is possible to use itol, in case of integer
   * could be kinda optimization
   */
  double precision = flash_pow10int(-prec);

  int mag1, mag2, count = 0;
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
    APPEND_CHAR('-');
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
      APPEND_CHAR(('0' + num))
      ptr++;
    }
    if (mag1 == 0 && val > 0) {
      APPEND_CHAR('.')
      ptr++;
    }
    mag1--;
  }
  if (use_e != 0) {
    int i, j;
    APPEND_CHAR('e');
    ptr++;
    if (mag2 > 0) {
      APPEND_CHAR('+')
    } else {
      APPEND_CHAR('-');
      mag2 = -mag2;
    }
    ptr++;
    mag1 = 0;
    while (mag2 > 0) {
      APPEND_CHAR(('0' + mag2 % 10))
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
  APPEND_CHAR('\0');

  return count - 1;
}

#undef APPEND_CHAR

void abort(void) {
  /* cause an unaligned access exception, that will drop you into gdb */
  *(int*) 1 = 1;
  while (1)
    ; /* avoid gcc warning because stdlib abort() has noreturn attribute */
}

uint32_t htonl(uint32_t hostlong) {
  return ((hostlong & 0xff000000) >> 24) | ((hostlong & 0x00ff0000) >> 8) |
         ((hostlong & 0x0000ff00) << 8) | ((hostlong & 0x000000ff) << 24);
}

uint16_t htons(uint16_t hostshort) {
  return ((hostshort & 0xff00) >> 8) | ((hostshort & 0x00ff) << 8);
}

uint16_t ntohs(uint16_t netshort) {
  return htons(netshort);
}

uint32_t ntohl(uint32_t netlong) {
  return htonl(netlong);
}

/* These are all dummy stubs. Some Newlib functions link them in for
 * stdin/stdout, but as far as I can tell they're not used by us. */
int _close_r(struct _reent* r, int fd) {
  return 0;
}

int _open_r(struct _reent* r, const char* n, int f, int m) {
  return -1;
}

int _fstat_r(struct _reent* r, int fd, struct stat* s) {
  return -1;
}

_ssize_t _read_r(struct _reent* r, int fd, void* buf, size_t len) {
  return -1;
}

_ssize_t _write_r(struct _reent* r, int fd, void* buf, size_t len) {
  return -1;
}

void* _sbrk_r(struct _reent* r, ptrdiff_t p) {
  return NULL;
}

_off_t _lseek_r(struct _reent* r, int fd, _off_t where, int how) {
  return 0;
}

int _gettimeofday_r(struct _reent* r, struct timeval* tp, void* tzp) {
  tp->tv_sec = 42;
  tp->tv_usec = 123;
  return 0;
}
