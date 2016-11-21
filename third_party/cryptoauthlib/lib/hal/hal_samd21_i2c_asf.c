/**
 * \file
 * \brief ATCA Hardware abstraction layer for SAMD21 I2C over ASF drivers.
 *
 * This code is structured in two parts.  Part 1 is the connection of the ATCA HAL API to the physical I2C
 * implementation. Part 2 is the ASF I2C primitives to set up the interface.
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

#include <asf.h>
#include <string.h>
#include <stdio.h>

#include "atca_hal.h"
#include "hal_samd21_i2c_asf.h"
#include "atca_device.h"

/** \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief
 * These methods define the hardware abstraction layer for communicating with a CryptoAuth device
 * using I2C driver of ASF.
 *
   @{ */

/** \brief logical to physical bus mapping structure */
ATCAI2CMaster_t *i2c_hal_data[MAX_I2C_BUSES];   // map logical, 0-based bus number to index
int i2c_bus_ref_ct = 0;                         // total in-use count across buses
static struct i2c_master_config config_i2c_master;

/** \brief discover i2c buses available for this hardware
 * this maintains a list of logical to physical bus mappings freeing the application
 * of the a-priori knowledge
 * \param[in] i2c_buses - an array of logical bus numbers
 * \param[in] max_buses - maximum number of buses the app wants to attempt to discover
 */

ATCA_STATUS hal_i2c_discover_buses(int i2c_buses[], int max_buses)
{

	/* if every SERCOM was a likely candidate bus, then would need to initialize the entire array to all SERCOM n numbers.
	 * As an optimization and making discovery safer, make assumptions about bus-num / SERCOM map based on D21 Xplained Pro board
	 * If you were using a raw D21 on your own board, you would supply your own bus numbers based on your particular hardware configuration.
	 */
#ifdef __SAMR21G18A__
	i2c_buses[0] = 1;   // default r21 for xplained pro dev board
#else
	i2c_buses[0] = 2;   // default d21 for xplained pro dev board
#endif

	return ATCA_SUCCESS;
}

/** \brief discover any CryptoAuth devices on a given logical bus number
 * \param[in]  busNum  logical bus number on which to look for CryptoAuth devices
 * \param[out] cfg     pointer to head of an array of interface config structures which get filled in by this method
 * \param[out] found   number of devices found on this bus
 */

ATCA_STATUS hal_i2c_discover_devices(int busNum, ATCAIfaceCfg cfg[], int *found )
{
	ATCAIfaceCfg *head = cfg;
	uint8_t slaveAddress = 0x01;
	ATCADevice device;
	ATCAIface discoverIface;
	ATCACommand command;
	ATCAPacket packet;
	uint32_t execution_time;
	ATCA_STATUS status;
	uint8_t revs508[1][4] = { { 0x00, 0x00, 0x50, 0x00 } };
	uint8_t revs108[1][4] = { { 0x80, 0x00, 0x10, 0x01 } };
	uint8_t revs204[2][4] = { { 0x00, 0x02, 0x00, 0x08 },
							  { 0x00, 0x04, 0x05, 0x00 } };
	int i;

	/** \brief default configuration, to be reused during discovery process */
	ATCAIfaceCfg discoverCfg = {
		.iface_type				= ATCA_I2C_IFACE,
		.devtype				= ATECC508A,
		.atcai2c.slave_address	= 0x07,
		.atcai2c.bus			= busNum,
		.atcai2c.baud			= 400000,
		//.atcai2c.baud = 100000,
		.wake_delay				= 800,
		.rx_retries				= 3
	};

	ATCAHAL_t hal;

	hal_i2c_init( &hal, &discoverCfg );
	device = newATCADevice( &discoverCfg );
	discoverIface = atGetIFace( device );
	command = atGetCommands( device );

	// iterate through all addresses on given i2c bus
	// all valid 7-bit addresses go from 0x07 to 0x78
	for ( slaveAddress = 0x07; slaveAddress <= 0x78; slaveAddress++ ) {
		discoverCfg.atcai2c.slave_address = slaveAddress << 1;  // turn it into an 8-bit address which is what the rest of the i2c HAL is expecting when a packet is sent

		// wake up device
		// If it wakes, send it a dev rev command.  Based on that response, determine the device type
		// BTW - this will wake every cryptoauth device living on the same bus (ecc508a, sha204a)

		if ( hal_i2c_wake( discoverIface ) == ATCA_SUCCESS ) {
			(*found)++;
			memcpy( (uint8_t*)head, (uint8_t*)&discoverCfg, sizeof(ATCAIfaceCfg));

			memset( packet.data, 0x00, sizeof(packet.data));

			// get devrev info and set device type accordingly
			atInfo( command, &packet );
			execution_time = atGetExecTime(command, CMD_INFO) + 1;

			// send the command
			if ( (status = atsend( discoverIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS ) {
				printf("packet send error\r\n");
				continue;
			}

			// delay the appropriate amount of time for command to execute
			atca_delay_ms(execution_time);

			// receive the response
			if ( (status = atreceive( discoverIface, &(packet.data[0]), &(packet.rxsize) )) != ATCA_SUCCESS )
				continue;

			if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS ) {
				printf("command response error\r\n");
				continue;
			}

			// determine device type from common info and dev rev response byte strings
			for ( i = 0; i < (int)sizeof(revs508) / 4; i++ ) {
				if ( memcmp( &packet.data[1], &revs508[i], 4) == 0 ) {
					discoverCfg.devtype = ATECC508A;
					break;
				}
			}

			for ( i = 0; i < (int)sizeof(revs204) / 4; i++ ) {
				if ( memcmp( &packet.data[1], &revs204[i], 4) == 0 ) {
					discoverCfg.devtype = ATSHA204A;
					break;
				}
			}

			for ( i = 0; i < (int)sizeof(revs108) / 4; i++ ) {
				if ( memcmp( &packet.data[1], &revs108[i], 4) == 0 ) {
					discoverCfg.devtype = ATECC108A;
					break;
				}
			}

			atca_delay_ms(15);
			// now the device type is known, so update the caller's cfg array element with it
			head->devtype = discoverCfg.devtype;
			head++;
		}

		hal_i2c_idle(discoverIface);
	}

	hal_i2c_release(&hal);

	return ATCA_SUCCESS;
}

/** \brief
    - this HAL implementation assumes you've included the ASF SERCOM I2C libraries in your project, otherwise,
    the HAL layer will not compile because the ASF I2C drivers are a dependency *
 */

/** \brief hal_i2c_init manages requests to initialize a physical interface.  it manages use counts so when an interface
 * has released the physical layer, it will disable the interface for some other use.
 * You can have multiple ATCAIFace instances using the same bus, and you can have multiple ATCAIFace instances on
 * multiple i2c buses, so hal_i2c_init manages these things and ATCAIFace is abstracted from the physical details.
 */

/** \brief initialize an I2C interface using given config
 * \param[in] hal - opaque ptr to HAL data
 * \param[in] cfg - interface configuration
 */
ATCA_STATUS hal_i2c_init(void *hal, ATCAIfaceCfg *cfg)
{
	int bus = cfg->atcai2c.bus;   // 0-based logical bus number
	ATCAHAL_t *phal = (ATCAHAL_t*)hal;

	if ( i2c_bus_ref_ct == 0 )     // power up state, no i2c buses will have been used
		for ( int i = 0; i < MAX_I2C_BUSES; i++ )
			i2c_hal_data[i] = NULL;

	i2c_bus_ref_ct++;  // total across buses

	if ( bus >= 0 && bus < MAX_I2C_BUSES ) {
		// if this is the first time this bus and interface has been created, do the physical work of enabling it
		if ( i2c_hal_data[bus] == NULL ) {
			i2c_hal_data[bus] = malloc( sizeof(ATCAI2CMaster_t) );
			i2c_hal_data[bus]->ref_ct = 1;  // buses are shared, this is the first instance
			i2c_master_get_config_defaults(&config_i2c_master);
#ifdef __SAMR21G18A__
			config_i2c_master.pinmux_pad0 = PINMUX_PA16C_SERCOM1_PAD0;
			config_i2c_master.pinmux_pad1 = PINMUX_PA17C_SERCOM1_PAD1;
#endif

			// config_i2c_master.buffer_timeout = 10000;
			config_i2c_master.baud_rate = cfg->atcai2c.baud / 1000;

			switch (bus) {
			case 0: i2c_master_init( &(i2c_hal_data[bus]->i2c_master_instance), SERCOM0, &config_i2c_master); break;
			case 1: i2c_master_init( &(i2c_hal_data[bus]->i2c_master_instance), SERCOM1, &config_i2c_master); break;
			case 2: i2c_master_init( &(i2c_hal_data[bus]->i2c_master_instance), SERCOM2, &config_i2c_master); break;
			case 3: i2c_master_init( &(i2c_hal_data[bus]->i2c_master_instance), SERCOM3, &config_i2c_master); break;
			case 4: i2c_master_init( &(i2c_hal_data[bus]->i2c_master_instance), SERCOM4, &config_i2c_master); break;
			case 5: i2c_master_init( &(i2c_hal_data[bus]->i2c_master_instance), SERCOM5, &config_i2c_master); break;
			}

			// store this for use during the release phase
			i2c_hal_data[bus]->bus_index = bus;
			i2c_master_enable(&(i2c_hal_data[bus]->i2c_master_instance));
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

/** \brief HAL implementation of I2C send over ASF
 * \param[in] iface     instance
 * \param[in] txdata    pointer to space to bytes to send
 * \param[in] txlength  number of bytes to send
 * \return ATCA_STATUS
 */

ATCA_STATUS hal_i2c_send(ATCAIface iface, uint8_t *txdata, int txlength)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;

	struct i2c_master_packet packet = {
		.address			= cfg->atcai2c.slave_address >> 1,
		.data_length		= txlength,
		.data				= txdata,
		.ten_bit_address	= false,
		.high_speed			= false,
		.hs_master_code		= 0x0,
	};

	// for this implementation of I2C with CryptoAuth chips, txdata is assumed to have ATCAPacket format

	// other device types that don't require i/o tokens on the front end of a command need a different hal_i2c_send and wire it up instead of this one
	// this covers devices such as ATSHA204A and ATECCx08A that require a word address value pre-pended to the packet
	// txdata[0] is using _reserved byte of the ATCAPacket
	txdata[0] = 0x03;   // insert the Word Address Value, Command token
	txlength++;         // account for word address value byte.
	packet.data_length = txlength;

	//	statusCode = i2c_master_write_packet_wait(&i2c_master_instance, &packet);
	//if ( i2c_master_write_packet_wait_no_stop( &(i2c_hal_data[bus]->i2c_master_instance), &packet) != STATUS_OK)
	if ( i2c_master_write_packet_wait(&(i2c_hal_data[bus]->i2c_master_instance), &packet) != STATUS_OK )
		return ATCA_COMM_FAIL;

	return ATCA_SUCCESS;
}

/** \brief HAL implementation of I2C receive function for ASF I2C
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
	int status = !STATUS_OK;

	struct i2c_master_packet packet = {
		.address			= cfg->atcai2c.slave_address >> 1,
		.data_length		= *rxlength,
		.data				= rxdata,
		.ten_bit_address	= false,
		.high_speed			= false,
		.hs_master_code		= 0x0,
	};

	while ( retries-- > 0 && status != STATUS_OK )
		status = i2c_master_read_packet_wait( &(i2c_hal_data[bus]->i2c_master_instance), &packet);

	if ( status != STATUS_OK )
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

	config_i2c_master.buffer_timeout = 10000;
	config_i2c_master.baud_rate = speed / 1000;

	i2c_master_disable(&(i2c_hal_data[bus]->i2c_master_instance));

	switch (bus) {
	case 0: i2c_master_init(  &(i2c_hal_data[bus]->i2c_master_instance), SERCOM0, &config_i2c_master); break;
	case 1: i2c_master_init(  &(i2c_hal_data[bus]->i2c_master_instance), SERCOM1, &config_i2c_master); break;
	case 2: i2c_master_init(  &(i2c_hal_data[bus]->i2c_master_instance), SERCOM2, &config_i2c_master); break;
	case 3: i2c_master_init(  &(i2c_hal_data[bus]->i2c_master_instance), SERCOM3, &config_i2c_master); break;
	case 4: i2c_master_init(  &(i2c_hal_data[bus]->i2c_master_instance), SERCOM4, &config_i2c_master); break;
	case 5: i2c_master_init(  &(i2c_hal_data[bus]->i2c_master_instance), SERCOM5, &config_i2c_master); break;
	}

	i2c_master_enable(&(i2c_hal_data[bus]->i2c_master_instance));
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
	int status = !STATUS_OK;
	uint8_t data[4], expected[4] = { 0x04, 0x11, 0x33, 0x43 };

	if ( bdrt != 100000 )  // if not already at 100KHz, change it
		change_i2c_speed( iface, 100000 );

	// Send the wake by writing to an address of 0x00
	struct i2c_master_packet packet = {
		.address			= 0x00,
		.data_length		= 0,
		.data				= &data[0],
		.ten_bit_address	= false,
		.high_speed			= false,
		.hs_master_code		= 0x0,
	};

	// Send the 00 address as the wake pulse
	i2c_master_write_packet_wait(  &(i2c_hal_data[bus]->i2c_master_instance), &packet );    // part will NACK, so don't check for status

	atca_delay_us(cfg->wake_delay);                                                         // wait tWHI + tWLO which is configured based on device type and configuration structure

	packet.address = cfg->atcai2c.slave_address >> 1;
	packet.data_length = 4;
	packet.data = data;

	while ( retries-- > 0 && status != STATUS_OK )
		status = i2c_master_read_packet_wait( &(i2c_hal_data[bus]->i2c_master_instance), &packet);

	// if necessary, revert baud rate to what came in.
	if ( bdrt != 100000 )
		change_i2c_speed( iface, bdrt );

	if ( status != STATUS_OK )
		return ATCA_COMM_FAIL;

	if ( memcmp( data, expected, 4 ) == 0 )
		return ATCA_SUCCESS;

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

	struct i2c_master_packet packet = {
		.address			= cfg->atcai2c.slave_address >> 1,
		.data_length		= 1,
		.data				= &data[0],
		.ten_bit_address	= false,
		.high_speed			= false,
		.hs_master_code		= 0x0,
	};

	data[0] = 0x02;  // idle word address value
	if ( i2c_master_write_packet_wait(&(i2c_hal_data[bus]->i2c_master_instance), &packet) != STATUS_OK )
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

	struct i2c_master_packet packet = {
		.address			= cfg->atcai2c.slave_address >> 1,
		.data_length		= 1,
		.data				= &data[0],
		.ten_bit_address	= false,
		.high_speed			= false,
		.hs_master_code		= 0x0,
	};

	data[0] = 0x01;  // sleep word address value
	if ( i2c_master_write_packet_wait(&(i2c_hal_data[bus]->i2c_master_instance), &packet) != STATUS_OK )
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

	// if the use count for this bus has gone to 0 references, disable it.  protect against an unbracketed release
	if ( hal && --(hal->ref_ct) <= 0 && i2c_hal_data[hal->bus_index] != NULL ) {
		i2c_master_reset(&(hal->i2c_master_instance));
		free(i2c_hal_data[hal->bus_index]);
		i2c_hal_data[hal->bus_index] = NULL;
	}

	return ATCA_SUCCESS;
}

/** @} */
