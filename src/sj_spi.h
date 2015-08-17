#ifndef SJ_SPI_INCLUDED
#define SJ_SPI_INCLUDED

#include <stdint.h>
#include <v7.h>

typedef void *spi_connection;

/* Initialize SPI */
int spi_init(spi_connection conn);

/*
 * Perform SPI transaction
 * Parameters:
 * conn - SPI connection
 * cmd_bits - actual number of bits to transmit
 * cmd_data - command data
 * addr_bits - actual number of bits to transmit
 * addr_data - address data
 * dout_bits - actual number of bits to transmit
 * dout_data - output data
 * din_bits - actual number of bits to receive
 *
 * Returns: read data - uint32_t containing read in data
 * only if RX was set
 * 0 - something went wrong (or actual read data was 0)
 * 1 - data sent ok (or actual read data is 1)
 * Note: all data is assumed to be stored in the lower
 * its of the data variables (for anything <32 bits).
 */
uint32_t spi_txn(spi_connection conn, uint8_t cmd_bits, uint16_t cmd_data,
                 uint8_t addr_bits, uint32_t addr_data, uint8_t dout_bits,
                 uint32_t dout_data, uint8_t din_bits, uint8_t dummy_bits);

/* Create SPI connection */
spi_connection sj_spi_create(struct v7 *v7, v7_val_t args);

/* Close SPI connection and free resources */
void sj_spi_close(spi_connection conn);

#endif
