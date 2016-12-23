/**
 * \file
 * \brief ATCA Hardware abstraction layer for Linux using kit protocol over a USB CDC device.
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


#include "atca_hal.h"
#include "hal_uboot_i2c.h"
#include <common.h>


/** \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief
 * These methods define the hardware abstraction layer for communicating with a CryptoAuth device
 *
@{ */

// File scope globals
ATCAI2CMaster_t *i2c_hal_data[MAX_I2C_BUSES];	// map logical, 0-based bus number to index
int i2c_bus_ref_ct = 0;				// total in-use count across buses

/** \brief discover i2c buses available for this hardware
 * this maintains a list of logical to physical bus mappings freeing the application
 * of the a-priori knowledge
 * \param[in] i2c_buses - an array of logical bus numbers
 * \param[in] max_buses - maximum number of buses the app wants to attempt to discover
 */

ATCA_STATUS hal_i2c_discover_buses( int i2c_buses[], int max_buses )
{
	return ATCA_UNIMPLEMENTED;
}

/** \brief discover any CryptoAuth devices on a given logical bus number
 * \param[in]  busNum  logical bus number on which to look for CryptoAuth devices
 * \param[out] cfg     pointer to head of an array of interface config structures which get filled in by this method
 * \param[out] found   number of devices found on this bus
 */

ATCA_STATUS hal_i2c_discover_devices( int busNum, ATCAIfaceCfg cfg[], int *found )
{
	return ATCA_UNIMPLEMENTED;
}

/** \brief HAL implementation of I2C init
 *
 * this implementation assumes I2C peripheral has been enabled by user. It only initialize an
 * I2C interface using given config.
 *
 *  \param[in] hal pointer to HAL specific data that is maintained by this HAL
 *  \param[in] cfg pointer to HAL specific configuration data that is used to initialize this HAL
 * \return ATCA_STATUS
 */

ATCA_STATUS hal_i2c_init( void* hal, ATCAIfaceCfg* cfg )
{
	int bus = cfg->atcai2c.bus;   // 0-based logical bus number
	ATCAHAL_t *phal = (ATCAHAL_t*)hal;
	int i = 0; 
	if ( i2c_bus_ref_ct == 0 )     // power up state, no i2c buses will have been used
		for ( i = 0; i < MAX_I2C_BUSES; i++ )
			i2c_hal_data[i] = NULL;

	i2c_bus_ref_ct++;  // total across buses

	if ( bus >= 0 && bus < MAX_I2C_BUSES ) {
		// if this is the first time this bus and interface has been created, do the physical work of enabling it
		if ( i2c_hal_data[bus] == NULL ) {
			i2c_hal_data[bus] = malloc( sizeof(ATCAI2CMaster_t) );
			i2c_hal_data[bus]->ref_ct = 1;  // buses are shared, this is the first instance
			// store this for use during the release phase
			i2c_hal_data[bus]->bus_index = bus;

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
ATCA_STATUS hal_i2c_post_init( ATCAIface iface )
{
	return ATCA_SUCCESS;
}

/** \brief HAL implementation of I2C send over ASF
 * \param[in] iface     instance
 * \param[in] txdata    pointer to space to bytes to send
 * \param[in] txlength  number of bytes to send
 * \return ATCA_STATUS
 */

ATCA_STATUS hal_i2c_send( ATCAIface iface, uint8_t *txdata, int txlength )
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	
	txdata[0] = 0x03;	// insert the Word Address Value, Command token
	txlength++;		// account for word address value byte.
	
	i2c_write((cfg->atcai2c.slave_address >> 1), 0, 0, txdata, txlength);
	
	return ATCA_SUCCESS;
}

/** \brief HAL implementation of I2C receive function for ASF I2C
 * \param[in] iface     instance
 * \param[in] rxdata    pointer to space to receive the data
 * \param[in] rxlength  ptr to expected number of receive bytes to request
 * \return ATCA_STATUS
 */

ATCA_STATUS hal_i2c_receive( ATCAIface iface, uint8_t *rxdata, uint16_t *rxlength )
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;
		
	if (i2c_read((cfg->atcai2c.slave_address >> 1), 0, 0, rxdata, *rxlength) == 0)
	{
		return ATCA_SUCCESS;
	}
	
	return ATCA_COMM_FAIL;
}

/** \brief method to change the bus speec of I2C
 * \param[in] iface  interface on which to change bus speed
 * \param[in] speed  baud rate (typically 100000 or 400000)
 */

void change_i2c_speed( ATCAIface iface, uint32_t speed )
{

}

/** \brief wake up CryptoAuth device using I2C bus
 * \param[in] iface  interface to logical device to wakeup
 */

ATCA_STATUS hal_i2c_wake( ATCAIface iface )
{
	uint8_t data[4], expected[4] = { 0x04, 0x11, 0x33, 0x43 };
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;
		
	i2c_write(0, 0, 0, data, 0);		// Dummy Write
	udelay(800);
	if (i2c_read((cfg->atcai2c.slave_address >> 1), 0, 0, data, 4) == 0)	// Read Response
	{
		if (memcmp(data, expected, 4) == 0)
		{
			return ATCA_SUCCESS;
		}
	}
	
	return ATCA_COMM_FAIL;
}

/** \brief idle CryptoAuth device using I2C bus
 * \param[in] iface  interface to logical device to idle
 */

ATCA_STATUS hal_i2c_idle( ATCAIface iface )
{
	uint8_t dummy = 0x02;
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);

	i2c_write((cfg->atcai2c.slave_address >> 1), 0, 0, &dummy, 1);
	
	return ATCA_SUCCESS;
}

/** \brief sleep CryptoAuth device using I2C bus
 * \param[in] iface  interface to logical device to sleep
 */

ATCA_STATUS hal_i2c_sleep( ATCAIface iface )
{
	uint8_t dummy = 0x01;
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	
	i2c_write((cfg->atcai2c.slave_address >> 1), 0, 0, &dummy, 1);
	
	return ATCA_SUCCESS;
}

/** \brief manages reference count on given bus and releases resource if no more refences exist
 * \param[in] hal_data - opaque pointer to hal data structure - known only to the HAL implementation
 */

ATCA_STATUS hal_i2c_release( void *hal_data )
{
	ATCAI2CMaster_t *hal = (ATCAI2CMaster_t*)hal_data;

	i2c_bus_ref_ct--;  // track total i2c bus interface instances for consistency checking and debugging

	// if the use count for this bus has gone to 0 references, disable it.  protect against an unbracketed release
	if ( hal && --(hal->ref_ct) <= 0 && i2c_hal_data[hal->bus_index] != NULL ) {
		free(i2c_hal_data[hal->bus_index]);
		i2c_hal_data[hal->bus_index] = NULL;
	}

	return ATCA_SUCCESS;
}

/** @} */
