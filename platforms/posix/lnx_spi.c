#ifndef SJ_DISABLE_SPI

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <errno.h>
#include <string.h>

#include <sj_spi.h>

struct lnx_spi_connection {
  int spi_no;
  int fd;
};

/*
 * Default values
 * TODO(alashkin): make it changeable
 */
static const uint8_t mode = 0;
static const uint8_t bits = 8;
static const uint32_t speed = 4000000;

int spi_init(spi_connection c) {
  struct lnx_spi_connection *conn = (struct lnx_spi_connection *) c;
  char bus_name[50];
  int ret = 0;

  snprintf(bus_name, sizeof(bus_name), "/dev/spidev0.%d", conn->spi_no);

  conn->fd = open(bus_name, O_RDWR);
  if (conn->fd < 0) {
    fprintf(stderr, "Cannot open device %s - %d (%s)\n ", bus_name, errno,
            strerror(errno));
    return -1;
  }

  ret = ioctl(conn->fd, SPI_IOC_WR_MODE, &mode);
  if (ret < 0) {
    fprintf(stderr, "Can't set spi mode - %d (%s)\n", errno, strerror(errno));
    return -1;
  }

  ret = ioctl(conn->fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
  if (ret < 0) {
    fprintf(stderr, "Can't set bits per word - %d (%s)\n", errno,
            strerror(errno));
    return -1;
  }

  ret = ioctl(conn->fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
  if (ret < 0) {
    fprintf(stderr, "Can't set max speedd - %d (%s)\n", errno, strerror(errno));
    return -1;
  }

  return 0;
}

uint32_t spi_txn(spi_connection c, uint8_t cmd_bits, uint16_t cmd_data,
                 uint8_t addr_bits, uint32_t addr_data, uint8_t dout_bits,
                 uint32_t dout_data, uint8_t din_bits, uint8_t dummy_bits) {
  /* Emulate ESP SPI API */
  struct lnx_spi_connection *conn = (struct lnx_spi_connection *) c;
  uint8_t op_count =
      (cmd_bits != 0) + (addr_bits != 0) + (dout_bits != 0) + (din_bits != 0);
  (void) dummy_bits;

  struct spi_ioc_transfer *trs = calloc(op_count, sizeof(*trs));
  uint8_t op_no = 0;
  uint32_t zero_buf = 0, ret = 0;

  int res;

  if (cmd_bits != 0) {
    trs[op_no].tx_buf = (unsigned long) &cmd_data;
    trs[op_no].len = cmd_bits / 8 + ((cmd_bits % 8) & 1);
    op_no++;
  }

  if (addr_bits != 0) {
    trs[op_no].tx_buf = (unsigned long) &addr_data;
    trs[op_no].len = addr_bits / 8 + ((addr_bits % 8) & 1);
    op_no++;
  }

  if (dout_bits != 0) {
    trs[op_no].tx_buf = (unsigned long) &dout_data;
    trs[op_no].len = dout_bits / 8 + ((dout_bits % 8) & 1);
    op_no++;
  }

  if (din_bits != 0) {
    trs[op_no].tx_buf = (unsigned long) &zero_buf;
    trs[op_no].rx_buf = (unsigned long) &ret;
    trs[op_no].len = din_bits / 8 + ((din_bits % 8) & 1);
    op_no++;
  }

  res = ioctl(conn->fd, SPI_IOC_MESSAGE(op_no), trs);

  free(trs);

  if (res < 0) {
    fprintf(stderr, "Cannot access bus: %d (%s)\n", errno, strerror(errno));
    return 0;
  }

  return ret;
}

/* HAL functions */
spi_connection sj_spi_create(struct v7 *v7, v7_val_t args) {
  struct lnx_spi_connection *conn;
  v7_val_t spi_no_val = v7_array_get(v7, args, 0);
  double spi_no = v7_to_number(spi_no_val);
  ;

  if (!v7_is_number(spi_no_val) || spi_no < 0) {
    v7_throw(v7, "Missing arguments for SPI number or wrong type.");
  }

  conn = malloc(sizeof(*conn));
  conn->spi_no = v7_to_number(spi_no);

  return conn;
}

void sj_spi_close(spi_connection c) {
  struct lnx_spi_connection *conn = (struct lnx_spi_connection *) c;
  close(conn->fd);
  free(c);
}

#endif
