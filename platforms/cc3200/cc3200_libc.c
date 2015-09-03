#include <reent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "hw_types.h"
#include "hw_memmap.h"
#include "rom.h"
#include "rom_map.h"
#include "uart.h"
#include "utils.h"

#include "cc3200_fs.h"
#include "config.h"

/* FIXME */
int _gettimeofday_r(struct _reent *r, struct timeval *tp, void *tzp) {
  tp->tv_sec = 42;
  tp->tv_usec = 123;
  return 0;
}

long int random(void) {
  return 42; /* FIXME */
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
