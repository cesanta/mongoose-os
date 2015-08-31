#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hw_types.h"
#include "hw_memmap.h"
#include "rom_map.h"
#include "uart.h"

#include "config.h"
#include "cc3200_fs.h"
#include "cc3200_fs_spiffs.h"

#define NUM_SYS_FDS 3

int _open(const char *pathname, int flags, mode_t mode) {
  int fd = fs_spiffs_open(pathname, flags, mode);
  if (fd < 0) return fd;
  return fd + NUM_SYS_FDS;
}

int _close(int fd) {
  if (fd <= 0) return -1;
  if (fd > 2) return fs_spiffs_close(fd - NUM_SYS_FDS);
  return -1;
}

off_t _lseek(int fd, off_t offset, int whence) {
  if (fd <= 2) return -1;
  return fs_spiffs_lseek(fd - NUM_SYS_FDS, offset, whence);
}

int _fstat(int fd, struct stat *s) {
  if (fd < 0) return -1;
  if (fd > 2) return fs_spiffs_fstat(fd - NUM_SYS_FDS, s);
  /* Create barely passable stats for STD{IN,OUT,ERR}. */
  memset(s, 0, sizeof(*s));
  s->st_ino = fd;
  s->st_rdev = fd;
  s->st_mode = S_IFCHR | 0666;
  return 0;
}

ssize_t _read(int fd, void *buf, size_t count) {
  if (fd > 2) return fs_spiffs_read(fd - NUM_SYS_FDS, buf, count);
  if (fd != 0) return -1;
  /* Should we allow reading from stdin = uart? */
  return -1;
}

ssize_t _write(int fd, const void *buf, size_t count) {
  if (fd <= 0) return -1;
  if (fd > 2) return fs_spiffs_write(fd - NUM_SYS_FDS, buf, count);
  for (size_t i = 0; i < count; i++) {
    const char c = ((const char *) buf)[i];
    if (c == '\n') MAP_UARTCharPut(CONSOLE_UART, '\r');
    MAP_UARTCharPut(CONSOLE_UART, c);
  }
  return count;
}

int _rename(const char *from, const char *to) {
  return fs_spiffs_rename(from, to);
}

int _link(const char *from, const char *to) {
  dprintf(("link(%s, %s)\n", from, to));
  errno = ENOTSUP;
  return -1;
}

int _unlink(const char *filename) {
  return fs_spiffs_unlink(filename);
}
