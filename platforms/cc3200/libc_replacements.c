#include <reent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "hw_types.h"
#include "hw_memmap.h"
#include "rom_map.h"
#include "uart.h"
#include "utils.h"

#include "config.h"

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
}

int _getpid() {
  fprint_str(stderr, "_getpid is not implemented\n");
  return 42;
}

int _isatty(int fd) {
  /* 0, 1 and 2 are TTYs. */
  return fd < 2;
}

ssize_t _write(int fd, const void *buf, size_t count) {
  if (fd != 1 && fd != 2) {
    _not_implemented("_write to files");
  }
  for (size_t i = 0; i < count; i++) {
    const char c = ((const char *) buf)[i];
    if (c == '\n') MAP_UARTCharPut(CONSOLE_UART, '\r');
    MAP_UARTCharPut(CONSOLE_UART, c);
  }
  return count;
}

int _close(int fd) {
  _not_implemented("_close");
}

off_t _lseek(int fd, off_t offset, int whence) {
  _not_implemented("_lseek");
}

int _fstat(int fd, struct stat *s) {
  if (fd > 2) {
    _not_implemented("_fstat on files");
  }
  /* Create barely passable stats for STD{IN,OUT,ERR}. */
  memset(s, 0, sizeof(*s));
  s->st_ino = fd;
  s->st_rdev = fd;
  s->st_mode = S_IFCHR | 0666;
  return 0;
}

ssize_t _read(int fd, void *buf, size_t count) {
  _not_implemented("_read");
}
