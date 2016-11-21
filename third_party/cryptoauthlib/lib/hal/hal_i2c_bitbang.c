/**
 * \file
 * \brief ATCA Hardware abstraction layer for I2C bit banging.
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
#include "hal_i2c_bitbang.h"


/**
 * \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief These methods define the hardware abstraction layer for
 *        communicating with a CryptoAuth device using I2C bit banging.
   @{ */

/**
 * \brief Logical to physical bus mapping structure.
 */
ATCAI2CMaster_t *i2c_hal_data[MAX_I2C_BUSES];   //!< map logical, 0-based bus number to index
int i2c_bus_ref_ct = 0;                         //!< total in-use count across buses


/**
 * \brief This function creates a Start condition and sends the I2C
 *        address.
 *
 * \param[in] RorW  I2C_READ for reading, I2C_WRITE for writing.
 *
 * \return ATCA_STATUS
 */
static ATCA_STATUS hal_i2c_send_slave_address(ATCAIface iface, uint8_t RorW)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);

	ATCA_STATUS status = ATCA_TX_TIMEOUT;

	uint8_t sla = cfg->atcai2c.slave_address | RorW;

	i2c_send_start();

	status = i2c_send_byte(sla);
	if (status != ATCA_SUCCESS)
		i2c_send_stop();

	return status;
}

/**
 * \brief hal_i2c_init manages requests to initialize a physical
 *        interface. It manages use counts so when an interface has
 *        released the physical layer, it will disable the interface for
 *        some other use. You can have multiple ATCAIFace instances using
 *        the same bus, and you can have multiple ATCAIFace instances on
 *        multiple i2c buses, so hal_i2c_init manages these things and
 *        ATCAIFace is abstracted from the physical details.
 */

/**
 * \brief Initialize an I2C interface using given config.
 *
 * \param[in] hal  opaque pointer to HAL data
 * \param[in] cfg  interface configuration
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_init(void *hal, ATCAIfaceCfg *cfg)
{
	ATCAHAL_t *phal = (ATCAHAL_t*)hal;

	int bus = cfg->atcai2c.bus; //!< 0-based logical bus number

	if (i2c_bus_ref_ct == 0)    //!< power up state, no i2c buses will have been used

		for (int i = 0; i < MAX_I2C_BUSES; i++)
			i2c_hal_data[i] = NULL;

	i2c_bus_ref_ct++;   //!< total across buses

	if (bus >= 0 && bus < MAX_I2C_BUSES) {
		//! if this is the first time this bus and interface has been created, do the physical work of enabling it
		if (i2c_hal_data[bus] == NULL) {
			i2c_hal_data[bus] = malloc(sizeof(ATCAI2CMaster_t));
			i2c_hal_data[bus]->ref_ct = 1;  //!< buses are shared, this is the first instance

			//! assign GPIO pins
			i2c_hal_data[bus]->pin_sda = i2c_buses_default.pin_sda[bus];
			i2c_hal_data[bus]->pin_scl = i2c_buses_default.pin_scl[bus];

			i2c_set_pin(i2c_hal_data[bus]->pin_sda, i2c_hal_data[bus]->pin_scl);
			i2c_enable();

			//! store this for use during the release phase
			i2c_hal_data[bus]->bus_index = bus;
		}else {
			//! otherwise, another interface already initialized the bus, so this interface will share it and any different
			//! cfg parameters will be ignored...first one to initialize this sets the configuration
			i2c_hal_data[bus]->ref_ct++;
		}

		phal->hal_data = i2c_hal_data[bus];

		return ATCA_SUCCESS;
	}

	return ATCA_COMM_FAIL;
}

/**
 * \brief HAL implementation of I2C post init.
 *
 * \param[in] iface  ATCAIface instance
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_post_init(ATCAIface iface)
{
	return ATCA_SUCCESS;
}

/**
 * \brief Send byte(s) via I2C.
 *
 * \param[in] iface     interface of the logical device to send data to
 * \param[in] txdata    pointer to bytes to send
 * \param[in] txlength  number of bytes to send
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_send(ATCAIface iface, uint8_t *txdata, int txlength)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);

	ATCA_STATUS status = ATCA_TX_TIMEOUT;

	int bus     = cfg->atcai2c.bus;

	txdata[0] = 0x03;   //!< Word Address Value = Command
	txlength++;         //!< count Word Address byte towards txlength

	//! Set I2C pins
	i2c_set_pin(i2c_hal_data[bus]->pin_sda, i2c_hal_data[bus]->pin_scl);

	do {
		//! Address the device and indicate that bytes are to be written
		status = hal_i2c_send_slave_address(iface, I2C_WRITE);
		if (status != ATCA_SUCCESS)
			break;

		//! Send the remaining bytes
		status = i2c_send_bytes(txlength, txdata);
	} while (0);

	//! Send STOP regardless of i2c_status
	i2c_send_stop();

	return status;
}

/**
 * \brief Receive byte(s) via I2C.
 *
 * \param[in] iface     interface of the logical device to receive data
 *                      from
 * \param[in] rxdata    pointer to where bytes will be received
 * \param[in] rxlength  pointer to expected number of receive bytes to
 *                      request
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_receive(ATCAIface iface, uint8_t *rxdata, uint16_t *rxlength)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);

	ATCA_STATUS status = ATCA_RX_TIMEOUT;

	int bus     = cfg->atcai2c.bus;
	int retries = cfg->rx_retries;
	uint8_t count;

	//! Set I2C pins
	i2c_set_pin(i2c_hal_data[bus]->pin_sda, i2c_hal_data[bus]->pin_scl);

	while (retries-- > 0 && status != ATCA_SUCCESS) {
		//! Address the device and indicate that bytes are to be read
		status = hal_i2c_send_slave_address(iface, I2C_READ);
		if (status == ATCA_SUCCESS) {
			//! Receive count byte
			i2c_receive_byte(rxdata);
			count = rxdata[0];
			if ((count < ATCA_RSP_SIZE_MIN) || (count > *rxlength)) {
				i2c_send_stop();
				status = ATCA_INVALID_SIZE;
				break;
			}

			//! Receive the remaining bytes
			i2c_receive_bytes(count - 1, &rxdata[1]);
		}
	}
	if (status == ATCA_TX_TIMEOUT)
		status = ATCA_RX_NO_RESPONSE;

	return status;
}

/**
 * \brief Send Wake flag via I2C.
 *
 * \param[in] iface  interface of the logical device to wake up
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_wake(ATCAIface iface)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);

	ATCA_STATUS status = ATCA_WAKE_FAILED;

	int bus     = cfg->atcai2c.bus;
	uint8_t response[4] = { 0x00, 0x00, 0x00, 0x00 };
	uint8_t expected_response[4] = { 0x04, 0x11, 0x33, 0x43 };

	//! Set I2C pins
	i2c_set_pin(i2c_hal_data[bus]->pin_sda, i2c_hal_data[bus]->pin_scl);

	//! Generate Wake Token
	i2c_send_wake_token();

	//! Wait tWHI + tWLO
	atca_delay_us(cfg->wake_delay);

	//! Receive Wake Response
	status = hal_i2c_receive(iface, response, sizeof(response));
	if (status == ATCA_SUCCESS) {
		//! Compare response with expected_response
		if (memcmp(response, expected_response, 4) != 0)
			status = ATCA_WAKE_FAILED;
	}

	return status;
}

/**
 * \brief Send Idle flag via I2C.
 *
 * \param[in] iface  interface of the logical device to idle
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_idle(ATCAIface iface)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);

	ATCA_STATUS status = ATCA_TX_TIMEOUT;

	int bus     = cfg->atcai2c.bus;

	//! Set I2C pins
	i2c_set_pin(i2c_hal_data[bus]->pin_sda, i2c_hal_data[bus]->pin_scl);

	//! Address the device and indicate that bytes are to be written
	status = hal_i2c_send_slave_address(iface, I2C_WRITE);
	if (status == ATCA_SUCCESS) {
		status = i2c_send_byte(0x02);   //!< Word Address Value = Idle
		i2c_send_stop();
	}

	return status;
}

/**
 * \brief Send Sleep flag via I2C.
 *
 * \param[in] iface  interface of the logical device to sleep
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_sleep(ATCAIface iface)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);

	ATCA_STATUS status = ATCA_TX_TIMEOUT;

	int bus     = cfg->atcai2c.bus;

	//! Set I2C pins
	i2c_set_pin(i2c_hal_data[bus]->pin_sda, i2c_hal_data[bus]->pin_scl);

	//! Address the device and indicate that bytes are to be written
	status = hal_i2c_send_slave_address(iface, I2C_WRITE);
	if (status == ATCA_SUCCESS) {
		status = i2c_send_byte(0x01);   //!< Word Address Value = Sleep
		i2c_send_stop();
	}

	return status;
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
ATCA_STATUS hal_i2c_release(void *hal_data)
{
	ATCAI2CMaster_t *hal = (ATCAI2CMaster_t*)hal_data;

	i2c_bus_ref_ct--;   // track total i2c bus interface instances for consistency checking and debugging

	// if the use count for this bus has gone to 0 references, disable it.  protect against an unbracketed release
	if (hal && --(hal->ref_ct) <= 0 && i2c_hal_data[hal->bus_index] != NULL) {
		i2c_set_pin(i2c_hal_data[hal->bus_index]->pin_sda, i2c_hal_data[hal->bus_index]->pin_scl);
		i2c_disable();
		free(i2c_hal_data[hal->bus_index]);
		i2c_hal_data[hal->bus_index] = NULL;
	}

	return ATCA_SUCCESS;
}

/** @} */