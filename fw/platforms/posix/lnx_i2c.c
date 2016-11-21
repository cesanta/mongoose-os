/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_i2c.h"

#if MIOT_ENABLE_I2C

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

#include "fw/src/miot_hal.h"
#include "fw/src/miot_i2c.h"
#include "v7/v7.h"

struct miot_i2c {
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

struct miot_i2c *miot_i2c_create(const struct sys_config_i2c *cfg) {
  char bus_name[50];
  struct miot_i2c *c = NULL;

  c = calloc(1, sizeof(*c));
  if (c == NULL) return NULL;

  snprintf(bus_name, sizeof(bus_name), "/dev/i2c-%d", cfg->bus_no);

  c->fd = open(bus_name, O_RDWR);

  if (c->fd < 0) {
    fprintf(stderr, "Cannot access i2c bus (%s): %d (%s)\n", bus_name, errno,
            strerror(errno));
    free(c);
    c = NULL;
  }

  return c;
}

enum i2c_ack_type miot_i2c_start(struct miot_i2c *c, uint16_t addr, enum i2c_rw mode) {
  (void) mode;
  /*
   * In Linux we should't set RW here
   * only address have to be set
   * TODO(alashkin): find the way to determine
   * if real error occured
   */
  if (ioctl(c->fd, I2C_SLAVE, addr) < 0) {
    fprintf(stderr, "Cannot access i2c device: %d (%s)\n", errno,
            strerror(errno));
    return I2C_ERR;
  }

  return I2C_ACK;
}

void miot_i2c_stop(struct miot_i2c *c) {
  /* for compatibility only */
  (void) c;
}

enum i2c_ack_type miot_i2c_send_byte(struct miot_i2c *c, uint8_t b) {
  return lnx_i2c_write_shim(c->fd, &b, sizeof(b)) == 1 ? I2C_ACK : I2C_ERR;
}

uint8_t miot_i2c_read_byte(struct miot_i2c *c, enum i2c_ack_type ack_type) {
  (void) ack_type;

  uint8_t ret;
  if (lnx_i2c_read_shim(c->fd, &ret, sizeof(ret)) != 1) {
    return 0x00; /* Error handling? */
  }

  return ret;
}

void miot_i2c_send_ack(struct miot_i2c *c, enum i2c_ack_type ack_type) {
  /* For compatibility only */
  (void) c;
  (void) ack_type;
}

void miot_i2c_close(struct miot_i2c *c) {
  if (c->fd >= 0) close(c->fd);
  free(c);
}

#if MIOT_ENABLE_JS && MIOT_ENABLE_I2C_API
enum v7_err miot_i2c_create_js(struct v7 *v7, struct miot_i2c **res) {
  enum v7_err rcode = V7_OK;
  struct sys_config_i2c cfg;
  cfg.bus_no = v7_get_double(v7, v7_arg(v7, 0));
  struct miot_i2c *conn = miot_i2c_create(&cfg);

  if (conn != NULL) {
    *res = conn;
  } else {
    rcode = v7_throwf(v7, "Error", "Failed to creat I2C connection");
  }

  return rcode;
}
#endif /* MIOT_ENABLE_JS && MIOT_ENABLE_I2C_API */

#endif /* MIOT_ENABLE_I2C */
