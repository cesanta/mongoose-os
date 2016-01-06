/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 *
 * Spiffy flasher. Implements strong checksums (MD5) and can use higher
 * baud rates. Actual max baud rate will differe from device to device,
 * but 921K seems to be common.
 *
 * SLIP protocol is used for communication.
 * First packet is a single byte - command number.
 * After that, a packet with a variable number of 32-bit (LE) arguments,
 * depending on command.
 *
 * Then command produces variable number of packets of output, but first
 * packet of length 1 is the response code: 0 for success, non-zero - error.
 *
 * See individual command description below.
 */

#include "rom_functions.h"

#include "eagle_soc.h"
#include "ets_sys.h"
#include "examples/driver_lib/include/driver/uart_register.h"

#include "slip.h"

/* Param: baud rate. */
uint32_t params[1] __attribute__((section(".params")));

/* TODO(rojer): read sector and block sizes from device ROM. */
#define FLASH_SECTOR_SIZE 4096
#define FLASH_BLOCK_SIZE 65536
#define UART_CLKDIV_26MHZ(B) (52000000 + B / 2) / B

#define UART_BUF_SIZE 4096
#define SPI_WRITE_SIZE 1024

#define UART_RX_INTS (UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA)

/* From spi_register.h */
#define REG_SPI_BASE(i)     (0x60000200 - i*0x100)

#define SPI_CMD(i)          (REG_SPI_BASE(i) + 0x0)
#define SPI_RDID            (BIT(28))

#define SPI_W0(i)         (REG_SPI_BASE(i) + 0x40)

enum stub_cmd {
  /*
   * Erase a region of SPI flash.
   *
   * Args: addr, len; must be FLASH_SECTOR_SIZE-aligned.
   * Input: none.
   * Output: none.
   */
  CMD_FLASH_ERASE = 0,

  /*
   * Write to the SPI flash.
   *
   * Args: addr, len, erase; addr and len must be SECTOR_SIZE-aligned.
   *       If erase != 0, perform erase before writing.
   * Input: Stream of data to be written, note: no SLIP encapsulation here.
   * Output: SLIP packets with number of bytes written after every write.
   *         This can (and should) be used for flow control. Flasher will
   *         write in 1K chunks but will buffer up to 4K of data
   *         Use this feedback to keep buffer above 1K but below 4K.
   *         Final packet will contain MD5 digest of the data written.
   */
  CMD_FLASH_WRITE = 1,

  /*
   * Read from the SPI flash.
   *
   * Args: addr, len, block_size; no alignment requirements, block_size <= 4K.
   * Input: none.
   * Output: Packets of up to block_size with data.
   *         Last packet is the MD5 digest of the data.
   *
   * Note: No flow control is performed, it is assumed that the host can cope
   * with the inbound stream.
   */
  CMD_FLASH_READ = 2,

  /*
   * Compute MD5 digest of the specified flash region.
   *
   * Args: addr, len, digest_block_size; no alignment requirements.
   * Input: none.
   * Output: If block digests are not enabled (digest_block_size == 0),
   *         only overall digest is produced.
   *         Otherwise, there will be a separate digest for each block,
   *         the remainder (if any) and the overall digest at the end.
   */
  CMD_FLASH_DIGEST = 3,

  CMD_FLASH_READ_CHIP_ID = 4,

  /*
   * Jump to _ResetVector.
   *
   * No Args, input or output (but status 0 is emitted before reboot).
   *
   * Note: currently this reboots back into ROM. Let's call it a feature.
   */
  CMD_REBOOT = 5,
};

int do_flash_erase(uint32_t addr, uint32_t len) {
  if (addr % FLASH_SECTOR_SIZE != 0) return 0x32;
  if (len % FLASH_SECTOR_SIZE != 0) return 0x33;
  if (SPIUnlock() != 0) return 0x34;

  while (len > 0 && (addr % FLASH_BLOCK_SIZE != 0)) {
    if (SPIEraseSector(addr / FLASH_SECTOR_SIZE) != 0) return 0x35;
    len -= FLASH_SECTOR_SIZE;
    addr += FLASH_SECTOR_SIZE;
  }

  while (len > FLASH_BLOCK_SIZE) {
    if (SPIEraseBlock(addr / FLASH_BLOCK_SIZE) != 0) return 0x36;
    len -= FLASH_BLOCK_SIZE;
    addr += FLASH_BLOCK_SIZE;
  }

  while (len > 0) {
    if (SPIEraseSector(addr / FLASH_SECTOR_SIZE) != 0) return 0x37;
    len -= FLASH_SECTOR_SIZE;
    addr += FLASH_SECTOR_SIZE;
  }

  return 0;
}

struct uart_buf {
  uint8_t data[UART_BUF_SIZE];
  uint32_t nr;
  uint8_t *pr, *pw;
};

void uart_isr(void *arg) {
  uint32_t int_st = READ_PERI_REG(UART_INT_ST(0));
  struct uart_buf *ub = (struct uart_buf *) arg;
  while (1) {
    uint32_t fifo_len = READ_PERI_REG(UART_STATUS(0)) & 0xff;
    if (fifo_len == 0) break;
    while (fifo_len-- > 0) {
      uint8_t byte = READ_PERI_REG(UART_FIFO(0)) & 0xff;
      *ub->pw++ = byte;
      ub->nr++;
      if (ub->pw >= ub->data + UART_BUF_SIZE) ub->pw = ub->data;
    }
  }
  WRITE_PERI_REG(UART_INT_CLR(0), int_st);
}

int do_flash_write(uint32_t addr, uint32_t len, uint32_t erase) {
  struct uart_buf ub;
  uint8_t digest[16];
  uint32_t num_written = 0;
  struct MD5Context ctx;
  MD5Init(&ctx);

  if (addr % FLASH_SECTOR_SIZE != 0) return 0x32;
  if (len % FLASH_SECTOR_SIZE != 0) return 0x33;
  if (SPIUnlock() != 0) return 0x34;

  if (erase) {
    int ret = do_flash_erase(addr, len);
    if (ret != 0) return ret;
  }

  ub.nr = 0;
  ub.pr = ub.pw = ub.data;
  ets_isr_attach(ETS_UART_INUM, uart_isr, &ub);
  SET_PERI_REG_MASK(UART_INT_ENA(0), UART_RX_INTS);
  ets_isr_unmask(1 << ETS_UART_INUM);

  SLIP_send(&num_written, 4);

  while (num_written < len) {
    volatile uint32_t *nr = &ub.nr;
    while (*nr < SPI_WRITE_SIZE) {
    }
    MD5Update(&ctx, ub.pr, SPI_WRITE_SIZE);
    if (SPIWrite(addr, ub.pr, SPI_WRITE_SIZE) != 0) return 0x35;
    ets_intr_lock();
    *nr -= SPI_WRITE_SIZE;
    ets_intr_unlock();
    num_written += SPI_WRITE_SIZE;
    addr += SPI_WRITE_SIZE;
    ub.pr += SPI_WRITE_SIZE;
    if (ub.pr >= ub.data + UART_BUF_SIZE) ub.pr = ub.data;
    SLIP_send(&num_written, 4);
  }

  ets_isr_mask(1 << ETS_UART_INUM);

  MD5Final(digest, &ctx);
  SLIP_send(digest, 16);

  return 0;
}

int do_flash_read(uint32_t addr, uint32_t len, uint32_t block_size) {
  uint8_t buf[FLASH_SECTOR_SIZE];
  uint8_t digest[16];
  struct MD5Context ctx;
  if (block_size > sizeof(buf)) return 0x52;
  MD5Init(&ctx);
  while (len > 0) {
    uint32_t n = len;
    struct MD5Context block_ctx;
    MD5Init(&block_ctx);
    if (n > block_size) n = block_size;
    if (SPIRead(addr, buf, n) != 0) return 0x53;
    MD5Update(&ctx, buf, n);
    send_packet(buf, n);
    addr += n;
    len -= n;
  }
  MD5Final(digest, &ctx);
  send_packet(digest, sizeof(digest));
  return 0;
}

int do_flash_digest(uint32_t addr, uint32_t len, uint32_t digest_block_size) {
  uint8_t buf[FLASH_SECTOR_SIZE];
  uint8_t digest[16];
  uint32_t read_block_size =
      digest_block_size ? digest_block_size : sizeof(buf);
  struct MD5Context ctx;
  if (digest_block_size > sizeof(buf)) return 0x62;
  MD5Init(&ctx);
  while (len > 0) {
    uint32_t n = len;
    struct MD5Context block_ctx;
    MD5Init(&block_ctx);
    if (n > read_block_size) n = read_block_size;
    if (SPIRead(addr, buf, n) != 0) return 0x63;
    MD5Update(&ctx, buf, n);
    if (digest_block_size > 0) {
      MD5Update(&block_ctx, buf, n);
      MD5Final(digest, &block_ctx);
      send_packet(digest, sizeof(digest));
    }
    addr += n;
    len -= n;
  }
  MD5Final(digest, &ctx);
  send_packet(digest, sizeof(digest));
  return 0;
}

int do_flash_read_chip_id() {
  uint32_t chip_id = 0;
  WRITE_PERI_REG(SPI_CMD(0), SPI_RDID);
  while (READ_PERI_REG(SPI_CMD(0)) & SPI_RDID) {}
  chip_id = READ_PERI_REG(SPI_W0(0)) & 0xFFFFFF;
  send_packet(&chip_id, sizeof(chip_id));
  return 0;
}

void cmd_loop() {
  while (1) {
    uint8_t cmd;
    uint32_t args[4];
    uint32_t len = SLIP_recv(&cmd, 1);
    if (len != 1) {
      continue;
    }
    uint8_t resp = 0xff;
    switch (cmd) {
      case CMD_FLASH_ERASE: {
        len = SLIP_recv(args, sizeof(args));
        if (len == 8) {
          resp = do_flash_erase(args[0] /* addr */, args[1] /* len */);
        } else {
          resp = 0x31;
        }
        break;
      }
      case CMD_FLASH_WRITE: {
        len = SLIP_recv(args, sizeof(args));
        if (len == 12) {
          resp = do_flash_write(args[0] /* addr */, args[1] /* len */,
                                args[2] /* erase */);
        } else {
          resp = 0x41;
        }
        break;
      }
      case CMD_FLASH_READ: {
        len = SLIP_recv(args, sizeof(args));
        if (len == 12) {
          resp = do_flash_read(args[0] /* addr */, args[1], /* len */
                               args[2] /* block_size */);
        } else {
          resp = 0x51;
        }
        break;
      }
      case CMD_FLASH_DIGEST: {
        len = SLIP_recv(args, sizeof(args));
        if (len == 12) {
          resp = do_flash_digest(args[0] /* addr */, args[1], /* len */
                                 args[2] /* digest_block_size */);
        } else {
          resp = 0x61;
        }
        break;
      }
      case CMD_FLASH_READ_CHIP_ID: {
        resp = do_flash_read_chip_id();
        break;
      }
      case CMD_REBOOT: {
        resp = 0;
        break;
      }
    }
    SLIP_send(&resp, 1);
  }
}

void stub_main() {
  uint32_t baud_rate = params[0];
  uint32_t greeting = 0x4941484f; /* OHAI */

  ets_set_user_start(NULL);

  /* Selects SPI functions for flash pins. */
  SelectSpiFunction();

  if (baud_rate > 0) {
    ets_delay_us(1000);
    uart_div_modify(0, UART_CLKDIV_26MHZ(baud_rate));
    ets_delay_us(10000);
  }

  SLIP_send(&greeting, 4);

  cmd_loop();

  ets_delay_us(10000);
  _ResetVector();
  /* Not reached */
}
