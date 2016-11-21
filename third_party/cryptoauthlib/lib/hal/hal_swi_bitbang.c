/**
 * \file
 * \brief ATCA Hardware abstraction layer for SWI bit banging.
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

#include <asf.h>
#include <string.h>
#include <stdio.h>
#include "atca_hal.h"
#include "hal_swi_bitbang.h"


/**
 * \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief These methods define the hardware abstraction layer for
 *        communicating with a CryptoAuth device using SWI bit banging.
   @{ */

/**
 * \brief Logical to physical bus mapping structure.
 */
ATCASWIMaster_t *swi_hal_data[MAX_SWI_BUSES];   //!< map logical, 0-based bus number to index
int swi_bus_ref_ct = 0;                         //!< total in-use count across buses


/**
 * \brief hal_swi_init manages requests to initialize a physical
 *        interface. It manages use counts so when an interface has
 *        released the physical layer, it will disable the interface for
 *        some other use. You can have multiple ATCAIFace instances using
 *        the same bus, and you can have multiple ATCAIFace instances on
 *        multiple swi buses, so hal_swi_init manages these things and
 *        ATCAIFace is abstracted from the physical details.
 */

/**
 * \brief Initialize an SWI interface using given config.
 *
 * \param[in] hal  opaque pointer to HAL data
 * \param[in] cfg  interface configuration
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_swi_init(void *hal, ATCAIfaceCfg *cfg)
{
	ATCAHAL_t *phal = (ATCAHAL_t*)hal;

	int bus = cfg->atcaswi.bus; //!< 0-based logical bus number

	if (swi_bus_ref_ct == 0)    //!< power up state, no swi buses will have been used

		for ( int i = 0; i < MAX_SWI_BUSES; i++ )
			swi_hal_data[i] = NULL;
	swi_bus_ref_ct++;   //!< total across buses

	if (bus >= 0 && bus < MAX_SWI_BUSES) {
		//! if this is the first time this bus and interface has been created, do the physical work of enabling it
		if ( swi_hal_data[bus] == NULL ) {
			swi_hal_data[bus] = malloc(sizeof(ATCASWIMaster_t));

			//! assign GPIO pin
			swi_hal_data[bus]->pin_sda = swi_buses_default.pin_sda[bus];

			swi_set_pin(swi_hal_data[bus]->pin_sda);
			swi_enable();

			//! store this for use during the release phase
			swi_hal_data[bus]->bus_index = bus;
		}else {
			//! otherwise, another interface already initialized the bus, any different
			//! cfg parameters will be ignored...first one to initialize this sets the configuration
		}

		phal->hal_data = swi_hal_data[bus];

		return ATCA_SUCCESS;
	}

	return ATCA_COMM_FAIL;
}

/**
 * \brief HAL implementation of SWI post init.
 *
 * \param[in] iface  ATCAIface instance
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_swi_post_init(ATCAIface iface)
{
	return ATCA_SUCCESS;
}

/**
 * \brief Send byte(s) via SWI.
 *
 * \param[in] iface     interface of the logical device to send data to
 * \param[in] txdata    pointer to bytes to send
 * \param[in] txlength  number of bytes to send
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_swi_send(ATCAIface iface, uint8_t *txdata, int txlength)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);

	int bus     = cfg->atcaswi.bus;

	//! Skip the Word Address data as SWI doesn't use it
	txdata++;

	//! Set SWI pin
	swi_set_pin(swi_hal_data[bus]->pin_sda);

	//! Send Command Flag
	swi_send_byte(SWI_FLAG_CMD);

	//! Send the remaining bytes
	swi_send_bytes(txlength, txdata);

	return ATCA_SUCCESS;
}

/**
 * \brief Receive byte(s) via SWI.
 *
 * \param[in] iface     interface of the logical device to receive data
 *                      from
 * \param[in] rxdata    pointer to where bytes will be received
 * \param[in] rxlength  pointer to expected number of receive bytes to
 *                      request
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_swi_receive(ATCAIface iface, uint8_t *rxdata, uint16_t *rxlength)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);

	ATCA_STATUS status = ATCA_RX_TIMEOUT;

	int bus     = cfg->atcaswi.bus;
	int retries = cfg->rx_retries;
	uint16_t count;

	//! Set SWI pin
	swi_set_pin(swi_hal_data[bus]->pin_sda);

	while (retries-- > 0 && status != ATCA_SUCCESS) {
		swi_send_byte(SWI_FLAG_TX);

		status = swi_receive_bytes(*rxlength, rxdata);
		if (status == ATCA_RX_FAIL) {
			count = rxdata[0];
			if ((count < ATCA_RSP_SIZE_MIN) || (count > *rxlength)) {
				status = ATCA_INVALID_SIZE;
				break;
			}else
				status = ATCA_SUCCESS;
		}else if (status == ATCA_RX_TIMEOUT)
			status = ATCA_RX_NO_RESPONSE;
	}

	return status;
}

/**
 * \brief Send Wake flag via SWI.
 *
 * \param[in] iface  interface of the logical device to wake up
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_swi_wake(ATCAIface iface)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);

	ATCA_STATUS status = ATCA_WAKE_FAILED;

	int bus     = cfg->atcaswi.bus;
	uint8_t response[4] = { 0x00, 0x00, 0x00, 0x00 };
	uint8_t expected_response[4] = { 0x04, 0x11, 0x33, 0x43 };

	//! Set SWI pin
	swi_set_pin(swi_hal_data[bus]->pin_sda);

	//! Generate Wake Token
	swi_send_wake_token();

	//! Wait tWHI + tWLO
	atca_delay_us(cfg->wake_delay);

	status = hal_swi_receive(iface, response, sizeof(response));
	if (status == ATCA_SUCCESS) {
		//! Compare response with expected_response
		if (memcmp(response, expected_response, 4) != 0)
			status = ATCA_WAKE_FAILED;
	}

	return status;
}

/**
 * \brief Send Idle flag via SWI.
 *
 * \param[in] iface  interface of the logical device to idle
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_swi_idle(ATCAIface iface)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);

	int bus     = cfg->atcaswi.bus;

	//! Set SWI pin
	swi_set_pin(swi_hal_data[bus]->pin_sda);

	swi_send_byte(SWI_FLAG_IDLE);

	return ATCA_SUCCESS;
}

/**
 * \brief Send Sleep flag via SWI.
 *
 * \param[in] iface  interface of the logical device to sleep
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_swi_sleep(ATCAIface iface)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);

	int bus     = cfg->atcaswi.bus;

	//! Set SWI pin
	swi_set_pin(swi_hal_data[bus]->pin_sda);

	swi_send_byte(SWI_FLAG_SLEEP);

	return ATCA_SUCCESS;
}

/**
 * \brief Manages reference count on given bus and releases resource if
 *        no more reference(s) exist.
 *
 * \param[in] hal_data  opaque pointer to hal data structure - known only
 *                      to the HAL implementation
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_swi_release(void *hal_data)
{
	ATCASWIMaster_t *hal = (ATCASWIMaster_t*)hal_data;

	swi_bus_ref_ct--;   //!< track total SWI instances

	//! if the use count for this bus has gone to 0 references, disable it.  protect against an unbracketed release
	if (hal && swi_hal_data[hal->bus_index] != NULL) {
		swi_set_pin(swi_hal_data[hal->bus_index]->pin_sda);
		swi_disable();
		free(swi_hal_data[hal->bus_index]);
		swi_hal_data[hal->bus_index] = NULL;
	}

	return ATCA_SUCCESS;
}

/** @} */