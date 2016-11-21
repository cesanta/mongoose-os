/**
 * \file
 * \brief ATXMEGA's ATCA Hardware abstraction layer for SWI interface over UART drivers.
 *
 * Prerequisite: add UART Polled support to application in Atmel Studio
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
#include "swi_uart_samd21_asf.h"
#include "basic/atca_helpers.h"

/** \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief
 * These methods define the hardware abstraction layer for communicating with a CryptoAuth device
 *
   @{ */

/** \brief usart configuration struct */
static struct usart_config config_usart;
struct port_config pin_conf; //for DEBUG purpose

/** \brief Implementation of SWI UART init.
 * \param[in] instance  instance
 * \return ATCA_STATUS
 */
ATCA_STATUS swi_uart_init(ATCASWIMaster_t *instance)
{
#ifdef DEBUG_PIN
	port_get_config_defaults(&pin_conf);
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(DEBUG_PIN_1, &pin_conf);
	port_pin_set_config(DEBUG_PIN_2, &pin_conf);
	port_pin_set_output_level(DEBUG_PIN_1, HIGH);
	port_pin_set_output_level(DEBUG_PIN_2, HIGH);
#endif

	ATCA_STATUS status = ATCA_SUCCESS;
	usart_get_config_defaults(&config_usart);
	// Set data size to 7
	config_usart.character_size = USART_CHARACTER_SIZE_7BIT;
	// Set parity to no parity
	config_usart.parity = USART_PARITY_NONE;
	// Set data byte to one stopbit
	config_usart.stopbits = USART_STOPBITS_1;
	// Set baudrate to 230400
	config_usart.baudrate = 230400;
#ifdef __SAMD21J18A__
	config_usart.mux_setting = EXT3_UART_SERCOM_MUX_SETTING;
	config_usart.pinmux_pad0 = EXT3_UART_SERCOM_PINMUX_PAD0;
	config_usart.pinmux_pad1 = EXT3_UART_SERCOM_PINMUX_PAD1;
	config_usart.pinmux_pad2 = EXT3_UART_SERCOM_PINMUX_PAD2;
	config_usart.pinmux_pad3 = EXT3_UART_SERCOM_PINMUX_PAD3;
#endif
	switch (instance->bus_index) {
	case 0: status = usart_init( &(instance->usart_instance), SERCOM0, &config_usart); break;
	case 1: status = usart_init( &(instance->usart_instance), SERCOM1, &config_usart); break;
	case 2: status = usart_init( &(instance->usart_instance), SERCOM2, &config_usart); break;
	case 3: status = usart_init( &(instance->usart_instance), SERCOM3, &config_usart); break;
	case 4: status = usart_init( &(instance->usart_instance), SERCOM4, &config_usart); break;
	case 5: status = usart_init( &(instance->usart_instance), SERCOM5, &config_usart); break;
	}
	usart_enable(&(instance->usart_instance));
	return status;
}

/** \brief Implementation of SWI UART deinit.
 * \param[in] instance  instance
 * \return ATCA_STATUS
 */
ATCA_STATUS swi_uart_deinit(ATCASWIMaster_t *instance)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	usart_reset(&(instance->usart_instance));
	return status;
}

/** \brief implementation of SWI UART change baudrate.
 * \param[in] instance  instance
 * \param[in] baudrate (typically 230400 , 160000 or 115200)
 */
void swi_uart_setbaud(ATCASWIMaster_t *instance, uint32_t baudrate)
{
	// Disable UART module
	usart_disable(&(instance->usart_instance));
	// Set baudrate for UART module
	config_usart.baudrate    = baudrate;
	switch (instance->bus_index) {
	case 0: usart_init( &(instance->usart_instance), SERCOM0, &config_usart); break;
	case 1: usart_init( &(instance->usart_instance), SERCOM1, &config_usart); break;
	case 2: usart_init( &(instance->usart_instance), SERCOM2, &config_usart); break;
	case 3: usart_init( &(instance->usart_instance), SERCOM3, &config_usart); break;
	case 4: usart_init( &(instance->usart_instance), SERCOM4, &config_usart); break;
	case 5: usart_init( &(instance->usart_instance), SERCOM5, &config_usart); break;
	}
	usart_enable(&(instance->usart_instance));
}


/** \brief implementation of SWI UART change mode.
 * \param[in] instance  instance
 * \param[in] mode (TRANSMIT_MODE or RECEIVE_MODE)
 */
void swi_uart_mode(ATCASWIMaster_t *instance, uint8_t mode)
{
	usart_disable(&(instance->usart_instance));
	// Do nothing as the available time is not enough for setting the baudrate in ASF
	usart_enable(&(instance->usart_instance));
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
	int8_t retries = 90;
	uint16_t bitdata = (uint16_t)data;

	// Send one byte over UART module
#ifdef DEBUG_PIN
	port_pin_toggle_output_level(DEBUG_PIN_1);
#endif

	while ((retries > 0) && (usart_write_wait(&(instance->usart_instance), bitdata) != STATUS_OK))
		retries--;

#ifdef DEBUG_PIN
	port_pin_toggle_output_level(DEBUG_PIN_1);
#endif

	if (retries <= 0x00)
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
	uint8_t retries = 100; // This retry is used to compensate the baudrate that do not match when receiving response
	uint16_t bitdata = 0x00;

#ifdef DEBUG_PIN
	port_pin_toggle_output_level(DEBUG_PIN_2);
#endif
	// Receive one byte over UART module
	while ((retries > 0) && (usart_read_wait(&(instance->usart_instance), &bitdata) != STATUS_OK))
		retries--;

#ifdef DEBUG_PIN
	port_pin_toggle_output_level(DEBUG_PIN_2);
#endif

	*data = bitdata;
	if (retries <= 0x00)
		return ATCA_TIMEOUT;
	else
		return ATCA_SUCCESS;
}

/** @} */
