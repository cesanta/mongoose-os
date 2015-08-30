#include <reent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "hw_types.h"
#include "hw_memmap.h"
#include "rom_map.h"
#include "uart.h"
#include "utils.h"

#include "cc3200_fs.h"
#include "config.h"

#define NUM_SYS_FDS 3

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

ssize_t _write(int fd, const void *buf, size_t count) {
  if (fd <= 0) return -1;
  if (fd > 2) return fs_write(fd - NUM_SYS_FDS, buf, count);
  for (size_t i = 0; i < count; i++) {
    const char c = ((const char *) buf)[i];
    if (c == '\n') MAP_UARTCharPut(CONSOLE_UART, '\r');
    MAP_UARTCharPut(CONSOLE_UART, c);
  }
  return count;
}

int _open(const char *pathname, int flags, mode_t mode) {
  int fd = fs_open(pathname, flags, mode);
  if (fd < 0) return fd;
  return fd + NUM_SYS_FDS;
}

int _close(int fd) {
  if (fd <= 0) return -1;
  if (fd > 2) return fs_close(fd - NUM_SYS_FDS);
  return -1;
}

off_t _lseek(int fd, off_t offset, int whence) {
  if (fd <= 2) return -1;
  return fs_lseek(fd - NUM_SYS_FDS, offset, whence);
}

int _fstat(int fd, struct stat *s) {
  if (fd < 0) return -1;
  if (fd > 2) return fs_fstat(fd - NUM_SYS_FDS, s);
  /* Create barely passable stats for STD{IN,OUT,ERR}. */
  memset(s, 0, sizeof(*s));
  s->st_ino = fd;
  s->st_rdev = fd;
  s->st_mode = S_IFCHR | 0666;
  return 0;
}

ssize_t _read(int fd, void *buf, size_t count) {
  if (fd > 2) return fs_read(fd - NUM_SYS_FDS, buf, count);
  if (fd != 0) return -1;
  /* Should we allow reading from stdin = uart? */
  return -1;
}
