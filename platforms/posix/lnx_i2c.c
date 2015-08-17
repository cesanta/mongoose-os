#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#ifndef SJ_DISABLE_I2C

#include <sj_hal.h>
#include <sj_i2c.h>

struct lnx_i2c_connection {
  uint8_t bus_no;

  int fd;
};

/*
 * Note: In Linux, i2c can be accessed via ioctl(file, I2C_SMBUS, ....) or
 * via usual file operations like read/write
 * in some cases ioctl might be faster (for example, it allows to read and
 * write in one call, i.e. w/out extra call to write(..., addr)
 * Here we use file operation because it allows to provide
 * ESP-like API
 */
int lnx_i2c_read_shim(int fd, uint8_t *buf, size_t buf_size) {
  int ret = read(fd, buf, buf_size);
  if (ret == -1) {
    fprintf(stderr, "Cannot read from i2c bus : %d (%s)\n", errno,
            strerror(errno));
  }
  return ret;
}

int lnx_i2c_write_shim(int fd, uint8_t *buf, size_t buf_size) {
  int ret = write(fd, buf, buf_size);
  if (ret == -1) {
    fprintf(stderr, "Cannot write to i2c bus : %d (%s)\n", errno,
            strerror(errno));
  }
  return ret;
}

int i2c_init(i2c_connection c) {
  struct lnx_i2c_connection *conn = (struct lnx_i2c_connection *) c;
  char bus_name[50];

  snprintf(bus_name, sizeof(bus_name), "/dev/i2c-%d", conn->bus_no);

  conn->fd = open(bus_name, O_RDWR);

  if (conn->fd < 0) {
    fprintf(stderr, "Cannot access i2c bus (%s): %d (%s)\n", bus_name, errno,
            strerror(errno));
    return errno;
  }

  return 0;
}

enum i2c_ack_type i2c_start(i2c_connection c, uint16_t addr, enum i2c_rw mode) {
  struct lnx_i2c_connection *conn = (struct lnx_i2c_connection *) c;
  /*
   * In Linux we should't set RW here
   * only address have to be set
   * TODO(alashkin): find the way to determine
   * if real error occured
   */
  if (ioctl(conn->fd, I2C_SLAVE, addr) < 0) {
    fprintf(stderr, "Cannot access i2c device: %d (%s)\n", errno,
            strerror(errno));
    return I2C_ERR;
  }

  return I2C_ACK;
}

void i2c_stop(i2c_connection c) {
  /* for compatibility only */
  (void) c;
}

enum i2c_ack_type i2c_send_byte(i2c_connection c, uint8_t data) {
  struct lnx_i2c_connection *conn = (struct lnx_i2c_connection *) c;
  return lnx_i2c_write_shim(conn->fd, &data, sizeof(data)) == 1 ? I2C_ACK
                                                                : I2C_ERR;
}

enum i2c_ack_type i2c_send_bytes(i2c_connection c, uint8_t *buf,
                                 size_t buf_size) {
  struct lnx_i2c_connection *conn = (struct lnx_i2c_connection *) c;
  return lnx_i2c_write_shim(conn->fd, buf, buf_size) == 0 ? I2C_ACK : I2C_ERR;
}

uint8_t i2c_read_byte(i2c_connection c, enum i2c_ack_type ack_type) {
  struct lnx_i2c_connection *conn = (struct lnx_i2c_connection *) c;
  (void) ack_type;

  uint8_t ret;
  if (lnx_i2c_read_shim(conn->fd, &ret, sizeof(ret)) != 1) {
    return 0x00; /* Error handling? */
  }

  return ret;
}

void i2c_read_bytes(i2c_connection c, size_t n, uint8_t *buf,
                    enum i2c_ack_type last_ack_type) {
  struct lnx_i2c_connection *conn = (struct lnx_i2c_connection *) c;
  lnx_i2c_read_shim(conn->fd, buf, n);

  /*
   * ESP's version doesn't provide return value
   * So, for compatibility reasons we do the same here
   * TODO(alashkin): fix esp?
   */
}

void i2c_send_ack(i2c_connection c, enum i2c_ack_type ack_type) {
  /* For compatibility only */
  (void) c;
}

void i2c_close(i2c_connection c) {
  struct lnx_i2c_connection *conn = (struct lnx_i2c_connection *) c;
  close(conn->fd);
}

/* HAL functions */
i2c_connection sj_i2c_create(struct v7 *v7, v7_val_t args) {
  struct lnx_i2c_connection *conn;
  v7_val_t bus_no_val = v7_array_get(v7, args, 0);
  double bus_no = v7_to_number(bus_no_val);

  if (!v7_is_number(bus_no) || bus_no < 0) {
    v7_throw(v7, "Missing bus number argument.");
  }

  conn = malloc(sizeof(*conn));
  conn->bus_no = bus_no;

  return conn;
}

void sj_i2c_close(i2c_connection conn) {
  i2c_close(conn);
  free(conn);
}

#endif /* SJ_DISABLE_I2C */
