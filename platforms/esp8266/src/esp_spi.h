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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP_SPI_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP_SPI_H_

/*
 * SPI low level API
 *
 * Example:
 *
 * -- Initialilzation --
 * Initialization with default paramaters: spi_init(HSPI);
 *
 * Advanced initialization:
 *  --- Put HSPI GPIOs in spi mode
 *  spi_init_gpio(HSPI, SPI_CLK_USE_DIV);
 *  --- Setup bus speed
 *  spi_clock(HSPI, SPI_CLK_PREDIV, SPI_CLK_CNTDIV);
 *  --- Setup transmission byte order
 *  spi_tx_byte_order(HSPI, SPI_BYTE_ORDER_HIGH_TO_LOW);
 *  --- Setup receiving byte order
 *  spi_rx_byte_order(HSPI, SPI_BYTE_ORDER_HIGH_TO_LOW);
 *  --- Finishing initialization, this function has to be called always
 *  spi_finalize_init(HSPI);
 *
 * -- Writing data to EEPROM --
 *
 * Command = 0b101 (3 bit write command)
 * Address = 0b111110011 or 0x1F3 (9 bit data address)
 * Data = 0b11001100 or 0xCC (8 bits of data)
 * SPI Transaction Packet = 0b10111111001111001100 or 0xBF3CC
 * spi_txn(HSPI, 3, 0b101, 9, 0x1F3, 8, 0xCC, 0, 0);
 *
 * -- Reading data from EEPROM --
 * Command = 0b110 (3 bit read command)
 * Address = 0b111110011 or 0x1F3 (9 bit data address)
 * (8 bits of data to read)
 * SPI Transaction Packet = 0b11011111001100000000 or 0xDF300
 * data_read = (uint8_t) spi_txn(HSPI, 3, 0b101, 9, 0x1F3, 0, 0, 8, 0);
 *
 * -- Basic Transmit Functions --
 * Send 'n' bits of data: spi_txd(spi_no, bits, data)
 * Send 8 bits of data (1 BYTE): spi_tx8(spi_no, data)
 * Send 16 bits of data (1 WORD): spi_tx16(spi_no, data)
 * Send 32 bits of data (1 DWORD): spi_tx32(spi_no, data)
 *
 * -- Example --
 * uint8_t byte = 0xEF;
 * uint16_t word = 0xBEEF;
 * uint32_t dword = 0xDEADBEEF;
 * uint16_t 9bit = 0b101010101;
 * spi_tx8(HSPI, byte);
 * spi_tx16(HSPI, word);
 * spi_tx32(HSPI, dword);
 * spi_txd(HSPI, 9, 9bit);
 *
 * -- Basic Receive Functions --
 * Receive 'n' bits of data: spi_rxd(spi_no, bits)
 * Receive 8 bits of data (1 BYTE): spi_rx8(spi_no)
 * Receive 16 bits of data (1 WORD): spi_rx16(spi_no)
 * Receive 32 bits of data (1 DWORD): spi_rx32(spi_no)
 *
 * -- Example --
 *
 * uint8_t byte;
 * uint16_t word;
 * uint32_t dword;
 * uint16_t 9bit;
 * byte = spi_rx8(HSPI);
 * word = spi_rx16(HSPI);
 * dword = spi_rx32(HSPI);
 * 9bit = (uint16_t) spi_rxd(HSPI, 9); //returned value is uint32_t. Cast to
 *uint16_t
 *
 * Also see spi.c, test_MPL115A1() function for
 * full-featured example
 */

#include <ets_sys.h>
#include "esp_missing_includes.h"

#include <os_type.h>
#include <osapi.h>

#include "mgos_spi.h"

#include "esp_spi_register.h"

#define SPI 0
#define HSPI 1

#define SPI_CLK_USE_DIV 0
#define SPI_CLK_80MHZ_NODIV 1

#define SPI_BYTE_ORDER_HIGH_TO_LOW 1
#define SPI_BYTE_ORDER_LOW_TO_HIGH 0

#ifndef CPU_CLK_FREQ /* Should already be defined in eagle_soc.h */
#define CPU_CLK_FREQ 80 * 1000000
#endif

/* Define some default SPI clock settings */
#define SPI_CLK_PREDIV 10
#define SPI_CLK_CNTDIV 2
#define SPI_CLK_FREQ \
  CPU_CLK_FREQ / (SPI_CLK_PREDIV * SPI_CLK_CNTDIV) /* 80 / 20 = 4 MHz */

/*
 * Function Name: spi_finalize_init
 *   Description: Finishes initializarion and get
 *   bus ready to work
 */
void spi_finalize_init(uint8_t spi_no);

/*
 * Function Name: spi_init_gpio
 *   Description: Initialises the GPIO pins for use as SPI pins.
 *    Parameters:
 *     spi_no - SPI (0) or HSPI (1)
 *     sysclk_as_spiclk - SPI_CLK_80MHZ_NODIV (1) if
 *     using 80MHz sysclock for SPI clock.
 *     SPI_CLK_USE_DIV (0) if using
 *     divider to get lower SPI clock
 *     speed.
 */
void spi_init_gpio(uint8_t spi_no, uint8_t sysclk_as_spiclk);

/*
 * Function Name: spi_clock
 *   Description: sets up the control registers for the SPI clock
 *    Parameters: spi_no - SPI (0) or HSPI (1)
 *    prediv - predivider value (actual division value)
 *    cntdiv - postdivider value (actual division value)
 *    Set either divider to 0 to disable all division
 *    (80MHz sysclock)
 */
void spi_clock(uint8_t spi_no, uint16_t prediv, uint8_t cntdiv);

/*
 * Function Name: spi_tx_byte_order
 *   Description: Setup the byte order for shifting data out of buffer
 *    Parameters: spi_no - SPI (0) or HSPI (1)
 *    byte_order - SPI_BYTE_ORDER_HIGH_TO_LOW (1)
 *    Data is sent out starting with Bit31 and down to Bit0
 *    SPI_BYTE_ORDER_LOW_TO_HIGH (0)
 *    Data is sent out starting with the lowest BYTE, from
 *    MSB to LSB, followed by the second lowest BYTE, from
 *    MSB to LSB, followed by the second highest BYTE, from
 *    MSB to LSB, followed by the highest BYTE, from MSB to LSB
 *    0xABCDEFGH would be sent as 0xGHEFCDAB
 */
void spi_tx_byte_order(uint8_t spi_no, uint8_t byte_order);

/*
 * Function Name: spi_rx_byte_order
 *   Description: Setup the byte order for shifting data into buffer
 *    Parameters: spi_no - SPI (0) or HSPI (1)
 *    byte_order - SPI_BYTE_ORDER_HIGH_TO_LOW (1)
 *    Data is read in starting with Bit31 and down to Bit0
 *    SPI_BYTE_ORDER_LOW_TO_HIGH (0)
 *    Data is read in starting with the lowest BYTE, from
 *    MSB to LSB, followed by the second lowest BYTE, from
 *    MSB to LSB, followed by the second highest BYTE, from
 *    MSB to LSB, followed by the highest BYTE, from MSB to LSB
 *    0xABCDEFGH would be read as 0xGHEFCDAB
 */
void spi_rx_byte_order(uint8_t spi_no, uint8_t byte_order);

#define spi_busy(spi_no) READ_PERI_REG(SPI_CMD(spi_no)) & SPI_USR

#define spi_txd(spi_no, bits, data) \
  spi_txn(spi_no, 0, 0, 0, 0, bits, (uint32_t) data, 0, 0)
#define spi_tx8(spi_no, data) \
  spi_txn(spi_no, 0, 0, 0, 0, 8, (uint32_t) data, 0, 0)
#define spi_tx16(spi_no, data) \
  spi_txn(spi_no, 0, 0, 0, 0, 16, (uint32_t) data, 0, 0)
#define spi_tx32(spi_no, data) \
  spi_txn(spi_no, 0, 0, 0, 0, 32, (uint32_t) data, 0, 0)

#define spi_rxd(spi_no, bits) spi_txn(spi_no, 0, 0, 0, 0, 0, 0, bits, 0)
#define spi_rx8(spi_no) (uint8_t) spi_txn(spi_no, 0, 0, 0, 0, 0, 0, 8, 0)
#define spi_rx16(spi_no) (uint16_t) spi_txn(spi_no, 0, 0, 0, 0, 0, 0, 16, 0)
#define spi_rx32(spi_no) (uint16_t) spi_txn(spi_no, 0, 0, 0, 0, 0, 0, 32, 0)

#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP_SPI_H_ */
