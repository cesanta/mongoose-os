/**
 * \file
 *
 * \brief  Atmel Crypto Auth hardware interface object
 *
 * Copyright (c) 2015 Atmel Corporation. All rights reserved.
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

#include <stdlib.h>
#include <stdio.h>
#include "swi_uart_samd21_start.h"
#include "basic/atca_helpers.h"

/** \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief
 * These methods define the hardware abstraction layer for communicating with a CryptoAuth device
 *
   @{ */

/** \brief HAL implementation of SWI UART init.
 * \param[in] instance  instance
 * \return ATCA_STATUS
 */
ATCA_STATUS swi_uart_init(ATCASWIMaster_t *instance)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	instance->USART_SWI = USART_1;

	status |= usart_sync_set_mode(&(instance->USART_SWI), USART_MODE_ASYNCHRONOUS);
	// Set data size to 7
	status |= usart_sync_set_character_size(&(instance->USART_SWI), USART_CHARACTER_SIZE_7BITS);
	// Set parity to no parity
	status |= usart_sync_set_parity(&(instance->USART_SWI), USART_PARITY_NONE);
	// Set data byte to one stopbit
	status |= usart_sync_set_stopbits(&(instance->USART_SWI), USART_STOP_BITS_ONE);
	// Set baudrate to 230400
	status |= usart_sync_set_baud_rate(&(instance->USART_SWI), 230400);
	// Enable SWI UART
	status |= usart_sync_enable(&(instance->USART_SWI));
	return status;
}

/** \brief HAL implementation of SWI UART deinit.
 * \param[in] instance  instance
 * \return ATCA_STATUS
 */
ATCA_STATUS swi_uart_deinit(ATCASWIMaster_t *instance)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	usart_sync_disable(&(instance->USART_SWI));

	return status;
}

/** \brief HAL implementation of SWI UART change baudrate.
 * \param[in] instance  instance
 * \param[in] baudrate (typically 230400 or 115200)
 */
void swi_uart_setbaud(ATCASWIMaster_t *instance, uint32_t baudrate)
{
	// Set baudrate for UART module
	delay_us(50);
	usart_sync_set_baud_rate(&(instance->USART_SWI), baudrate);

}


/** \brief HAL implementation of SWI UART change mode.
 * \param[in] instance  instance
 * \param[in] mode (TRANSMIT_MODE or RECEIVE_MODE)
 */
void swi_uart_mode(ATCASWIMaster_t *instance, uint8_t mode)
{

	usart_sync_disable(&(instance->USART_SWI));

	if (mode == TRANSMIT_MODE) {
		// Set baudrate to 230400
		usart_sync_set_baud_rate(&(instance->USART_SWI), 230400);
		// Disable Receiver
		hri_sercomusart_clear_CTRLB_RXEN_bit(&(instance->USART_SWI.device.hw));
		hri_sercomusart_set_CTRLB_TXEN_bit(&(instance->USART_SWI.device.hw));
	}else if (mode == RECEIVE_MODE) {
		// Set baudrate to 160000
		usart_sync_set_baud_rate(&(instance->USART_SWI), 160000);
		// Enable Receiver
		hri_sercomusart_clear_CTRLB_TXEN_bit(&(instance->USART_SWI.device.hw));
		hri_sercomusart_set_CTRLB_RXEN_bit(&(instance->USART_SWI.device.hw));
	}
	usart_sync_enable(&(instance->USART_SWI));
	instance->bus_index &= 0x07;
}

/** \brief discover UART buses available for this hardware
 * this maintains a list of logical to physical bus mappings freeing the application
 * of the a-priori knowledge
 * \param[in] swi_uart_buses - an array of logical bus numbers
 * \param[in] max_buses - maximum number of buses the app wants to attempt to discover
 */
void swi_uart_discover_buses(int swi_uart_buses[], int max_buses)
{
	/* if every SERCOM was a likely candidate bus, then would need to initialize the entire array to all SERCOM n numbers.
	 * As an optimization and making discovery safer, make assumptions about bus-num / SERCOM map based on SAMD21 Xplained Pro board
	 * If you were using a raw SAMD21 on your own board, you would supply your own bus numbers based on your particular hardware configuration.
	 */
	swi_uart_buses[0] = 4;   // default samd21 for xplained dev board
}

/** \brief HAL implementation of SWI UART send byte over ASF.  This function send one byte over UART
 * \param[in] instance  instance
 * \param[in] data      byte to send
 * \return ATCA_STATUS
 */
ATCA_STATUS swi_uart_send_byte(ATCASWIMaster_t *instance, uint8_t data)
{
	uint8_t retries = 2;
	int32_t byte_sent = 0;

#ifdef DEBUG_PIN
	gpio_toggle_pin_level(PA20);
#endif
	// Send one byte over UART module
	while ((retries > 0) && (byte_sent < 1)) {
		byte_sent = io_write(&(instance->USART_SWI.io), &data, 1);
		retries--;
	}
#ifdef DEBUG_PIN
	gpio_toggle_pin_level(PA20);
#endif
	if (byte_sent <= 0)
		return ATCA_TIMEOUT;
	else
		return ATCA_SUCCESS;
}

/** \brief HAL implementation of SWI UART receive bytes over ASF.  This function receive one byte over UART
 * \param[in]    instance instance
 * \param[inout] data     pointer to space to receive the data
 * \return ATCA_STATUS
 */
ATCA_STATUS swi_uart_receive_byte(ATCASWIMaster_t *instance, uint8_t *data)
{
	int32_t byte_sent = 0;
	uint8_t retries = 2;

#ifdef DEBUG_PIN
	gpio_toggle_pin_level(PA21);
#endif
	// Receive one byte over UART module
	while ((retries > 0) && (byte_sent < 1)) {
		byte_sent = io_read(&(instance->USART_SWI.io), data, 1);
		retries--;
	}
#ifdef DEBUG_PIN
	gpio_toggle_pin_level(PA21);
#endif
	if (byte_sent <= 0)
		return ATCA_TIMEOUT;
	else
		return ATCA_SUCCESS;
}

/** @} */
