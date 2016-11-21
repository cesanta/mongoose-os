/**
 * \file
 * \brief  definitions for bit-banged I2C
 * \author Atmel Crypto Group
 * \date   November 18, 2015
 *
 * \copyright Copyright (c) 2015 Atmel Corporation. All rights reserved.
 *
 * \atmel_crypto_device_library_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel integrated circuit.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \atmel_crypto_device_library_license_stop
 */

#ifndef I2C_BITBANG_AT88CK9000_H_
#define I2C_BITBANG_AT88CK9000_H_

#include "atca_status.h"
#include "timer_utilities.h"


#define MAX_I2C_BUSES   11      //!< AT88CK9000 has 11 sets of GPIO pins dedicated for I2C (including 1 for on-board SHA204A)


typedef struct {
	uint32_t pin_sda[MAX_I2C_BUSES];
	uint32_t pin_scl[MAX_I2C_BUSES];
} I2CBuses;

extern I2CBuses i2c_buses_default;

extern uint32_t pin_sda;
extern uint32_t pin_scl;


#define I2C_ENABLE()             pio_configure_pin(pin_sda, PIO_OUTPUT_0); pio_configure_pin(pin_scl, PIO_OUTPUT_0)
#define I2C_DISABLE()            pio_configure_pin(pin_sda, PIO_INPUT); pio_configure_pin(pin_scl, PIO_INPUT)
#define I2C_CLOCK_LOW()          pio_set_pin_low(pin_scl)
#define I2C_CLOCK_HIGH()         pio_set_pin_high(pin_scl)
#define I2C_DATA_LOW()           pio_set_pin_low(pin_sda)
#define I2C_DATA_HIGH()          pio_set_pin_high(pin_sda)
#define I2C_DATA_IN()            pio_get_pin_value(pin_sda)
#define I2C_SET_OUTPUT()         pio_configure_pin(pin_sda, PIO_OUTPUT_0)
#define I2C_SET_OUTPUT_HIGH()    I2C_SET_OUTPUT(); I2C_DATA_HIGH()
#define I2C_SET_OUTPUT_LOW()     I2C_SET_OUTPUT(); I2C_DATA_LOW()
#define I2C_SET_INPUT()          pio_configure_pin(pin_sda, PIO_INPUT)
#define DISABLE_INTERRUPT()      cpu_irq_disable()
#define ENABLE_INTERRUPT()       cpu_irq_enable()


#define I2C_CLOCK_DELAY_WRITE_LOW()  delay_us(1)
#define I2C_CLOCK_DELAY_WRITE_HIGH() delay_us(1)
#define I2C_CLOCK_DELAY_READ_LOW()   delay_us(1)
#define I2C_CLOCK_DELAY_READ_HIGH()  delay_us(1)
//! This delay plus I2C_CLOCK_HIGH and I2C_CLOCK_LOW takes about 1.3 us.
#define I2C_CLOCK_DELAY_SEND_ACK()   delay_us(1)
//! This delay is inserted to make the Start and Stop hold time at least 250 ns.
#define I2C_HOLD_DELAY()             i2c_delay()


//! loop count when waiting for an acknowledgment
#define I2C_ACK_TIMEOUT                 (4)


/**
 * \brief Set I2C data and clock pin.
 *        Other functions will use these pins.
 *
 * \param[in] sda  definition of GPIO pin to be used as data pin
 * \param[in] scl  definition of GPIO pin to be used as clock pin
 */
void i2c_set_pin(uint32_t sda, uint32_t scl);


/**
 * \brief Configure GPIO pins for I2C clock and data as output.
 */
void i2c_enable(void);

/**
 * \brief Configure GPIO pins for I2C clock and data as input.
 */
void i2c_disable(void);


/**
 * \brief Send a START condition.
 */
void i2c_send_start(void);

/**
 * \brief Send an ACK or NACK (after receive).
 *
 * \param[in] ack  0: NACK, else: ACK
 */
void i2c_send_ack(uint8_t ack);

/**
 * \brief Send a STOP condition.
 */
void i2c_send_stop(void);

/**
 * \brief Send a Wake Token.
 */
void i2c_send_wake_token(void);

/**
 * \brief Send one byte.
 *
 * \param[in] i2c_byte  byte to write
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS i2c_send_byte(uint8_t i2c_byte);

/**
 * \brief Send a number of bytes.
 *
 * \param[in] count  number of bytes to send
 * \param[in] data   pointer to buffer containing bytes to send
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS i2c_send_bytes(uint8_t count, uint8_t *data);

/**
 * \brief Receive one byte (MSB first).
 *
 * \param[in] ack  0:NACK, else:ACK
 *
 * \return byte received
 */
uint8_t i2c_receive_one_byte(uint8_t ack);

/**
 * \brief Receive one byte and send ACK.
 *
 * \param[out] data  pointer to received byte
 */
void i2c_receive_byte(uint8_t *data);

/**
 * \brief Receive a number of bytes.
 *
 * \param[out] data   pointer to receive buffer
 * \param[in]  count  number of bytes to receive
 */
void i2c_receive_bytes(uint8_t count, uint8_t *data);

#endif /* I2C_BITBANG_AT88CK9000_H_ */