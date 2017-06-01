/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
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

#include "stub_flasher.h"

#include <stdint.h>
#include <string.h>

#include "rom_functions.h"

#if defined(ESP8266)
#include "eagle_soc.h"
#include "ets_sys.h"
#elif defined(ESP32)
#include "soc/uart_reg.h"
#endif

#include "slip.h"
#include "uart.h"

/* Param: baud rate. */
uint32_t params[1] __attribute__((section(".params")));

#define FLASH_BLOCK_SIZE 65536
#define FLASH_SECTOR_SIZE 4096
#define FLASH_PAGE_SIZE 256

/* These consts should be in sync with flasher_client.go */
#define UART_BUF_SIZE (8 * FLASH_SECTOR_SIZE)
#define FLASH_WRITE_SIZE FLASH_SECTOR_SIZE

#define UART_RX_INTS (UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA)

extern uint32_t _bss_start, _bss_end;

#ifdef ESP8266
#define REG_SPI_BASE(i) (0x60000200 - i * 0x100)

#define SPI_CMD_REG(i) (REG_SPI_BASE(i) + 0x0)
#define SPI_FLASH_RDID (BIT(28))

#define SPI_W0_REG(i) (REG_SPI_BASE(i) + 0x40)
#endif

int do_flash_erase(uint32_t addr, uint32_t len) {
  if (addr % FLASH_SECTOR_SIZE != 0) return 0x32;
  if (len % FLASH_SECTOR_SIZE != 0) return 0x33;
  if (esp_rom_spiflash_unlock() != 0) return 0x34;

  while (len > 0 && (addr % FLASH_BLOCK_SIZE != 0)) {
    if (esp_rom_spiflash_erase_sector(addr / FLASH_SECTOR_SIZE) != 0)
      return 0x35;
    len -= FLASH_SECTOR_SIZE;
    addr += FLASH_SECTOR_SIZE;
  }

  while (len > FLASH_BLOCK_SIZE) {
    if (esp_rom_spiflash_erase_block(addr / FLASH_BLOCK_SIZE) != 0) return 0x36;
    len -= FLASH_BLOCK_SIZE;
    addr += FLASH_BLOCK_SIZE;
  }

  while (len > 0) {
    if (esp_rom_spiflash_erase_sector(addr / FLASH_SECTOR_SIZE) != 0)
      return 0x37;
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

uint32_t ccount(void) {
  uint32_t r;
  __asm volatile("rsr.ccount %0" : "=a"(r));
  return r;
}

struct write_progress {
  uint32_t num_written;
  uint32_t buf_level;
};

struct write_result {
  uint32_t wait_time;
  uint32_t write_time;
  uint32_t erase_time;
  uint32_t total_time;
  uint8_t digest[16];
};

static struct uart_buf ub;

void uart_isr(void *arg) {
  uint32_t int_st = READ_PERI_REG(UART_INT_ST_REG(0));
  uint8_t fifo_len;
  while ((fifo_len = READ_PERI_REG(UART_STATUS_REG(0))) > 0 &&
         ub.nr < UART_BUF_SIZE) {
    while (fifo_len-- > 0 && ub.nr < UART_BUF_SIZE) {
      uint8_t byte = READ_PERI_REG(UART_FIFO_REG(0));
      *ub.pw++ = byte;
      ub.nr++;
      if (ub.pw >= ub.data + UART_BUF_SIZE) ub.pw = ub.data;
    }
  }
  WRITE_PERI_REG(UART_INT_CLR_REG(0), int_st);
  (void) arg;
}

int do_flash_write(uint32_t addr, uint32_t len, uint32_t erase) {
  uint32_t num_erased = 0;
  struct MD5Context ctx;
  MD5Init(&ctx);

  if (addr % FLASH_SECTOR_SIZE != 0) return 0x32;
  if (len % FLASH_SECTOR_SIZE != 0) return 0x33;
  if (esp_rom_spiflash_unlock() != 0) return 0x34;

  ub.nr = 0;
  ub.pr = ub.pw = ub.data;
  ets_isr_attach(ETS_UART0_INUM, uart_isr, &ub);
  uint32_t saved_conf1 = READ_PERI_REG(UART_CONF1_REG(0));
  /* Reduce frequency of UART interrupts */
  WRITE_PERI_REG(UART_CONF1_REG(0), UART_RX_TOUT_EN |
                                        (20 << UART_RX_TOUT_THRHD_S) |
                                        (100 << UART_RXFIFO_FULL_THRHD_S));
  SET_PERI_REG_MASK(UART_INT_ENA_REG(0), UART_RX_INTS);
  ets_isr_unmask(1 << ETS_UART0_INUM);

  struct write_result wr;
  memset(&wr, 0, sizeof(wr));

  struct write_progress wp = {.num_written = 0, .buf_level = ub.nr};
  SLIP_send(&wp, sizeof(wp));
  wr.total_time = ccount();
  while (wp.num_written < len) {
    volatile uint32_t *nr = &ub.nr;
    /* Prepare the space ahead. */
    uint32_t start_count = ccount();
    while (erase && num_erased < wp.num_written + FLASH_WRITE_SIZE) {
      const uint32_t num_left = (len - num_erased);
      if (num_left >= FLASH_BLOCK_SIZE && addr % FLASH_BLOCK_SIZE == 0) {
        if (esp_rom_spiflash_erase_block(addr / FLASH_BLOCK_SIZE) != 0)
          return 0x35;
        num_erased += FLASH_BLOCK_SIZE;
      } else {
        /* len % FLASH_SECTOR_SIZE == 0 is enforced, no further checks needed */
        if (esp_rom_spiflash_erase_sector(addr / FLASH_SECTOR_SIZE) != 0)
          return 0x36;
        num_erased += FLASH_SECTOR_SIZE;
      }
    }
    wr.erase_time += ccount() - start_count;
    start_count = ccount();
    /* Wait for data to arrive. */
    wp.buf_level = *nr;
    while (*nr < FLASH_WRITE_SIZE) {
    }
    wr.wait_time += ccount() - start_count;
    MD5Update(&ctx, ub.pr, FLASH_WRITE_SIZE);
    start_count = ccount();
    if (esp_rom_spiflash_write(addr, (uint32_t *) ub.pr, FLASH_WRITE_SIZE) != 0)
      return 0x37;
    wr.write_time += ccount() - start_count;
    ets_intr_lock();
    *nr -= FLASH_WRITE_SIZE;
    ets_intr_unlock();
    addr += FLASH_WRITE_SIZE;
    ub.pr += FLASH_WRITE_SIZE;
    if (ub.pr >= ub.data + UART_BUF_SIZE) ub.pr = ub.data;
    wp.num_written += FLASH_WRITE_SIZE;
    SLIP_send(&wp, sizeof(wp));
  }

  ets_isr_mask(1 << ETS_UART0_INUM);
  WRITE_PERI_REG(UART_CONF1_REG(0), saved_conf1);
  MD5Final(wr.digest, &ctx);

  wr.total_time = ccount() - wr.total_time;
  SLIP_send(&wr, sizeof(wr));

  return 0;
}

int do_flash_read(uint32_t addr, uint32_t len, uint32_t block_size,
                  uint32_t max_in_flight) {
  uint8_t buf[FLASH_SECTOR_SIZE];
  uint8_t digest[16];
  struct MD5Context ctx;
  uint32_t num_sent = 0, num_acked = 0;
  if (block_size > sizeof(buf)) return 0x52;
  MD5Init(&ctx);
  while (num_acked < len) {
    while (num_sent < len && num_sent - num_acked < max_in_flight) {
      uint32_t n = len - num_sent;
      if (n > block_size) n = block_size;
      if (esp_rom_spiflash_read(addr, (uint32_t *) buf, n) != 0) return 0x53;
      send_packet(buf, n);
      MD5Update(&ctx, buf, n);
      addr += n;
      num_sent += n;
    }
    {
      if (SLIP_recv(&num_acked, sizeof(num_acked)) != 4) return 0x54;
      if (num_acked > num_sent) return 0x55;
    }
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
    if (esp_rom_spiflash_read(addr, (uint32_t *) buf, n) != 0) return 0x63;
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

int do_flash_read_chip_id(void) {
  uint32_t chip_id = 0;
  WRITE_PERI_REG(SPI_CMD_REG(0), SPI_FLASH_RDID);
  while (READ_PERI_REG(SPI_CMD_REG(0)) & SPI_FLASH_RDID) {
  }
  chip_id = READ_PERI_REG(SPI_W0_REG(0)) & 0xFFFFFF;
  send_packet((uint8_t *) &chip_id, sizeof(chip_id));
  return 0;
}

uint8_t cmd_loop(void) {
  uint8_t cmd;
  do {
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
        if (len == 16) {
          resp = do_flash_read(args[0] /* addr */, args[1], /* len */
                               args[2] /* block_size */,
                               args[3] /* max_in_flight */);
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
      case CMD_FLASH_ERASE_CHIP: {
        resp = esp_rom_spiflash_erase_chip();
        break;
      }
      case CMD_BOOT_FW:
      case CMD_REBOOT: {
        resp = 0;
        SLIP_send(&resp, 1);
        return cmd;
      }
    }
    SLIP_send(&resp, 1);
  } while (cmd != CMD_BOOT_FW && cmd != CMD_REBOOT);
  return cmd;
}

void stub_main(void) {
  uint32_t baud_rate = params[0];
  uint32_t greeting = 0x4941484f; /* OHAI */
  uint8_t last_cmd;

  /* This points at us right now, reset for next boot. */
  ets_set_user_start(0);

  memset(&_bss_start, 0, (&_bss_end - &_bss_start));

/* Selects SPI functions for flash pins. */
#if defined(ESP8266)
  SelectSpiFunction();
  SET_PERI_REG_MASK(0x3FF00014, 1); /* Switch to 160 MHz */
#elif defined(ESP32)
  esp_rom_spiflash_attach(0 /* ishspi */, 0 /* legacy */);
  /* Set flash to 40 MHz. Note: clkdiv _should_ be 2, but actual meausrement
   * shows that with clkdiv = 1 clock is indeed 40 MHz. */
  esp_rom_spiflash_config_clk(1, 1);
#endif

  esp_rom_spiflash_config_param(
      0 /* deviceId */, 16 * 1024 * 1024 /* chip_size */, FLASH_BLOCK_SIZE,
      FLASH_SECTOR_SIZE, FLASH_PAGE_SIZE, 0xffff /* status_mask */);

  if (baud_rate > 0) {
    ets_delay_us(10000);
    set_baud_rate(0, baud_rate);
  }

  /* Give host time to get ready too. */
  ets_delay_us(50000);

  SLIP_send(&greeting, 4);

  last_cmd = cmd_loop();

  ets_delay_us(10000);

  if (last_cmd == CMD_BOOT_FW) {
#if defined(ESP8266)
    /*
     * Find the return address in our own stack and change it.
     * "flash_finish" it gets to the same point, except it doesn't need to
     * patch up its RA: it returns from UartDwnLdProc, then from f_400011ac,
     * then jumps to 0x4000108a, then checks strapping bits again (which will
     * not have changed), and then proceeds to 0x400010a8.
     */
    volatile uint32_t *sp = &baud_rate;
    while (*sp != (uint32_t) 0x40001100) sp++;
    *sp = 0x400010a8;
    /*
     * The following dummy asm fragment acts as a barrier, to make sure function
     * epilogue, including return address loading, is added after our stack
     * patching.
     */
    __asm volatile("nop.n");
    return; /* To 0x400010a8 */
#elif defined(ESP32)
/* TODO(rojer) */
#endif
  } else {
    software_reset();
  }
  /* Not reached */
}
