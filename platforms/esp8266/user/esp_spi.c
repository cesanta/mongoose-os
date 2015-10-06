/*
* The MIT License (MIT)
*
* Copyright (c) 2015 David Ogilvy (MetalPhreak)
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Modified by Cesanta Software, 2015
*/

#include "esp_spi.h"
#include <stdlib.h>

#ifdef RTOS_SDK
#include <eagle_soc.h>
#endif

struct esp_spi_connection {
  uint8_t spi_no;
};

int spi_init(spi_connection c) {
  struct esp_spi_connection *conn = (struct esp_spi_connection *) c;

  if (conn->spi_no > 1) return -1;

  spi_init_gpio(conn->spi_no, SPI_CLK_USE_DIV);
  spi_clock(conn->spi_no, SPI_CLK_PREDIV, SPI_CLK_CNTDIV);
  spi_tx_byte_order(conn->spi_no, SPI_BYTE_ORDER_HIGH_TO_LOW);
  spi_rx_byte_order(conn->spi_no, SPI_BYTE_ORDER_HIGH_TO_LOW);

  spi_finalize_init(conn->spi_no);

  return 0;
}

void spi_finalize_init(uint8_t spi_no) {
  SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_CS_SETUP | SPI_CS_HOLD);
  CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_FLASH_MODE);
}

void spi_init_gpio(uint8_t spi_no, uint8_t sysclk_as_spiclk) {
  if (spi_no > 1) return;

  uint32_t clock_div_flag = 0;
  if (sysclk_as_spiclk) {
    clock_div_flag = 0x0001;
  }

  if (spi_no == SPI) {
    WRITE_PERI_REG(PERIPHS_IO_MUX, 0x005 | (clock_div_flag << 8));
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CLK_U, 1);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CMD_U, 1);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA0_U, 1);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA1_U, 1);
  } else if (spi_no == HSPI) {
    WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105 | (clock_div_flag << 9));
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2);
  }
}

void spi_clock(uint8_t spi_no, uint16_t prediv, uint8_t cntdiv) {
  if (spi_no > 1) return;

  if ((prediv == 0) | (cntdiv == 0)) {
    WRITE_PERI_REG(SPI_CLOCK(spi_no), SPI_CLK_EQU_SYSCLK);

  } else {
    WRITE_PERI_REG(SPI_CLOCK(spi_no),
                   (((prediv - 1) & SPI_CLKDIV_PRE) << SPI_CLKDIV_PRE_S) |
                       (((cntdiv - 1) & SPI_CLKCNT_N) << SPI_CLKCNT_N_S) |
                       (((cntdiv >> 1) & SPI_CLKCNT_H) << SPI_CLKCNT_H_S) |
                       ((0 & SPI_CLKCNT_L) << SPI_CLKCNT_L_S));
  }
}

void spi_tx_byte_order(uint8_t spi_no, uint8_t byte_order) {
  if (spi_no > 1) return;

  if (byte_order) {
    SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_WR_BYTE_ORDER);
  } else {
    CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_WR_BYTE_ORDER);
  }
}

void spi_rx_byte_order(uint8_t spi_no, uint8_t byte_order) {
  if (spi_no > 1) return;

  if (byte_order) {
    SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_RD_BYTE_ORDER);
  } else {
    CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_RD_BYTE_ORDER);
  }
}

static void spi_set_command(uint8_t spi_no, uint8_t cmd_bits,
                            uint16_t cmd_data) {
  /* Setup Command Data */
  SET_PERI_REG_MASK(
      SPI_USER(spi_no),
      SPI_USR_COMMAND); /* enable COMMAND function in SPI module */
  uint16_t command = cmd_data
                     << (16 - cmd_bits); /* align command data to high bits */
  command =
      ((command >> 8) & 0xff) | ((command << 8) & 0xff00); /* swap byte order */
  WRITE_PERI_REG(SPI_USER2(spi_no), ((((cmd_bits - 1) & SPI_USR_COMMAND_BITLEN)
                                      << SPI_USR_COMMAND_BITLEN_S) |
                                     (command & SPI_USR_COMMAND_VALUE)));
}

static void spi_set_address(uint8_t spi_no, uint32_t addr_bits,
                            uint32_t addr_data) {
  SET_PERI_REG_MASK(SPI_USER(spi_no),
                    SPI_USR_ADDR); /*enable address function in SPI module */
  WRITE_PERI_REG(
      SPI_ADDR(spi_no),
      addr_data << (32 - addr_bits)); /* align address data to high bits */
}

static void spi_set_out_data(uint8_t spi_no, uint32_t dout_bits,
                             uint32_t dout_data) {
  SET_PERI_REG_MASK(SPI_USER(spi_no),
                    SPI_USR_MOSI); /* enable MOSI function in SPI module */
  /* copy data to W0 */
  if (READ_PERI_REG(SPI_USER(spi_no)) & SPI_WR_BYTE_ORDER) {
    WRITE_PERI_REG(SPI_W0(spi_no), dout_data << (32 - dout_bits));
  } else {
    uint8_t dout_extra_bits = dout_bits % 8;

    if (dout_extra_bits) {
      /* if your data isn't a byte multiple (8/16/24/32 bits)and you don't
      * have SPI_WR_BYTE_ORDER set, you need this to move the non-8bit
      * remainder to the MSBs
      * not sure if there's even a use case for this, but it's here if you
      * need it...
      * for example, 0xDA4 12 bits without SPI_WR_BYTE_ORDER would usually be
      * output as if it were 0x0DA4,
      * of which 0xA4, and then 0x0 would be shifted out (first 8 bits of low
      * byte, then 4 MSB bits of high byte - ie reverse byte order).
      * The code below shifts it out as 0xA4 followed by 0xD as you might
      * require.
      */
      WRITE_PERI_REG(
          SPI_W0(spi_no),
          ((0xFFFFFFFF << (dout_bits - dout_extra_bits) & dout_data)
               << (8 - dout_extra_bits) |
           (0xFFFFFFFF >> (32 - (dout_bits - dout_extra_bits))) & dout_data));
    } else {
      WRITE_PERI_REG(SPI_W0(spi_no), dout_data);
    }
  }
}

static void spi_begin_tran(uint8_t spi_no) {
  SET_PERI_REG_MASK(SPI_CMD(spi_no), SPI_USR);
}

static uint32_t spi_read_data(uint8_t spi_no, uint32_t din_bits) {
  while (spi_busy(spi_no))
    ; /* wait for SPI transaction to complete */

  if (READ_PERI_REG(SPI_USER(spi_no)) & SPI_RD_BYTE_ORDER) {
    return READ_PERI_REG(SPI_W0(spi_no)) >>
           (32 - din_bits); /* Assuming data in is written to MSB.*/
  } else {
    return READ_PERI_REG(
        SPI_W0(spi_no)); /* Read in the same way as DOUT is sent. Note
                          existing contents of SPI_W0 remain unless
                          overwritten! */
  }
}

uint32_t spi_txn(spi_connection c, uint8_t cmd_bits, uint16_t cmd_data,
                 uint8_t addr_bits, uint32_t addr_data, uint8_t dout_bits,
                 uint32_t dout_data, uint8_t din_bits, uint8_t dummy_bits) {
  struct esp_spi_connection *conn = (struct esp_spi_connection *) c;

  if (conn->spi_no > 1) return 0;

  while (spi_busy(conn->spi_no))
    ; /* wait for SPI to be ready*/

  /* Enable SPI Functions */
  /* disable MOSI, MISO, ADDR, COMMAND, DUMMY in case previously set */
  CLEAR_PERI_REG_MASK(SPI_USER(conn->spi_no), SPI_USR_MOSI | SPI_USR_MISO |
                                                  SPI_USR_COMMAND |
                                                  SPI_USR_ADDR | SPI_USR_DUMMY);

  /* enable functions based on number of bits. 0 bits = disabled. */
  if (din_bits) {
    SET_PERI_REG_MASK(SPI_USER(conn->spi_no), SPI_USR_MISO);
  }

  if (dummy_bits) {
    SET_PERI_REG_MASK(SPI_USER(conn->spi_no), SPI_USR_DUMMY);
  }

  /* Setup Bitlengths */
  WRITE_PERI_REG(
      SPI_USER1(conn->spi_no),
      ((addr_bits - 1) & SPI_USR_ADDR_BITLEN)
              << SPI_USR_ADDR_BITLEN_S | /* Number of bits in Address */
          ((dout_bits - 1) & SPI_USR_MOSI_BITLEN)
              << SPI_USR_MOSI_BITLEN_S | /* Number of bits to Send */
          ((din_bits - 1) & SPI_USR_MISO_BITLEN)
              << SPI_USR_MISO_BITLEN_S | /* Number of bits to receive */
          ((dummy_bits - 1) & SPI_USR_DUMMY_CYCLELEN)
              << SPI_USR_DUMMY_CYCLELEN_S); /* Number of Dummy bits to insert */

  if (cmd_bits) {
    spi_set_command(conn->spi_no, cmd_bits, cmd_data);
  }

  if (addr_bits) {
    spi_set_address(conn->spi_no, addr_bits, addr_data);
  }

  if (dout_bits) {
    spi_set_out_data(conn->spi_no, dout_bits, dout_data);
  }

  spi_begin_tran(conn->spi_no);

  if (din_bits) {
    return spi_read_data(conn->spi_no, din_bits);
  }

  return 1;
}

spi_connection sj_spi_create(struct v7 *v7) {
  /* Support HSPI only */
  struct esp_spi_connection *conn = malloc(sizeof(*conn));

  (void) v7;

  conn->spi_no = HSPI;

  return conn;
}

void sj_spi_close(spi_connection conn) {
  free(conn);
}

#ifdef MPL115A1_EXAMPLE_ENABLED

/*
 * Usage sample : measuring pressure with MPL115A1 barometer
 */

static double convert(uint16_t n, uint32_t fcoef, uint16_t neg_pos) {
  double res = 1;
  if ((n & (1 << neg_pos)) != 0) {
    n = ~n;
    res = -1;
  }

  res *= ((double) n / fcoef);
  return res;
}

void test_MPL115A1() {
  spi_init(HSPI);
  uint8_t a0_MSB = (uint8_t) spi_txn(HSPI, 0, 0, 0, 0, 8, 0x88, 8, 0);
  uint8_t a0_LSB = (uint8_t) spi_txn(HSPI, 0, 0, 0, 0, 8, 0x8A, 8, 0);
  uint8_t b1_MSB = (uint8_t) spi_txn(HSPI, 0, 0, 0, 0, 8, 0x8C, 8, 0);
  uint8_t b1_LSB = (uint8_t) spi_txn(HSPI, 0, 0, 0, 0, 8, 0x8E, 8, 0);
  uint8_t b2_MSB = (uint8_t) spi_txn(HSPI, 0, 0, 0, 0, 8, 0x90, 8, 0);
  uint8_t b2_LSB = (uint8_t) spi_txn(HSPI, 0, 0, 0, 0, 8, 0x92, 8, 0);
  uint8_t c12_MSB = (uint8_t) spi_txn(HSPI, 0, 0, 0, 0, 8, 0x94, 8, 0);
  uint8_t c12_LSB = (uint8_t) spi_txn(HSPI, 0, 0, 0, 0, 8, 0x96, 8, 0);
  spi_txn(HSPI, 0, 0, 0, 0, 8, 0x00, 8, 0);

  spi_txn(HSPI, 0, 0, 0, 0, 8, 0x24, 8, 0);

  os_delay_us(3000);

  uint8_t p_MSB = (uint8_t) spi_txn(HSPI, 0, 0, 0, 0, 8, 0x80, 8, 0);
  uint8_t p_LSB = (uint8_t) spi_txn(HSPI, 0, 0, 0, 0, 8, 0x82, 8, 0);
  uint8_t t_MSB = (uint8_t) spi_txn(HSPI, 0, 0, 0, 0, 8, 0x84, 8, 0);
  uint8_t t_LSB = (uint8_t) spi_txn(HSPI, 0, 0, 0, 0, 8, 0x86, 8, 0);
  spi_txn(HSPI, 0, 0, 0, 0, 8, 0x00, 8, 0);

  double a0, b1, b2, c12, t_adc, p_adc;
  a0 = convert(a0_MSB << 8 | a0_LSB, 1 << 3, 15);
  b1 = convert(b1_MSB << 8 | b1_LSB, 1 << 13, 15);
  b2 = convert(b2_MSB << 8 | b2_LSB, 1 << 14, 15);
  c12 = convert((c12_MSB << 8 | c12_LSB) >> 2, 1 << 22, 13);
  t_adc = (t_MSB << 8 | t_LSB) >> 6;
  p_adc = (p_MSB << 8 | p_LSB) >> 6;

  double p_comp = a0 + (b1 + c12 * t_adc) * p_adc + b2 * t_adc;
  double pressure = p_comp * (115 - 50) / 1023 + 50;

  printf("Pressure=%d\n", (int) (pressure * 10));
}

#endif
