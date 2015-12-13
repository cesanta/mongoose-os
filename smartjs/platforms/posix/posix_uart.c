#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <sj_uart.h>
#include <mongoose.h>
#include <sj_mongoose.h>

#define SJ_UART_DEFAULT_BAUD 115000

static void uart_ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  size_t n;
  (void) ev_data;

  switch (ev) {
    case MG_EV_RECV:
      do {
        n = sj_uart_recv_cb(nc->user_data, nc->recv_mbuf.buf,
                            nc->recv_mbuf.len);
        mbuf_remove(&nc->recv_mbuf, n);
      } while (n > 0);
      break;
  }
}

void sj_hal_write_uart(void *uart, const char *d, size_t len) {
  struct mg_connection *nc = (struct mg_connection *) uart;
  mg_send(nc, d, len);
}

v7_val_t sj_hal_read_uart(struct v7 *v7, void *uart, size_t len) {
  struct mg_connection *nc = (struct mg_connection *) uart;
  v7_val_t s;

  if (len > nc->recv_mbuf.len) {
    len = nc->recv_mbuf.len;
  }
  s = v7_create_string(v7, nc->recv_mbuf.buf, len, 1);
  mbuf_remove(&nc->recv_mbuf, len);
  return s;
}

void *sj_hal_open_uart(const char *name, void *ctx) {
  struct termios tio;
  int fd, flags;
  struct mg_connection *nc;

  fprintf(stderr, "opening uart %s\n", name);
  fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd < 0) {
    perror("opening");
    return NULL;
  }
  fprintf(stderr, "opened %d\n", fd);

  flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  cfmakeraw(&tio);
  cfsetispeed(&tio, SJ_UART_DEFAULT_BAUD);
  cfsetospeed(&tio, SJ_UART_DEFAULT_BAUD);
  tcsetattr(fd, TCSANOW, &tio);

  nc = mg_add_sock(&sj_mgr, fd, uart_ev_handler);
  nc->user_data = ctx;

  if (nc == NULL) {
    fprintf(stderr, "cannto add socket to mongoose\n");
    return NULL;
  }

#if 0  
  int len;
  char buf[8192];
  for (;;) {
    len = read(fd, &buf[0], 8192);
    if (len > 0) {
      write(1, buf, len);
    } else if (len < 0) {
      perror("read error");
    }
    sleep(1);
  }
#endif
  return nc;
}

void sj_hal_close_uart(void *uart) {
  struct mg_connection *nc = (struct mg_connection *) uart;
  fprintf(stderr, "closing %p sock %d\n", uart, nc->sock);
}
