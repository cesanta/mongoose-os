/**
 * \file
 * \brief  Hardware Interface Functions - SWI bit-banged
 * \author Atmel Crypto Products
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

#include <asf.h>
#include <stdint.h>
#include "swi_bitbang_at88ck9000.h"


SWIBuses swi_buses_default = {
	{ SWI0, SWI1, SWI2, SWI3, SWI4, SWI5, SWI6, SWI7, SWI8, SWI9 }
};

//! declaration of the variable indicating which pin the selected device is connected to
static uint8_t device_pin;


void swi_set_pin(uint8_t id)
{
	device_pin = id;
}

void swi_enable(void)
{
	pio_configure_pin(device_pin, PIO_OUTPUT_1 | PIO_OPENDRAIN);
}

void swi_disable(void)
{
	pio_configure_pin(device_pin, PIO_INPUT | PIO_DEFAULT);
}


void swi_set_signal_pin(uint8_t is_high)
{
	if (is_high)
		pio_set_pin_high(device_pin);
	else
		pio_set_pin_low(device_pin);
}

void swi_send_wake_token(void)
{
	swi_set_signal_pin(0);
	delay_us(60);
	swi_set_signal_pin(1);
}

void swi_send_bytes(uint8_t count, uint8_t *buffer)
{
	uint8_t i, bit_mask;

	pio_configure_pin(device_pin, PIO_OUTPUT_1 | PIO_OPENDRAIN);

	//! Wait turn around time.
	RX_TX_DELAY;

	for (i = 0; i < count; i++) {
		for (bit_mask = 1; bit_mask > 0; bit_mask <<= 1) {
			if (bit_mask & buffer[i]) { //!< Send Logic 1 (7F)
				pio_set_pin_low(device_pin);
				BIT_DELAY_1L;
				pio_set_pin_high(device_pin);
				BIT_DELAY_7;
			}else {  //!< Send Logic 0 (7D)
				pio_set_pin_low(device_pin);
				BIT_DELAY_1L;
				pio_set_pin_high(device_pin);
				BIT_DELAY_1H;
				pio_set_pin_low(device_pin);
				BIT_DELAY_1L;
				pio_set_pin_high(device_pin);
				BIT_DELAY_5;
			}
		}
	}
}

void swi_send_byte(uint8_t byte)
{
	swi_send_bytes(1, &byte);
}

ATCA_STATUS swi_receive_bytes(uint8_t count, uint8_t *buffer)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	uint8_t i;
	uint8_t bit_mask;
	uint8_t pulse_count;
	uint8_t timeout_count;

	pio_configure_pin(device_pin, PIO_INPUT);

	//! Receive bits and store in buffer.
	for (i = 0; i < count; i++) {
		for (bit_mask = 1; bit_mask > 0; bit_mask <<= 1) {
			pulse_count = 0;

			//! Make sure that the variable below is big enough.
			//! Change it to uint16_t if 255 is too small, but be aware that
			//! the loop resolution decreases on an 8-bit controller in that case.
			timeout_count = START_PULSE_TIME_OUT;

			//! Detect start bit.
			while (--timeout_count > 0) {
				//! Wait for falling edge.
				if ( (pio_get_pin_value(device_pin)) == 0)
					break;
			}

			if (timeout_count == 0) {
				status = ATCA_RX_TIMEOUT;
				break;
			}

			do {
				//! Wait for rising edge.
				if ( (pio_get_pin_value(device_pin)) != 0) {
					//! For an Atmel microcontroller this might be faster than "pulse_count++".
					pulse_count = 1;
					break;
				}
			} while (--timeout_count > 0);

			if (pulse_count == 0) {
				status = ATCA_RX_TIMEOUT;
				break;
			}

			//! Trying to measure the time of start bit and calculating the timeout
			//! for zero bit detection is not accurate enough for an 8 MHz 8-bit CPU.
			//! So let's just wait the maximum time for the falling edge of a zero bit
			//! to arrive after we have detected the rising edge of the start bit.
			timeout_count = ZERO_PULSE_TIME_OUT;

			//! Detect possible edge indicating zero bit.
			do {
				if ( (pio_get_pin_value(device_pin)) == 0) {
					//! For an Atmel microcontroller this might be faster than "pulse_count++".
					pulse_count = 2;
					break;
				}
			} while (--timeout_count > 0);

			//! Wait for rising edge of zero pulse before returning. Otherwise we might interpret
			//! its rising edge as the next start pulse.
			if (pulse_count == 2) {
				timeout_count = ZERO_PULSE_TIME_OUT;
				do
					if ( (pio_get_pin_value(device_pin)) != 0)
						break;
				while (timeout_count-- > 0);
			}
			//! Update byte at current buffer index.
			else
				//! received "one" bit
				buffer[i] |= bit_mask;
		}

		if (status != ATCA_SUCCESS)
			break;
	}

	if (status == ATCA_RX_TIMEOUT) {
		if (i > 0)
			//! Indicate that we timed out after having received at least one byte.
			status = ATCA_RX_FAIL;
	}
	return status;
}