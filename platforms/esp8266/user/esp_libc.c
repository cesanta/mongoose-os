/*
* Copyright (c) 2015 Cesanta Software Limited
* All rights reserved
*/

#include <ctype.h>
#include <sys/time.h>
#include <stdint.h>
#include "esp_missing_includes.h"

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef RTOS_SDK

#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <ets_sys.h>
#include <os_type.h>
#include <osapi.h>
#include <mem.h>
#include <user_interface.h>
#include <errno.h>

#else

#include <espressif/esp_system.h>

#endif /* RTOS_SDK */

/* #define ESP_ABORT_ON_MALLOC_FAILURE */

int c_vsnprintf(char *buf, size_t buf_size, const char *format, va_list ap);

/*
 * strerror provided by libc consumes 2kb RAM
 * Moreover, V7 uses strerror mostly for
 * file operation, so returns of strerror
 * are undefined, because spiffs uses its own
 * error codes and doesn't provide
 * error descriptions
 */
char *strerror(int errnum) {
  static char buf[15];
  snprintf(buf, sizeof(buf), "err: %d", errnum);
  buf[sizeof(buf) - 1] = 0;
  return buf;
}

#ifndef RTOS_SDK

void *malloc(size_t size) {
  void *res = (void *) os_malloc(size);
#ifdef ESP_ABORT_ON_MALLOC_FAILURE
  if (res == NULL) abort();
#endif
  return res;
}

void free(void *ptr) {
  os_free(ptr);
}

void *realloc(void *ptr, size_t size) {
  void *res;
  /* ESP realloc is annoying - it prints an error message if reallocing to 0. */
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  res = (void *) os_realloc(ptr, size);
#ifdef ESP_ABORT_ON_MALLOC_FAILURE
  if (res == NULL) {
    printf("failed to alloc %u bytes, %d avail\n", size,
           system_get_free_heap_size());
    abort();
  }
#endif
  return res;
}

void *calloc(size_t num, size_t size) {
  void *res = (void *) os_zalloc(num * size);
#ifdef ESP_ABORT_ON_MALLOC_FAILURE
  if (res == NULL) abort();
#endif
  return res;
}

/*
 * TODO(alashkin): remove this code
 * if newlib's sprintf implementation
 * is good
 */
#if 0
int sprintf(char *buffer, const char *format, ...) {
  int ret;
  va_list arglist;
  va_start(arglist, format);
  ret = c_vsnprintf(buffer, ~0, format, arglist);
  va_end(arglist);
  return ret;
}
#endif

#ifndef LWIP_OPEN_SRC

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

#endif

void *_malloc_r(struct _reent *r, size_t size) {
  return malloc(size);
}

void _free_r(struct _reent *r, void *ptr) {
  free(ptr);
}

void *_realloc_r(struct _reent *r, void *ptr, size_t size) {
  return realloc(ptr, size);
}

#else /* !RTOS_SDK */

int printf(const char *format, ...) {
  va_list arglist;
  int ret;

  va_start(arglist, format);
  ret = vfprintf(stdout, format, arglist);
  va_end(arglist);
  return ret;
}

#endif /* !RTOS_SDK */

/*
 * TODO(alashkin): remove this code
 * if newlib's snprintf implementation
 * is good
 */
#if 0
int snprintf(char *buffer, size_t size, const char *format, ...) {
  int ret;
  va_list arglist;
  va_start(arglist, format);
  ret = c_vsnprintf(buffer, size, format, arglist);
  va_end(arglist);
  return ret;
}
#endif

int vsnprintf(char *buffer, size_t size, const char *format, va_list arg) {
  return c_vsnprintf(buffer, size, format, arg);
}

#ifndef ESP_DISABLE_STRTOD
/*
 * based on Source:
 * https://github.com/anakod/Sming/blob/master/Sming/system/stringconversion.cpp#L93
 */
double strtod(const char *str, char **endptr) {
  double result = 0.0;
  char c;
  const char *str_start;
  struct {
    unsigned neg : 1;        /* result is negative */
    unsigned decimals : 1;   /* parsing decimal part */
    unsigned is_exp : 1;     /* parsing exponent like e+5 */
    unsigned is_exp_neg : 1; /* exponent is negative */
  } flags = {0, 0, 0, 0};

  while (isspace((int) *str)) {
    str++;
  }

  if (*str == 0x00) {
    /* only space in str? */
    if (endptr != NULL) {
      *endptr = (char *) str;
    }
    return result;
  }

  /* Handle leading plus/minus signs */
  while (*str == '-' || *str == '+') {
    if (*str == '-') {
      flags.neg = !flags.neg;
    }
    str++;
  }

  str_start = str;

  if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
    /* base 16 */
    str += 2;
    while ((c = tolower((int) *str))) {
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
  } else if (*str == '0' && (*(str + 1) == 'b' || *(str + 1) == 'B')) {
    /* base 2 */
    str += 2;
    while ((c = *str)) {
      int d = c - '0';
      if (c != '0' && c != '1') break;
      result = 2 * result + d;
      str++;
    }
  } else if (*str == '0' && *(str + 1) >= '0' && *(str + 1) <= '7') {
    /* base 8 */
    while ((c = *str)) {
      int d = c - '0';
      if (c < '0' || c > '7') {
        if (c == '8' || c == '9') {
          /* fallback to base 10 */
          str = str_start;
          result = 0;
        }
        break;
      }
      result = 8 * result + d;
      str++;
    }
  }

  if (str == str_start) {
    /* base 10 */

    /* exponent specified explicitly, like in 3e-5, exponent is -5 */
    int exp = 0;
    /* exponent calculated from dot, like in 1.23, exponent is -2 */
    int exp_dot = 0;

    while ((c = *str)) {
      int d;

      if (c == '.') {
        if (!flags.decimals) {
          /* going to parse decimal part */
          flags.decimals = 1;
          str++;
          continue;
        } else {
          /* non-expected dot: assume number data is over */
          break;
        }
      } else if (c == 'e' || c == 'E') {
        /* going to parse exponent part */
        flags.is_exp = 1;
        str++;
        c = *str;

        /* check sign of the exponent */
        if (c == '-' || c == '+') {
          if (c == '-') {
            flags.is_exp_neg = 1;
          }
          str++;
        }

        continue;
      }

      d = c - '0';
      if (d < 0 || d > 9) {
        break;
      }

      if (!flags.is_exp) {
        /* apply current digit to the result */
        result = 10 * result + d;
        if (flags.decimals) {
          exp_dot--;
        }
      } else {
        /* apply current digit to the exponent */
        if (flags.is_exp_neg) {
          if (exp > -1022) {
            exp = 10 * exp - d;
          }
        } else {
          if (exp < 1023) {
            exp = 10 * exp + d;
          }
        }
      }

      str++;
    }

    exp += exp_dot;

    /*
     * TODO(dfrank): it probably makes sense not to adjust intermediate `double
     * result`, but build double number accordingly to IEEE 754 from taken
     * (integer) mantissa, exponent and sign. That would work faster, and we
     * can avoid any possible round errors.
     */

    /* if exponent is non-zero, apply it */
    if (exp != 0) {
      if (exp < 0) {
        while (exp++ != 0) {
          result /= 10;
        }
      } else {
        while (exp-- != 0) {
          result *= 10;
        }
      }
    }
  }

  if (flags.neg) {
    result = -result;
  }

  if (endptr != NULL) {
    *endptr = (char *) str;
  }

  return result;
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

#define flash_log10 log10
/*
 * Attempt to reproduce sprintf's %g
 * Returns _required_ number of symbols
 */

#define APPEND_CHAR(ch)              \
  {                                  \
    if (count < buf_size) *ptr = ch; \
    count++;                         \
  }

int double_to_str(char *buf, size_t buf_size, double val, int prec) {
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

  int mag1 = 0, mag2 = 0, count = 0;
  char *ptr = buf;
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
#endif /* ESP_DISABLE_STRTOD */

#undef APPEND_CHAR
#undef flash_log10

void abort(void) __attribute__((no_instrument_function));
void abort(void) {
  /* cause an unaligned access exception, that will drop you into gdb */
  *(int *) 1 = 1;
  while (1)
    ; /* avoid gcc warning because stdlib abort() has noreturn attribute */
}

void _exit(int status) {
  printf("_exit(%d)\n", status);
  abort();
}

int _gettimeofday_r(struct _reent *r, struct timeval *tp, void *tzp) {
  uint32_t time = system_get_time();
  tp->tv_sec = time / 1000000;
  tp->tv_usec = time % 1000000;
  return 0;
}

#ifndef RTOS_SDK
long int random(void) {
  return os_random();
}
#endif
