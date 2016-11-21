/**
 * \file
 * \brief ATCA Hardware abstraction layer for SAMD21 I2C over START drivers.
 *
 * This code is structured in two parts.  Part 1 is the connection of the ATCA HAL API to the physical I2C
 * implementation. Part 2 is the START I2C primitives to set up the interface.
 *
 * Prerequisite: add SERCOM I2C Master Polled support to application in Atmel Studio
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
#include <string.h>
#include <stdio.h>
#include <atmel_start.h>
#include <hal_gpio.h>
#include <hal_delay.h>
#include "atca_hal.h"
#include "hal_samd21_i2c_start.h"
#include "peripheral_gclk_config.h"

#include "atca_start_config.h"
#include "atca_start_iface.h"

/** \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief
 * These methods define the hardware abstraction layer for communicating with a CryptoAuth device
 *
   @{ */

ATCAI2CMaster_t *i2c_hal_data[MAX_I2C_BUSES];   // map logical, 0-based bus number to index
int i2c_bus_ref_ct = 0;                         // total in-use count across buses

/* Notes:
    - this HAL implementation assumes you've included the Atmel START SERCOM I2C libraries in your project, otherwise,
    the HAL layer will not compile because the START I2C drivers are a dependency *
 */

/* \brief hal_i2c_init manages requests to initialize a physical interface.  it manages use counts so when an interface
 * has released the physical layer, it will disable the interface for some other use.
 * You can have multiple ATCAIFace instances using the same bus, and you can have multiple ATCAIFace instances on
 * multiple i2c buses, so hal_i2c_init manages these things and ATCAIFace is abstracted from the physical details.
 */

ATCA_STATUS hal_i2c_init(void *hal, ATCAIfaceCfg *cfg)
{
	int bus = cfg->atcai2c.bus; // 0-based logical bus number
	ATCAHAL_t *phal = (ATCAHAL_t*)hal;
	uint32_t freq_constant;     // I2C frequency configuration constant in kHz

	if ( i2c_bus_ref_ct == 0 )  // power up state, no i2c buses will have been used
		for ( int i = 0; i < MAX_I2C_BUSES; i++ )
			i2c_hal_data[i] = NULL;

	i2c_bus_ref_ct++;  // total across buses

	if ( bus >= 0 && bus < MAX_I2C_BUSES ) {
		// if this is the first time this bus and interface has been created, do the physical work of enabling it
		if ( i2c_hal_data[bus] == NULL ) {
			i2c_hal_data[bus] = malloc( sizeof(ATCAI2CMaster_t) );
			i2c_hal_data[bus]->ref_ct = 1;  // buses are shared, this is the first instance

			// store I2C baudrate in kHz
			freq_constant = cfg->atcai2c.baud / 1000;

			switch (bus) {
			case 0: break;
			case 1: break;
			case 2: memcpy(&(i2c_hal_data[bus]->i2c_master_instance), &ATCA_I2C_A_MODULE, sizeof(struct i2c_m_sync_desc)); break;
			case 3: break;
			case 4: break;
			case 5: break;
			}

			// store this for use during the release phase
			i2c_hal_data[bus]->bus_index = bus;

			// set I2C baudrate and enable I2C module
			i2c_m_sync_set_baudrate(&(i2c_hal_data[bus]->i2c_master_instance), CONF_GCLK_SERCOM2_CORE_FREQUENCY / 1000, freq_constant);
			i2c_m_sync_enable(&(i2c_hal_data[bus]->i2c_master_instance));

		}  else{
			// otherwise, another interface already initialized the bus, so this interface will share it and any different
			// cfg parameters will be ignored...first one to initialize this sets the configuration
			i2c_hal_data[bus]->ref_ct++;
		}
		phal->hal_data = i2c_hal_data[bus];

		return ATCA_SUCCESS;
	}

	return ATCA_COMM_FAIL;
}

/** \brief HAL implementation of I2C post init
 * \param[in] iface  instance
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_post_init(ATCAIface iface)
{
	return ATCA_SUCCESS;
}

/** \brief HAL implementation of I2C send over START
 * \param[in] iface     instance
 * \param[in] txdata    pointer to space to bytes to send
 * \param[in] txlength  number of bytes to send
 * \return ATCA_STATUS
 */

ATCA_STATUS hal_i2c_send(ATCAIface iface, uint8_t *txdata, int txlength)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;

	struct _i2c_m_msg packet = {
		.addr	= cfg->atcai2c.slave_address >> 1,
		.buffer = txdata,
		.flags	= I2C_M_SEVEN | I2C_M_STOP,
	};

	// for this implementation of I2C with CryptoAuth chips, txdata is assumed to have ATCAPacket format

	// other device types that don't require i/o tokens on the front end of a command need a different hal_i2c_send and wire it up instead of this one
	// this covers devices such as ATSHA204A and ATECCx08A that require a word address value pre-pended to the packet
	// txdata[0] is using _reserved byte of the ATCAPacket
	txdata[0] = 0x03;   // insert the Word Address Value, Command token
	txlength++;         // account for word address value byte.
	packet.len = txlength;

	if (i2c_m_sync_transfer(&(i2c_hal_data[bus]->i2c_master_instance), &packet) != I2C_OK)
		return ATCA_COMM_FAIL;

	return ATCA_SUCCESS;
}

/** \brief HAL implementation of I2C receive function for START I2C
 * \param[in] iface     instance
 * \param[in] rxdata    pointer to space to receive the data
 * \param[in] rxlength  ptr to expected number of receive bytes to request
 * \return ATCA_STATUS
 */

ATCA_STATUS hal_i2c_receive( ATCAIface iface, uint8_t *rxdata, uint16_t *rxlength)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;
	int retries = cfg->rx_retries;
	int status = !I2C_OK;

	struct _i2c_m_msg packet = {
		.addr	= cfg->atcai2c.slave_address >> 1,
		.len	= *rxlength,
		.buffer = rxdata,
		.flags	= I2C_M_SEVEN | I2C_M_RD | I2C_M_STOP,
	};

	while ( retries-- > 0 && status != I2C_OK )
		status = i2c_m_sync_transfer(&(i2c_hal_data[bus]->i2c_master_instance), &packet);

	if ( status != I2C_OK )
		return ATCA_COMM_FAIL;

	return ATCA_SUCCESS;
}

/** \brief method to change the bus speec of I2C
 * \param[in] iface  interface on which to change bus speed
 * \param[in] speed  baud rate (typically 100000 or 400000)
 */

void change_i2c_speed( ATCAIface iface, uint32_t speed )
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;
	uint32_t freq_constant; // I2C frequency configuration constant

	// disable I2C module
	i2c_m_sync_disable(&(i2c_hal_data[bus]->i2c_master_instance));

	// store I2C baudrate in kHz
	freq_constant = speed / 1000;

	// set I2C baudrate and enable I2C module
	i2c_m_sync_set_baudrate(&(i2c_hal_data[bus]->i2c_master_instance), CONF_GCLK_SERCOM2_CORE_FREQUENCY / 1000, freq_constant);
	i2c_m_sync_enable(&(i2c_hal_data[bus]->i2c_master_instance));
}

/** \brief wake up CryptoAuth device using I2C bus
 * \param[in] iface  interface to logical device to wakeup
 */

ATCA_STATUS hal_i2c_wake(ATCAIface iface)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;
	int retries = cfg->rx_retries;
	uint32_t bdrt = cfg->atcai2c.baud;
	int status = !I2C_OK;
	uint8_t data[4], expected[4] = { 0x04, 0x11, 0x33, 0x43 };

	if ( bdrt != 100000 )  // if not already at 100KHz, change it
		change_i2c_speed( iface, 100000 );

	// send the wake by writing to an address of 0x00
	struct _i2c_m_msg packet = {
		.addr	= 0x00,
		.len	= 0,
		.buffer = NULL,
		.flags	= I2C_M_SEVEN | I2C_M_STOP,
	};

	// Send the 00 address as the wake pulse
	status = i2c_m_sync_transfer(&(i2c_hal_data[bus]->i2c_master_instance), &packet);   // part will NACK, so don't check for status

	delay_us(cfg->wake_delay);                                                          // wait tWHI + tWLO which is configured based on device type and configuration structure

	// receive the wake up response
	packet.addr = cfg->atcai2c.slave_address >> 1;
	packet.len = 4;
	packet.buffer = data;
	packet.flags  = I2C_M_SEVEN | I2C_M_RD | I2C_M_STOP;

	while ( retries-- > 0 && status != I2C_OK )
		status = i2c_m_sync_transfer(&(i2c_hal_data[bus]->i2c_master_instance), &packet);

	if ( status == I2C_OK ) {
		// if necessary, revert baud rate to what came in.
		if ( bdrt != 100000 )
			change_i2c_speed( iface, bdrt );
		// compare received data with expected value
		if ( memcmp( data, expected, 4 ) == 0 )
			return ATCA_SUCCESS;
	}

	return ATCA_COMM_FAIL;
}

/** \brief idle CryptoAuth device using I2C bus
 * \param[in] iface  interface to logical device to idle
 */

ATCA_STATUS hal_i2c_idle(ATCAIface iface)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;
	uint8_t data[4];

	struct _i2c_m_msg packet = {
		.addr	= cfg->atcai2c.slave_address >> 1,
		.len	= 1,
		.buffer = &data[0],
		.flags	= I2C_M_SEVEN | I2C_M_STOP,
	};

	data[0] = 0x02;  // idle word address value

	if (i2c_m_sync_transfer(&(i2c_hal_data[bus]->i2c_master_instance), &packet) != I2C_OK)
		return ATCA_COMM_FAIL;

	return ATCA_SUCCESS;
}

/** \brief sleep CryptoAuth device using I2C bus
 * \param[in] iface  interface to logical device to sleep
 */

ATCA_STATUS hal_i2c_sleep(ATCAIface iface)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;
	uint8_t data[4];

	struct _i2c_m_msg packet = {
		.addr	= cfg->atcai2c.slave_address >> 1,
		.len	= 1,
		.buffer = &data[0],
		.flags	= I2C_M_SEVEN | I2C_M_STOP,
	};

	data[0] = 0x01;  // sleep word address value

	if (i2c_m_sync_transfer(&(i2c_hal_data[bus]->i2c_master_instance), &packet) != I2C_OK)
		return ATCA_COMM_FAIL;

	return ATCA_SUCCESS;
}

/** \brief manages reference count on given bus and releases resource if no more refences exist
 * \param[in] hal_data - opaque pointer to hal data structure - known only to the HAL implementation
 */

ATCA_STATUS hal_i2c_release( void *hal_data )
{
	ATCAI2CMaster_t *hal = (ATCAI2CMaster_t*)hal_data;

	i2c_bus_ref_ct--;  // track total i2c bus interface instances for consistency checking and debugging

	//if the use count for this bus has gone to 0 references, disable it.  protect against an unbracketed release
	if ( hal && --(hal->ref_ct) <= 0 && i2c_hal_data[hal->bus_index] != NULL ) {
		i2c_m_sync_disable(&(hal->i2c_master_instance));
		free(i2c_hal_data[hal->bus_index]);
		i2c_hal_data[hal->bus_index] = NULL;
	}

	return ATCA_SUCCESS;
}

/** @} */
