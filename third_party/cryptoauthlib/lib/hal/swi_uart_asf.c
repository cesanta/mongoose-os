/* swi_uart_asf.h.c
*
* \file
*
* \brief  Atmel Crypto Auth hardware interface object
*
* Copyright (c) 2015 Atmel Corporation. All rights reserved.
*
* \asf_license_start
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
*    Atmel microcontroller product.
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
* \asf_license_stop
*/

#include <stdlib.h>
#include <stdio.h>
#include "swi_uart_asf.h"
#include "basic/atca_helpers.h"

/** \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief
 * These methods define the hardware abstraction layer for communicating with a CryptoAuth device
 *
@{ */

/** \brief usart configuration struct */
static struct usart_config config_usart;

/** \brief Implementation of SWI UART init.
 * \param[in] ATCASWIMaster_t instance
 * \return ATCA_STATUS
 */
ATCA_STATUS swi_uart_init(ATCASWIMaster_t *instance)
{
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
	switch(instance->bus_index) {
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
 * \param[in] ATCASWIMaster_t instance
 * \return ATCA_STATUS
 */
ATCA_STATUS swi_uart_deinit(ATCASWIMaster_t *instance)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	usart_reset(&(instance->usart_instance));
	return status;
}

/** \brief implementation of SWI UART change baudrate.
 * \param[in] ATCASWIMaster_t instance
 * \param[in] baudrate (typically 230400 or 115200)
 * \return ATCA_STATUS
 */
void swi_uart_setbaud (ATCASWIMaster_t *instance, uint32_t baudrate){
	// Disable UART module
	usart_disable(&(instance->usart_instance));
	// Set baudrate for UART module
	config_usart.baudrate    = baudrate;
	switch(instance->bus_index) {
		case 0: usart_init( &(instance->usart_instance), SERCOM0, &config_usart); break;
		case 1: usart_init( &(instance->usart_instance), SERCOM1, &config_usart); break;
		case 2: usart_init( &(instance->usart_instance), SERCOM2, &config_usart); break;
		case 3: usart_init( &(instance->usart_instance), SERCOM3, &config_usart); break;
		case 4: usart_init( &(instance->usart_instance), SERCOM4, &config_usart); break;
		case 5: usart_init( &(instance->usart_instance), SERCOM5, &config_usart); break;
	}
	usart_enable(&(instance->usart_instance));
}

/** \brief HAL implementation of SWI UART send byte over ASF.  This function send one byte over UART
 * \param[in] ATCASWIMaster_t instance
 * \param[in] txdata pointer to bytes to send
 * \return ATCA_STATUS
 */
ATCA_STATUS swi_uart_send_byte(ATCASWIMaster_t *instance, uint16_t data)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	// Send one byte over UART module
	status = usart_write_wait(&(instance->usart_instance), data);
	return status;
}

/** \brief HAL implementation of SWI UART receive bytes over ASF.  This function receive one byte over UART
 * \param[in] ATCASWIMaster_t instance
 * \param[inout] rxdata pointer to space to receive the data
 * \return ATCA_STATUS
 */
ATCA_STATUS swi_uart_receive_byte(ATCASWIMaster_t *instance, uint16_t *data)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	// Receive one byte over UART module
	while (usart_read_wait(&(instance->usart_instance), data) != STATUS_OK);
	return status;
}

/** @} */
