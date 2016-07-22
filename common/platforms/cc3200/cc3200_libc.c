/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if CS_PLATFORM == CS_P_CC3200

#include <stdio.h>
#include <string.h>

#ifndef __TI_COMPILER_VERSION__
#include <reent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/prcm.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/uart.h>
#include <driverlib/utils.h>

#define CONSOLE_UART UARTA0_BASE

#ifdef __TI_COMPILER_VERSION__
int asprintf(char **strp, const char *fmt, ...) {
  va_list ap;
  int len;

  *strp = malloc(BUFSIZ);
  if (*strp == NULL) return -1;

  va_start(ap, fmt);
  len = vsnprintf(*strp, BUFSIZ, fmt, ap);
  va_end(ap);

  if (len > 0) {
    *strp = realloc(*strp, len + 1);
    if (*strp == NULL) return -1;
  }

  if (len >= BUFSIZ) {
    va_start(ap, fmt);
    len = vsnprintf(*strp, len + 1, fmt, ap);
    va_end(ap);
  }

  return len;
}

#if MG_TI_NO_HOST_INTERFACE
time_t HOSTtime() {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return tp.tv_sec;
}
#endif

#endif /* __TI_COMPILER_VERSION__ */

#ifndef __TI_COMPILER_VERSION__
int _gettimeofday_r(struct _reent *r, struct timeval *tp, void *tzp) {
#else
int gettimeofday(struct timeval *tp, void *tzp) {
#endif
  unsigned long long r1 = 0, r2;
  /* Achieve two consecutive reads of the same value. */
  do {
    r2 = r1;
    r1 = PRCMSlowClkCtrFastGet();
  } while (r1 != r2);
  /* This is a 32768 Hz counter. */
  tp->tv_sec = (r1 >> 15);
  /* 1/32768-th of a second is 30.517578125 microseconds, approx. 31,
   * but we round down so it doesn't overflow at 32767 */
  tp->tv_usec = (r1 & 0x7FFF) * 30;
  return 0;
}

void fprint_str(FILE *fp, const char *str) {
  while (*str != '\0') {
    if (*str == '\n') MAP_UARTCharPut(CONSOLE_UART, '\r');
    MAP_UARTCharPut(CONSOLE_UART, *str++);
  }
}

void _exit(int status) {
  fprint_str(stderr, "_exit\n");
  /* cause an unaligned access exception, that will drop you into gdb */
  *(int *) 1 = status;
  while (1)
    ; /* avoid gcc warning because stdlib abort() has noreturn attribute */
}

void _not_implemented(const char *what) {
  fprint_str(stderr, what);
  fprint_str(stderr, " is not implemented\n");
  _exit(42);
}

int _kill(int pid, int sig) {
  (void) pid;
  (void) sig;
  _not_implemented("_kill");
  return -1;
}

int _getpid() {
  fprint_str(stderr, "_getpid is not implemented\n");
  return 42;
}

int _isatty(int fd) {
  /* 0, 1 and 2 are TTYs. */
  return fd < 2;
}

#endif /* CS_PLATFORM == CS_P_CC3200 */
