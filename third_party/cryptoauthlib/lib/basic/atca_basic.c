/**
 * \file
 * \brief CryptoAuthLib Basic API methods.  These methods provide a simpler way to access the core crypto
 * methods.  Their design center is around the most common modes and functions of each command
 * rather than the complete implementation of each possible feature of the chip.  If you need a feature
 * not supplied in the Basic API, you can achieve the feature through the datasheet level command
 * supplied through the ATCADevice and ATCACommand object.
 *
 * One primary assumption to all atcab_ routines is that the caller manages the wake/sleep/idle bracket
 * so prior to calling the atcab_ routine, the chip should be awake; the routine will manage wake/sleep/idle
 * within the function and leave the chip awake upon return.
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


#include "atca_basic.h"
#include "host/atca_host.h"

char atca_version[] = { "20160108" };  // change for each release, yyyymmdd

/** \brief returns a version string for the CryptoAuthLib release.
 *  The format of the version string returned is "yyyymmdd"
 * \param[out] verstr ptr to space to receive version string
 * \return ATCA_STATUS
 */

ATCA_STATUS atcab_version( char *verstr )
{
	strcpy( verstr, atca_version );
	return ATCA_SUCCESS;
}

/** \brief basic API methods are all prefixed with atcab_  (Atmel CryptoAuth Basic)
 *  the fundamental premise of the basic API is it is based on a single interface
 *  instance and that instance is global, so all basic API commands assume that
 *  one global device is the one to operate on.
 */

ATCADevice _gDevice = NULL;
ATCACommand _gCommandObj = NULL;
ATCAIface _gIface = NULL;

/** \brief atcab_init is called once for the life of the application and creates a global ATCADevice object used by Basic API.
 *  This method builds a global ATCADevice instance behinds the scenes that's used for all Basic API operations
 *  \param[in] cfg is a pointer to an interface configuration.  This is usually a predefined configuration found in atca_cfgs.h
 *  \return ATCA_STATUS
 *  \see atcab_init_device()
 */
ATCA_STATUS atcab_init( ATCAIfaceCfg *cfg )
{
	if ( _gDevice )     // if there's already a device created, release it
		atcab_release();

	_gDevice = newATCADevice( cfg );
	if ( _gDevice == NULL )
		return ATCA_GEN_FAIL; // Device creation failed

	_gCommandObj = atGetCommands( _gDevice );
	_gIface = atGetIFace( _gDevice );

	if ( _gCommandObj == NULL || _gIface == NULL )
		return ATCA_GEN_FAIL; // More of an assert to make everything was constructed properly

	return ATCA_SUCCESS;
}


/** \brief atcab_init_device can be used to initialize the global ATCADevice object to point to one of your choosing
 *  for use with all the atcab_ basic API.
 *  \param[in] cadevice ATCADevice instance to use as the global Basic API crypto device instance
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_init_device( ATCADevice cadevice )
{
	if ( cadevice == NULL )
		return ATCA_BAD_PARAM;

	if ( _gDevice )     // if there's already a device created, release it
		atcab_release();

	_gDevice = cadevice;
	_gCommandObj = atGetCommands( _gDevice );
	_gIface = atGetIFace(_gDevice);

	if ( _gCommandObj == NULL || _gIface == NULL )
		return ATCA_GEN_FAIL;

	return ATCA_SUCCESS;
}

/** \brief release (free) the global ATCADevice instance.
 *  This must be called in order to release or free up the interface.
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_release( void )
{
	deleteATCADevice(&_gDevice);
	return ATCA_SUCCESS;
}

/** \brief a way to get the global device object.  Generally for more sophisticated users of atca
 *  \return instance of global ATCADevice
 */

ATCADevice atcab_getDevice(void)
{
	return _gDevice;
}


/** \brief wakeup the CryptoAuth device
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_wakeup(void)
{
	if ( _gDevice == NULL )
		return ATCA_GEN_FAIL;

	return atwake(_gIface);
}

/** \brief idle the CryptoAuth device
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_idle(void)
{
	if ( _gDevice == NULL )
		return ATCA_GEN_FAIL;

	return atidle(_gIface);
}

/** \brief invoke sleep on the CryptoAuth device
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_sleep(void)
{
	if ( _gDevice == NULL )
		return ATCA_GEN_FAIL;

	return atsleep(_gIface);
}


/** \brief auto discovery of crypto auth devices
 *
 * Calls interface discovery functions and fills in cfgArray up to the maximum
 * number of configurations either found or the size of the array. The cfgArray
 * can have a mixture of interface types (ie: some I2C, some SWI or UART) depending upon
 * which interfaces you've enabled
 *
 * \param[out] cfgArray, ptr to an array of interface configs
 * \param[in] max, maximum size of cfgArray
 * \return ATCA_STATUS
 */

#define MAX_BUSES   4

ATCA_STATUS atcab_cfg_discover( ATCAIfaceCfg cfgArray[], int maxIfaces)
{
	int ifaceNum = 0, i;
	int found = 0;

	// this cumulatively gathers all the interfaces enabled by #defines

#ifdef ATCA_HAL_I2C
	int i2c_buses[MAX_BUSES];
	memset( i2c_buses, -1, sizeof(i2c_buses));
	hal_i2c_discover_buses(i2c_buses, MAX_BUSES);

	for ( i = 0; i < MAX_BUSES && ifaceNum < maxIfaces; i++ ) {
		if ( i2c_buses[i] != -1 ) {
			hal_i2c_discover_devices( i2c_buses[i], &cfgArray[ifaceNum], &found );
			ifaceNum += found;
		}
	}
#endif

#ifdef ATCA_HAL_SWI
	int swi_buses[MAX_BUSES];
	memset( swi_buses, -1, sizeof(swi_buses));
	hal_swi_discover_buses(swi_buses, MAX_BUSES);
	for ( i = 0; i < MAX_BUSES && ifaceNum < maxIfaces; i++ ) {
		if ( swi_buses[i] != -1 ) {
			hal_swi_discover_devices( swi_buses[i], &cfgArray[ifaceNum], &found);
			ifaceNum += found;
		}
	}

#endif

#ifdef ATCA_HAL_UART
	int uart_buses[MAX_BUSES];
	memset( uart_buses, -1, sizeof(uart_buses));
	hal_uart_discover_buses(uart_buses, MAX_BUSES);
	for ( i = 0; i < MAX_BUSES && ifaceNum < maxIfaces; i++ ) {
		if ( uart_buses[i] != -1 ) {
			hal_uart_discover_devices( uart_buses[i], &cfgArray[ifaceNum], &found);
			ifaceNum += found;
		}
	}
#endif

#ifdef ATCA_HAL_KIT_CDC
	int cdc_buses[MAX_BUSES];
	memset( cdc_buses, -1, sizeof(cdc_buses));
	hal_kit_cdc_discover_buses(cdc_buses, MAX_BUSES);
	for ( i = 0; i < MAX_BUSES && ifaceNum < maxIfaces; i++ ) {
		if ( cdc_buses[i] != -1 ) {
			hal_kit_cdc_discover_devices( cdc_buses[i], &cfgArray[ifaceNum++], &found );
			ifaceNum += found;
		}
	}
#endif

#ifdef ATCA_HAL_KIT_HID
	int hid_buses[MAX_BUSES];
	memset( hid_buses, -1, sizeof(hid_buses));
	hal_kit_hid_discover_buses(hid_buses, MAX_BUSES);
	for ( i = 0; i < MAX_BUSES && ifaceNum < maxIfaces; i++ ) {
		if ( hid_buses[i] != -1 ) {
			hal_kit_hid_discover_devices( hid_buses[i], &cfgArray[ifaceNum++], &found);
			ifaceNum += found;
		}
	}
#endif

	return ATCA_SUCCESS;
}

/** \brief common cleanup code which idles the device after any operation
 *  \return ATCA_STATUS
 */
static ATCA_STATUS _atcab_exit(void)
{
	return atcab_idle();
}


/** \brief get the device revision information
 *  \param[out] revision - 4-byte storage for receiving the revision number from the device
 *  \return ATCA_STATUS
 */

ATCA_STATUS atcab_info( uint8_t *revision )
{
	ATCAPacket packet;
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint32_t execution_time;

	if ( !_gDevice )
		return ATCA_GEN_FAIL;

	// build an info command
	packet.param1 = INFO_MODE_REVISION;
	packet.param2 = 0;

	do {
		if ( (status = atInfo( _gCommandObj, &packet )) != ATCA_SUCCESS )
			break;

		execution_time = atGetExecTime( _gCommandObj, CMD_INFO);

		if ( (status = atcab_wakeup()) != ATCA_SUCCESS )
			break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ( (status = atreceive( _gIface, &(packet.data[0]), &(packet.rxsize) )) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

		memcpy( revision, &packet.data[1], 4 );  // don't include the receive length, only payload
	} while (0);

	if ( status != ATCA_COMM_FAIL )   // don't keep shoving more stuff at the chip if there's something wrong with comm
		_atcab_exit();

	return status;
}

/** \brief Get a 32 byte random number from the CryptoAuth device
 *	\param[out] rand_out ptr to 32 bytes of storage for random number
 *	\return status of the operation
 */
ATCA_STATUS atcab_random(uint8_t *rand_out)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	if ( !_gDevice )
		return ATCA_GEN_FAIL;

	// build an random command
	packet.param1 = RANDOM_SEED_UPDATE;
	packet.param2 = 0x0000;
	status = atRandom( _gCommandObj, &packet );
	execution_time = atGetExecTime( _gCommandObj, CMD_RANDOM);

	do {
		if ( (status = atcab_wakeup()) != ATCA_SUCCESS )
			break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS)
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &packet.rxsize)) != ATCA_SUCCESS)
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

		memcpy( rand_out, &packet.data[1], 32 );  // data[0] is the length byte of the response
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief generate a key on given slot
 *   \param[in]  slot    slot number where ECC key is configured
 *   \param[out] pubkey  64 bytes of returned public key for given slot
 *   \return ATCA_STATUS
 */
ATCA_STATUS atcab_genkey( int slot, uint8_t *pubkey )
{
	ATCAPacket packet;
	uint16_t execution_time = 0;
	ATCA_STATUS status = ATCA_GEN_FAIL;

	// build a genkey command
	packet.param1 = GENKEY_MODE_PRIVATE_KEY_GENERATE;   // a random private key is generated and stored in slot keyID
	packet.param2 = (uint16_t)slot;                     // slot and KeyID are the same thing

	do {
		if ( (status = atGenKey( _gCommandObj, &packet, false )) != ATCA_SUCCESS )
			break;

		execution_time = atGetExecTime( _gCommandObj, CMD_GENKEY);

		if ( (status = atcab_wakeup()) != ATCA_SUCCESS )
			break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &(packet.rxsize) )) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

		memcpy(pubkey, &packet.data[1], 64 );
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief Execute a pass-through Nonce command to initialize TempKey to the specified value
 *  \param[in] tempkey - pointer to 32 bytes of data which will be used to initialize TempKey
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_nonce(const uint8_t *tempkey)
{
	return atcab_challenge(tempkey);
}

/** \brief Initialize TempKey with a random Nonce
 *  \param[in] seed - pointer to 20 bytes of data which will be used to calculate TempKey
 *  \param[out] rand_out - pointer to 32 bytes of data that is the output of the Nonce command
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_nonce_rand(const uint8_t *seed, uint8_t* rand_out)
{
	return atcab_challenge_seed_update(seed, rand_out);
}

/** \brief send a challenge to the device (a pass-through nonce)
 *  \param[in] challenge - pointer to 32 bytes of data which will be sent as the challenge
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_challenge(const uint8_t *challenge)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	do {
		// Verify the inputs
		if (challenge == NULL) {
			status = ATCA_BAD_PARAM;
			break;
		}

		// build a nonce command (pass through mode)
		packet.param1 = NONCE_MODE_PASSTHROUGH;
		packet.param2 = 0x0000;
		memcpy( packet.data, challenge, 32 );

		if ((status = atNonce( _gCommandObj, &packet )) != ATCA_SUCCESS )
			break;

		execution_time = atGetExecTime( _gCommandObj, CMD_NONCE);

		if ((status = atcab_wakeup()) != ATCA_SUCCESS )
			break;

		// send the command
		if ((status = atsend( _gIface, (uint8_t*)&packet, packet.txsize)) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ((status = atreceive( _gIface, packet.data, &(packet.rxsize))) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief send a challenge to the device (a seed update nonce)
 *  \param[in] seed - pointer to 32 bytes of data which will be sent as the challenge
 *  \param[out] rand_out - points to space to receive random number
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_challenge_seed_update( const uint8_t *seed, uint8_t* rand_out )
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	do {
		// Verify the inputs
		if (seed == NULL || rand_out == NULL) {
			status = ATCA_BAD_PARAM;
			break;
		}

		// build a nonce command (pass through mode)
		packet.param1 = NONCE_MODE_SEED_UPDATE;
		packet.param2 = 0x0000;
		memcpy( packet.data, seed, 20 );

		if ((status = atNonce(_gCommandObj, &packet)) != ATCA_SUCCESS) break;

		execution_time = atGetExecTime(_gCommandObj, CMD_NONCE);

		if ((status = atcab_wakeup()) != ATCA_SUCCESS ) break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize)) != ATCA_SUCCESS ) break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ((status = atreceive(_gIface, packet.data, &(packet.rxsize))) != ATCA_SUCCESS) break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ((status = isATCAError(packet.data)) != ATCA_SUCCESS) break;

		memcpy(&rand_out[0], &packet.data[ATCA_RSP_DATA_IDX], 32);

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief read the serial number of the device
 *  \param[out] serial_number  pointer to space to receive serial number. This space should be 9 bytes long
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_read_serial_number(uint8_t* serial_number)
{
	// read config zone bytes 0-3 and 4-7, concatenate the two bits into serial_number
	uint8_t status = ATCA_GEN_FAIL;
	uint8_t bytes_read[ATCA_BLOCK_SIZE];
	uint8_t block = 0;
	uint8_t cpyIndex = 0;
	uint8_t offset = 0;

	do {
		memset(serial_number, 0x00, ATCA_SERIAL_NUM_SIZE);
		// Read first 32 byte block.  Copy the bytes into the config_data buffer
		block = 0;
		offset = 0;
		if ( (status = atcab_read_zone(ATCA_ZONE_CONFIG, 0, block, offset, bytes_read, ATCA_WORD_SIZE)) != ATCA_SUCCESS )
			break;

		memcpy(&serial_number[cpyIndex], bytes_read, ATCA_WORD_SIZE);
		cpyIndex += ATCA_WORD_SIZE;

		block = 0;
		offset = 2;
		if ( (status = atcab_read_zone(ATCA_ZONE_CONFIG, 0, block, offset, bytes_read, ATCA_WORD_SIZE)) != ATCA_SUCCESS )
			break;

		memcpy(&serial_number[cpyIndex], bytes_read, ATCA_WORD_SIZE);
		cpyIndex += ATCA_WORD_SIZE;

		block = 0;
		offset = 3;
		if ( (status = atcab_read_zone(ATCA_ZONE_CONFIG, 0, block, offset, bytes_read, ATCA_WORD_SIZE)) != ATCA_SUCCESS )
			break;

		memcpy(&serial_number[cpyIndex], bytes_read, 1);

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief verify a signature using CryptoAuth hardware (as opposed to an ECDSA software implementation)
 *  \param[in]  message    pointer
 *  \param[in]  signature  pointer
 *  \param[in]  pubkey     pointer
 *  \param[out] verified   boolean whether or not the challenge/signature/pubkey verified
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_verify_extern(const uint8_t *message, const uint8_t *signature, const uint8_t *pubkey, bool *verified)
{
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	do {
		*verified = false;

		// nonce passthrough
		if ( (status = atcab_challenge(message)) != ATCA_SUCCESS )
			break;

		// build a verify command
		packet.param1 = VERIFY_MODE_EXTERNAL; //verify the signature
		packet.param2 = VERIFY_KEY_P256;
		memcpy( &packet.data[0], signature, ATCA_SIG_SIZE);
		memcpy( &packet.data[64], pubkey, ATCA_PUB_KEY_SIZE);

		if ( (status = atVerify( _gCommandObj, &packet )) != ATCA_SUCCESS )
			break;

		execution_time = atGetExecTime( _gCommandObj, CMD_VERIFY );

		if ( (status = atcab_wakeup()) != ATCA_SUCCESS )
			break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &(packet.rxsize) )) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		status = isATCAError(packet.data);
		*verified = (status == 0);
		if (status == ATCA_CHECKMAC_VERIFY_FAILED)
			status = ATCA_SUCCESS; // Verify failed, but command succeeded
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief issues ecdh command
 *  \param[in] key_id slot of key for ECDH computation
 *  \param[in] pubkey public key
 *  \param[out] ret_ecdh - computed ECDH key - A buffer with size of ATCA_KEY_SIZE
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_ecdh(uint16_t key_id, const uint8_t* pubkey, uint8_t* ret_ecdh)
{
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	do {
		if (pubkey == NULL || ret_ecdh == NULL) {
			status = ATCA_BAD_PARAM;
			break;
		}
		memset(ret_ecdh, 0, ATCA_KEY_SIZE);

		// build a ecdh command
		packet.param1 = ECDH_PREFIX_MODE;
		packet.param2 = key_id;
		memcpy( packet.data, pubkey, ATCA_PUB_KEY_SIZE );

		if ( (status = atECDH( _gCommandObj, &packet )) != ATCA_SUCCESS ) break;

		execution_time = atGetExecTime( _gCommandObj, CMD_ECDH);

		if ( (status = atcab_wakeup()) != ATCA_SUCCESS ) break;

		if ( (status = atsend(_gIface, (uint8_t*)&packet, packet.txsize)) != ATCA_SUCCESS ) break;

		atca_delay_ms(execution_time);

		if ((status = atreceive(_gIface, packet.data, &packet.rxsize)) != ATCA_SUCCESS) break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS ) break;

		// The ECDH command may return a single byte. Then the CRC is copied into indices [1:2]
		memcpy(ret_ecdh, &packet.data[ATCA_RSP_DATA_IDX], ATCA_KEY_SIZE);

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief issues ecdh command
 *  \param[in] slotid slot of key for ECDH computation
 *  \param[in] pubkey public key
 *  \param[out] ret_ecdh - computed ECDH key - A buffer with size of ATCA_KEY_SIZE
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_ecdh_enc(uint16_t slotid, const uint8_t* pubkey, uint8_t* ret_ecdh, const uint8_t* enckey, const uint8_t enckeyid)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t cmpBuf[ATCA_WORD_SIZE];
	uint8_t block = 0;

	do {
		// Check the inputs
		if (pubkey == NULL || ret_ecdh == NULL || enckey == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Bad input parameters");
		}
		// Send the ECDH command with the public key provided
		if ((status = atcab_ecdh(slotid, pubkey, ret_ecdh)) != ATCA_SUCCESS) BREAK(status, "ECDH Failed");

		// ECDH may return a key or a single byte.  The atcab_ecdh() function performs a memset to 00 on ecdhRsp.
		memset(cmpBuf, 0, ATCA_WORD_SIZE);

		// Compare arbitrary bytes to see if they are 00
		if (memcmp(cmpBuf, &ret_ecdh[18], ATCA_WORD_SIZE) == 0) {
			// There is no ecdh key, check the value of the first byte for success
			if (ret_ecdh[0] != CMD_STATUS_SUCCESS) BREAK(status, "ECDH Command Execution Failure");

			// ECDH succeeded, perform an encrypted read from the n+1 slot.
			if ((status = atcab_read_enc(slotid + 1, block, ret_ecdh, enckey, enckeyid)) != ATCA_SUCCESS) BREAK(status, "Encrypte read failed");
		}
	} while (0);

	return status;
}


/** \brief Compute the address given the zone, slot, block, and offset
 *  \param[in] zone
 *  \param[in] slot
 *  \param[in] block
 *  \param[in] offset
 *  \param[in] addr
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_get_addr(uint8_t zone, uint8_t slot, uint8_t block, uint8_t offset, uint16_t* addr)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t memzone = zone & 0x03;

	if (addr == NULL)
		return ATCA_BAD_PARAM;
	if ((memzone != ATCA_ZONE_CONFIG) && (memzone != ATCA_ZONE_DATA) && (memzone != ATCA_ZONE_OTP))
		return ATCA_BAD_PARAM;
	do {
		// Initialize the addr to 00
		*addr = 0;
		// Mask the offset
		offset = offset & (uint8_t)0x07;
		if ((memzone == ATCA_ZONE_CONFIG) || (memzone == ATCA_ZONE_OTP)) {
			*addr = block << 3;
			*addr |= offset;
		}else {  // ATCA_ZONE_DATA
			*addr = slot << 3;
			*addr  |= offset;
			*addr |= block << 8;
		}
	} while (0);

	return status;
}



/** \brief Query to see if the specified slot is locked
 *  \param[in]  slot      The slot to query for locked (slot 0-15)
 *  \param[out] islocked  true if the specified slot is locked
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_is_slot_locked(uint8_t slot, bool *islocked)
{
	uint8_t ret = ATCA_GEN_FAIL;
	uint8_t slotLock_data[ATCA_WORD_SIZE];
	uint8_t lockable_data[ATCA_WORD_SIZE];
	uint8_t slotLock_idx = 0;
	uint8_t lockBit_idx = 0;
	uint8_t keyConfig_idx = 0;
	uint8_t lockableBit_idx = 5;

	do {
		// Read the word with the lock bytes ( SlotLock[2], RFU[2] ) (config block = 2, word offset = 6)
		if ( (ret = atcab_read_zone(ATCA_ZONE_CONFIG, 0, 2 /*block*/, 6 /*offset*/, slotLock_data, ATCA_WORD_SIZE)) != ATCA_SUCCESS )
			break;

		// Determine the slotlock and lockbit index into the word_data (slotLocked byte) based on the slot we are querying for
		if (slot < 8) {
			slotLock_idx = 0;
			lockBit_idx = slot;
		}else if ((slot > 8) && (slot <= ATCA_KEY_ID_MAX)) {
			slotLock_idx = 1;
			lockBit_idx = slot - 8;
		}else
			return ATCA_BAD_PARAM;

		// check the slotLocked[] bit is set to zero
		if ( ((slotLock_data[slotLock_idx] >> lockBit_idx) & 0x01) == 0x00 )
			*islocked = true;
		else{
			keyConfig_idx = (slot % 2) ? 2 : 0;
			// Read the word with the lockable bytes in keyConfig block (config block = 3, word offset = depending on the slot)
			if ( (ret = atcab_read_zone(ATCA_ZONE_CONFIG, 0, 3 /*block*/, (slot >> 1) /*offset*/, lockable_data, ATCA_WORD_SIZE)) != ATCA_SUCCESS )
				break;

			if (((lockable_data[keyConfig_idx] >> lockableBit_idx) & 0x01) == 0x00)
				*islocked = true;
			else
				*islocked = false;
		}
	} while (0);

	// all atcab commands within this method follow the wake/idle pattern, no non-atcab methods called, so don't need to _atcab_exit()
	return ret;
}

/** \brief Query to see if the specified zone is locked
 *  \param[in]  zone      The zone to query for locked (use LOCK_ZONE_CONFIG or LOCK_ZONE_DATA)
 *  \param[out] islocked  true if the specified zone is locked
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_is_locked(uint8_t zone, bool *islocked)
{
	uint8_t ret = ATCA_GEN_FAIL;
	uint8_t word_data[ATCA_WORD_SIZE];
	uint8_t zone_idx = 2;

	do {
		// Read the word with the lock bytes (UserExtra, Selector, LockValue, LockConfig) (config block = 2, word offset = 5)
		if ( (ret = atcab_read_zone(ATCA_ZONE_CONFIG, 0, 2 /*block*/, 5 /*offset*/, word_data, ATCA_WORD_SIZE)) != ATCA_SUCCESS )
			break;

		// Determine the index into the word_data based on the zone we are querying for
		if (zone == LOCK_ZONE_DATA) zone_idx = 2;
		if (zone == LOCK_ZONE_CONFIG) zone_idx = 3;

		// Set the locked return variable base on the value.
		if (word_data[zone_idx] == 0)
			*islocked = true;
		else
			*islocked = false;

	} while (0);

	return ret;
}

/** \brief Write either 4 or 32 bytes of data into a device zone.
 *
 *  See ECC108A datasheet, datazone address values, table 9-8
 *
 *  \param[in] zone    Device zone to write to (0=config, 1=OTP, 2=data).
 *  \param[in] slot    If writing to the data zone, whit is the slot to write to, otherwise it should be 0.
 *  \param[in] block   32-byte block to write to.
 *  \param[in] offset  4-byte word within the specified block to write to. If performing a 32-byte write, this should
 *                     be 0.
 *  \param[in] data    Data to be written.
 *  \param[in] len     Number of bytes to be written. Must be either 4 or 32.
 *  \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_write_zone(uint8_t zone, uint8_t slot, uint8_t block, uint8_t offset, const uint8_t *data, uint8_t len)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t addr;
	uint16_t execution_time = 0;

	// Check the input parameters
	if (data == NULL)
		return ATCA_BAD_PARAM;

	if ( len != 4 && len != 32 )
		return ATCA_BAD_PARAM;

	do {
		// The get address function checks the remaining variables
		if ( (status = atcab_get_addr(zone, slot, block, offset, &addr)) != ATCA_SUCCESS )
			break;

		// If there are 32 bytes to write, then xor the bit into the mode
		if (len == ATCA_BLOCK_SIZE)
			zone = zone | ATCA_ZONE_READWRITE_32;

		// build a write command
		packet.param1 = zone;
		packet.param2 = addr;
		memcpy( packet.data, data, len );

		if ( (status = atWrite( _gCommandObj, &packet )) != ATCA_SUCCESS )
			break;

		execution_time = atGetExecTime( _gCommandObj, CMD_WRITEMEM);

		if ( (status = atcab_wakeup()) != ATCA_SUCCESS )
			break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &(packet.rxsize) )) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		status = isATCAError(packet.data);

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief read either 4 or 32 bytes of data into given slot
 *
 *  for 32 byte read, offset is ignored
 *  data receives the contents read from the slot
 *
 *  data zone must be locked and the slot configuration must not be secret for a slot to be successfully read
 *
 *  \param[in] zone
 *  \param[in] slot
 *  \param[in] block
 *  \param[in] offset
 *  \param[in] data
 *  \param[in] len  Must be either 4 or 32
 *  returns ATCA_STATUS
 */
ATCA_STATUS atcab_read_zone(uint8_t zone, uint8_t slot, uint8_t block, uint8_t offset, uint8_t *data, uint8_t len)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	ATCAPacket packet;
	uint16_t addr;
	uint16_t execution_time = 0;

	do {
		// Check the input parameters
		if (data == NULL)
			return ATCA_BAD_PARAM;

		if ( len != 4 && len != 32 )
			return ATCA_BAD_PARAM;

		// The get address function checks the remaining variables
		if ( (status = atcab_get_addr(zone, slot, block, offset, &addr)) != ATCA_SUCCESS )
			break;

		// If there are 32 bytes to write, then xor the bit into the mode
		if (len == ATCA_BLOCK_SIZE)
			zone = zone | ATCA_ZONE_READWRITE_32;

		// build a read command
		packet.param1 = zone;
		packet.param2 = addr;

		if ( (status = atRead( _gCommandObj, &packet )) != ATCA_SUCCESS )
			break;

		execution_time = atGetExecTime( _gCommandObj, CMD_READMEM);

		if ( (status = atcab_wakeup()) != ATCA_SUCCESS ) break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &(packet.rxsize) )) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

		memcpy( data, &packet.data[1], len );
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief Read 32 bytes of data from the given slot.
 *		The function returns clear text bytes. Encrypted bytes are read over the wire, then subsequently decrypted
 *		Data zone must be locked and the slot configuration must be set to encrypted read for the block to be successfully read
 *  \param[in]  slotid    The slot id for the encrypted read
 *  \param[in]  block     The block id in the specified slot
 *  \param[out] data      The 32 bytes of clear text data that was read encrypted from the slot, then decrypted
 *  \param[in]  enckey    The key to encrypt with for writing
 *  \param[in]  enckeyid  The keyid of the parent encryption key
 *  returns ATCA_STATUS
 */
ATCA_STATUS atcab_read_enc(uint8_t slotid, uint8_t block, uint8_t *data, const uint8_t* enckey, const uint16_t enckeyid)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t zone = ATCA_ZONE_DATA | ATCA_ZONE_READWRITE_32;
	atca_nonce_in_out_t nonceParam;
	atca_gen_dig_in_out_t genDigParam;
	atca_temp_key_t tempkey;
	uint8_t numin[NONCE_NUMIN_SIZE] = { 0 };
	uint8_t randout[RANDOM_NUM_SIZE] = { 0 };
	int i = 0;

	do {
		// Verify inputs parameters
		if (data == NULL || enckey == NULL) {
			status = ATCA_BAD_PARAM;
			break;
		}

		// Random Nonce inputs
		nonceParam.mode = NONCE_MODE_SEED_UPDATE;
		nonceParam.num_in = (uint8_t*)&numin;
		nonceParam.rand_out = (uint8_t*)&randout;
		nonceParam.temp_key = &tempkey;

		// Send the random Nonce command
		if ((status = atcab_nonce_rand(numin, randout)) != ATCA_SUCCESS) BREAK(status, "Nonce failed");

		// Calculate Tempkey
		if ((status = atcah_nonce(&nonceParam)) != ATCA_SUCCESS) BREAK(status, "Calc TempKey failed");

		// GenDig inputs
		genDigParam.key_id = enckeyid;
		genDigParam.stored_value = enckey;
		genDigParam.zone = GENDIG_ZONE_DATA;
		genDigParam.temp_key = &tempkey;

		// Send the GenDig command
		if ((status = atcab_gendig(GENDIG_ZONE_DATA, enckeyid)) != ATCA_SUCCESS) BREAK(status, "GenDig failed");

		// Calculate Tempkey
		if ((status = atcah_gen_dig(&genDigParam)) != ATCA_SUCCESS) BREAK(status, "");

		// Read Encrypted
		if ((status = atcab_read_zone(zone, slotid, block, 0, data, ATCA_BLOCK_SIZE)) != ATCA_SUCCESS) BREAK(status, "Read encrypted failed");

		// Decrypt
		for (i = 0; i < ATCA_BLOCK_SIZE; i++)
			data[i] = data[i] ^ tempkey.value[i];

		status = ATCA_SUCCESS;

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief Write 32 bytes of data into given slot.
 *		The function takes clear text bytes, but encrypts them for writing over the wire
 *		Data zone must be locked and the slot configuration must be set to encrypted write for the block to be successfully written
 *  \param[in] slotid
 *  \param[in] block
 *  \param[in] data      The 32 bytes of clear text data to be written to the slot
 *  \param[in] enckey    The key to encrypt with for writing
 *  \param[in] enckeyid  The keyid of the parent encryption key
 *  returns ATCA_STATUS
 */
ATCA_STATUS atcab_write_enc(uint8_t slotid, uint8_t block, const uint8_t *data, const uint8_t* enckey, const uint16_t enckeyid)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t i = 0;
	uint8_t zone = ATCA_ZONE_DATA | ATCA_ZONE_READWRITE_32;
	atca_nonce_in_out_t nonceParam;
	atca_gen_dig_in_out_t genDigParam;
	atca_temp_key_t tempkey;
	uint8_t numin[NONCE_NUMIN_SIZE] = { 0 };
	uint8_t randout[RANDOM_NUM_SIZE] = { 0 };
	uint8_t cipher_text[ATCA_KEY_SIZE] = { 0 };
	ATCAPacket packet;
	uint16_t addr;
	uint16_t execution_time = 0;

	do {
		// Verify inputs parameters
		if (data == NULL || enckey == NULL) {
			status = ATCA_BAD_PARAM;
			break;
		}
		// Random Nonce inputs
		nonceParam.mode = NONCE_MODE_SEED_UPDATE;
		nonceParam.num_in = (uint8_t*)&numin;
		nonceParam.rand_out = (uint8_t*)&randout;
		nonceParam.temp_key = &tempkey;

		// Send the random Nonce command
		if ((status = atcab_nonce_rand(numin, randout)) != ATCA_SUCCESS) BREAK(status, "Nonce failed");

		// Calculate Tempkey
		if ((status = atcah_nonce(&nonceParam)) != ATCA_SUCCESS) BREAK(status, "Calc TempKey failed");

		// GenDig inputs
		genDigParam.key_id = enckeyid;
		genDigParam.stored_value = (uint8_t*)enckey;
		genDigParam.zone = GENDIG_ZONE_DATA;
		genDigParam.temp_key = &tempkey;

		// Send the GenDig command
		if ((status = atcab_gendig(GENDIG_ZONE_DATA, enckeyid)) != ATCA_SUCCESS) BREAK(status, "GenDig failed");

		// Calculate Tempkey
		if ((status = atcah_gen_dig(&genDigParam)) != ATCA_SUCCESS) BREAK(status, "");

		// Xoring plain text with session key
		for (i = 0; i < ATCA_KEY_SIZE; i++)
			cipher_text[i] = data[i] ^ tempkey.value[i];

		// The get address function checks the remaining variables
		if ((status = atcab_get_addr(ATCA_ZONE_DATA, slotid, block, 0, &addr)) != ATCA_SUCCESS) BREAK(status, "Get address failed");

		// Calculate Auth MAC
		genDigParam.zone = zone;
		genDigParam.key_id = addr;
		genDigParam.stored_value = (uint8_t*)data;
		genDigParam.temp_key = &tempkey;

		if ((status = atcah_gen_mac(&genDigParam)) != ATCA_SUCCESS) BREAK(status, "Calculate Auth MAC failed");

		// build a write command for encrypted writes
		packet.param1 = zone;
		packet.param2 = addr;
		memcpy(packet.data, cipher_text, ATCA_KEY_SIZE);
		memcpy(&packet.data[ATCA_KEY_SIZE], tempkey.value, ATCA_KEY_SIZE);

		if ((status = atWriteEnc(_gCommandObj, &packet)) != ATCA_SUCCESS) BREAK(status, "format write command bytes failed");

		execution_time = atGetExecTime(_gCommandObj, CMD_WRITEMEM);

		if ((status = atcab_wakeup()) != ATCA_SUCCESS) BREAK(status, "wakeup failed");

		// send the command
		if ((status = atsend(_gIface, (uint8_t*)&packet, packet.txsize)) != ATCA_SUCCESS) BREAK(status, "send write command bytes failed");

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ((status = atreceive(_gIface, packet.data, &(packet.rxsize))) != ATCA_SUCCESS) BREAK(status, "receive write command bytes failed");

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		status = isATCAError(packet.data);

	} while (0);

	_atcab_exit();
	return status;
}


/** \brief read the config zone by block by block
 *  for 32 byte read, offset is ignored
 *  data receives the contents read from the slot
 *  Config zone can be read regardless of it being locked or unlocked
 *  \param[in] config_data pointer to buffer containing a contiguous set of bytes to read from the config zone
 *  returns ATCA_STATUS
 */
ATCA_STATUS atcab_read_ecc_config_zone(uint8_t* config_data)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint8_t zone = 0, block = 0, offset = 0, slot = 0, index = 0;
	uint16_t addr = 0x0000;

	//reading the zone block by block until word 16 (block 2, offset 6)
	do {
		if ((block == 2) && (offset <= 7)) {
			// read 32 bytes at once
			packet.param1 = ATCA_ZONE_CONFIG;

			// compute the word addr and build the read command
			if ( (status = atcab_get_addr(zone, slot, block, offset, &addr)) != ATCA_SUCCESS )
				break;

			packet.param2 =  addr;
			status = atRead(_gCommandObj, &packet);
			execution_time = atGetExecTime( _gCommandObj, CMD_READMEM);

			if ( (status = atcab_wakeup()) != ATCA_SUCCESS )
				break;

			// send the command
			if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
				break;

			// delay the appropriate amount of time for command to execute
			atca_delay_ms(execution_time);

			memset(packet.data, 0x00, 130);

			// receive the response
			if ( (status = atreceive( _gIface, packet.data, &packet.rxsize)) != ATCA_SUCCESS )
				break;

			// Check response size
			if (packet.rxsize < 4) {
				if (packet.rxsize > 0)
					status = ATCA_RX_FAIL;
				else
					status = ATCA_RX_NO_RESPONSE;
				break;
			}

			if ( (status = atcab_idle()) != ATCA_SUCCESS )
				break;

			// check for error in response
			if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
				break;

			// update the word address after reading each block
			++offset;
			if ((block == 2) && (offset > 7))
				block = 3;

			// copy the contents to a config data buffer
			memcpy(&config_data[index], &packet.data[1], ATCA_WORD_SIZE );
			index += ATCA_WORD_SIZE;
		}else {
			// build a read command (read from the start of zone)
			offset = 0;
			// read 32 bytes at once
			packet.param1 = ATCA_ZONE_CONFIG | ATCA_ZONE_READWRITE_32;

			// compute the word addr and build the read command
			if ( (status = atcab_get_addr(zone, slot, block, offset, &addr)) != ATCA_SUCCESS )
				break;

			packet.param2 =  addr;
			status = atRead(_gCommandObj, &packet);
			execution_time = atGetExecTime( _gCommandObj, CMD_READMEM);

			if ( (status = atcab_wakeup()) != ATCA_SUCCESS ) break;

			// send the command
			if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
				break;

			// delay the appropriate amount of time for command to execute
			atca_delay_ms(execution_time);

			memset(packet.data, 0x00, sizeof(packet.data));

			// receive the response
			if ( (status = atreceive( _gIface, packet.data, &packet.rxsize)) != ATCA_SUCCESS )
				break;

			// Check response size
			if (packet.rxsize < 4) {
				if (packet.rxsize > 0)
					status = ATCA_RX_FAIL;
				else
					status = ATCA_RX_NO_RESPONSE;
				break;
			}

			if ( (status = atcab_idle()) != ATCA_SUCCESS )
				break;

			// check for error in response
			if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
				break;

			// update the word address after reading each block
			++block;

			// copy the contents to a config data buffer
			memcpy(&config_data[index], &packet.data[1], ATCA_BLOCK_SIZE );
			index += ATCA_BLOCK_SIZE;
		}

	} while (block <= 3);

	_atcab_exit();
	return status;
}

/** \brief given an ECC configuration zone buffer, write its parts to the device's config zone
 *  \param[in] config_data pointer to buffer containing a contiguous set of bytes to write to the config zone
 *  \returns ATCA_STATUS
 */
ATCA_STATUS atcab_write_ecc_config_zone(const uint8_t* config_data)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint8_t zone = 0, block = 0, offset = 0, slot = 0, index = 0;
	uint16_t addr = 0;

	// write the ecc zone one block at a time starting after address 0x04 (block 0, offset 4)
	offset = 4;
	do {
		if ((block == 0) || (block == 2)) {

			if (offset <= 7) {
				memset(packet.data, 0x00, 130);
				// read 4 bytes at once
				packet.param1 = ATCA_ZONE_CONFIG;
				// build a write command (write from the start)
				if ( (status = atcab_get_addr(zone, slot, block, offset, &addr)) != ATCA_SUCCESS )
					break;

				packet.param2 =  addr;
				memcpy(&packet.data[0], &config_data[index + 16], ATCA_WORD_SIZE);
				index += ATCA_WORD_SIZE;
				status = atWrite(_gCommandObj, &packet);
				execution_time = atGetExecTime( _gCommandObj, CMD_WRITEMEM);

				if ( (status = atcab_wakeup()) != ATCA_SUCCESS ) break;

				// send the command
				if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
					break;

				// delay the appropriate amount of time for command to execute
				atca_delay_ms(execution_time);

				// receive the response
				if ( (status = atreceive( _gIface, packet.data, &packet.rxsize)) != ATCA_SUCCESS )
					break;

				// Check response size
				if (packet.rxsize < 4) {
					if (packet.rxsize > 0)
						status = ATCA_RX_FAIL;
					else
						status = ATCA_RX_NO_RESPONSE;
					break;
				}

				if ( (status = atcab_idle()) != ATCA_SUCCESS ) break;

				if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
					break;

				// update the offset address after reading each block
				++offset;
				if ((block == 2) && (offset == 5)) {
					// words above (block 2 offset 5 cant be written)
					++offset; index += ATCA_WORD_SIZE;
				}
			}else {
				// update the word address after completely reading each block
				++block; offset = 0;
			}
		}else {
			memset(packet.data, 0x00, 130);
			// read 32 bytes at once
			packet.param1 = ATCA_ZONE_CONFIG | ATCA_ZONE_READWRITE_32;
			// build a write command (write from the start)
			atcab_get_addr(zone, slot, block, offset, &addr);
			packet.param2 =  addr;
			memcpy(&packet.data[0], &config_data[index + 16], ATCA_BLOCK_SIZE);
			index += ATCA_BLOCK_SIZE;
			if ( (status = atWrite(_gCommandObj, &packet)) != ATCA_SUCCESS )
				break;

			execution_time = atGetExecTime( _gCommandObj, CMD_WRITEMEM);

			if ( (status = atcab_wakeup()) != ATCA_SUCCESS ) break;

			// send the command
			if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
				break;

			// delay the appropriate amount of time for command to execute
			atca_delay_ms(execution_time);

			// receive the response
			if ( (status = atreceive( _gIface, packet.data, &packet.rxsize)) != ATCA_SUCCESS )
				break;

			// Check response size
			if (packet.rxsize < 4) {
				if (packet.rxsize > 0)
					status = ATCA_RX_FAIL;
				else
					status = ATCA_RX_NO_RESPONSE;
				break;
			}

			if ( (status = atcab_idle()) != ATCA_SUCCESS ) break;

			if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
				break;

			// update the word address after completely reading each block
			++block; offset = 0;
		}

	} while (block <= 3);

	_atcab_exit();
	return status;
}

/** \brief given an SHA configuration zone buffer, read its parts from the device's config zone
 *  \param[out] config_data pointer to buffer containing a contiguous set of bytes to write to the config zone
 *  \returns ATCA_STATUS
 */
ATCA_STATUS atcab_read_sha_config_zone(uint8_t* config_data)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	do {

		// Verify the inputs
		if ( config_data == NULL ) {
			status = ATCA_BAD_PARAM;
			break;
		}

		status = atcab_read_bytes_zone(ATSHA204A, ATCA_ZONE_CONFIG, 0x00, ATCA_SHA_CONFIG_SIZE, config_data);
		if ( status != ATCA_SUCCESS )
			break;

	} while (0);

	return status;
}

/** \brief given an SHA configuration zone buffer, write its parts to the device's config zone
 *  \param[in] config_data pointer to buffer containing a contiguous set of bytes to write to the config zone
 *  \returns ATCA_STATUS
 */
ATCA_STATUS atcab_write_sha_config_zone(const uint8_t* config_data)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	do {

		// Verify the inputs
		if ( config_data == NULL ) {
			status = ATCA_BAD_PARAM;
			break;
		}

		status = atcab_write_bytes_zone(ATSHA204A, ATCA_ZONE_CONFIG, 0x10, &config_data[16], 68);
		if ( status != ATCA_SUCCESS )
			break;

	} while (0);

	return status;
}

/** \brief given an SHA configuration zone buffer and dev type, read its parts from the device's config zone
 *  \param[in]  dev_type     device type
 *  \param[out] config_data  pointer to buffer containing a contiguous set of bytes to write to the config zone
 *  \returns ATCA_STATUS
 */
ATCA_STATUS atcab_read_config_zone(ATCADeviceType dev_type, uint8_t* config_data)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	do {

		// Verify the inputs
		if ( config_data == NULL ) {
			status = ATCA_BAD_PARAM;
			break;
		}

		if (dev_type == ATSHA204A)
			status = atcab_read_bytes_zone(dev_type, ATCA_ZONE_CONFIG, 0x00, ATCA_SHA_CONFIG_SIZE, config_data);
		else
			status = atcab_read_bytes_zone(dev_type, ATCA_ZONE_CONFIG, 0x00, ATCA_CONFIG_SIZE, config_data);

		if ( status != ATCA_SUCCESS )
			break;

	} while (0);

	return status;
}

/** \brief given an SHA configuration zone buffer and dev type, write its parts to the device's config zone
 *  \param[in] config_data pointer to buffer containing a contiguous set of bytes to write to the config zone
 *  \returns ATCA_STATUS
 */
ATCA_STATUS atcab_write_config_zone(ATCADeviceType dev_type, const uint8_t* config_data)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	do {

		// Verify the inputs
		if ( config_data == NULL ) {
			status = ATCA_BAD_PARAM;
			break;
		}

		if (dev_type == ATSHA204A)
			status = atcab_write_bytes_zone(dev_type, ATCA_ZONE_CONFIG, 0x00, config_data, ATCA_SHA_CONFIG_SIZE);
		else
			status = atcab_write_bytes_zone(dev_type, ATCA_ZONE_CONFIG, 0x00, config_data, ATCA_CONFIG_SIZE);

		if ( status != ATCA_SUCCESS )
			break;

	} while (0);

	return status;
}

/** \brief This function compares all writable bytes in the configuration zone that is passed in to the bytes on the device
 *
 *  \param[in]  config_data  pointer to all 128 bytes in configuration zone. Not used if NULL.
 *  \param[out] same_config  pointer to boolean status whether config data passed in matches the actual config zone
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_cmp_config_zone(uint8_t* config_data, bool* same_config)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t device_config_data[ATCA_CONFIG_SIZE];

	do {
		// Check the inputs
		if ((config_data == NULL) || (same_config == NULL)) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Invalid Parameters");
		}
		// Set the boolean to false
		*same_config = false;

		// Read all of the configuration bytes from the device
		if ((status = atcab_read_ecc_config_zone(device_config_data)) != ATCA_SUCCESS) BREAK(status, "Read config zone failed");

		// Compare writable bytes 16-51 & writable bytes 90-127.
		// Skip the counter & LastKeyUse bytes [52-83]
		if (memcmp(&device_config_data[16], &config_data[16], 52 - 16) == 0
		    && memcmp(&device_config_data[90], &config_data[90], 128 - 90) == 0) {
			*same_config = true;
			break;
		}
	} while (0);
	return status;
}


/** \brief lock the ATCA ECC config zone.  config zone must be unlocked for the zone to be successfully locked
 *
 *  \param[in] lock_response
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_lock_config_zone(uint8_t* lock_response)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	// build command for lock zone and send
	packet.param1 = LOCK_ZONE_NO_CRC | LOCK_ZONE_CONFIG;

	do {
		if ( (status = atLock(_gCommandObj, &packet)) != ATCA_SUCCESS ) break;

		execution_time = atGetExecTime( _gCommandObj, CMD_LOCK);

		if ( (status = atcab_wakeup()) != ATCA_SUCCESS ) break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &packet.rxsize)) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		//check the response for error
		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

		memcpy(lock_response, &packet.data[1], 1);
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief lock the ATCA ECC Data zone.
 *
 *	ConfigZone must be locked and DataZone must be unlocked for the zone to be successfully locked
 *
 *  \param[in] lock_response
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_lock_data_zone(uint8_t* lock_response)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	// build command for lock zone and send
	packet.param1 = LOCK_ZONE_NO_CRC | LOCK_ZONE_DATA;
	packet.param2 = 0x0000;

	do {
		status = atLock(_gCommandObj, &packet);
		execution_time = atGetExecTime( _gCommandObj, CMD_LOCK);

		if ((status = atcab_wakeup()) != ATCA_SUCCESS ) break;

		// send the command
		if ((status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ((status = atreceive( _gIface, packet.data, &packet.rxsize)) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		//check the response for error
		if ((status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

		memcpy(lock_response, &packet.data[1], 1);
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief lock the ATCA ECC Data Slot
 *  ConfigZone must be locked and DataZone may or may not be locked for a individual data slot to be locked
 *
 *  \param[in] slot to be locked in data zone
 *  \param[in] lock_response pointer to the lock response from the chip - 0 is successful lock
 *  \return ATAC_STATUS
 */
ATCA_STATUS atcab_lock_data_slot(uint8_t slot, uint8_t* lock_response)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	// build command for lock slot and send
	packet.param1 = (slot << 2) | LOCK_ZONE_DATA_SLOT;
	packet.param2 = 0x0000;

	do {
		if ( (status = atLock(_gCommandObj, &packet)) != ATCA_SUCCESS ) break;

		execution_time = atGetExecTime( _gCommandObj, CMD_LOCK);

		if ( (status = atcab_wakeup()) != ATCA_SUCCESS ) break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &packet.rxsize)) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		//check the response for error
		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

		memcpy(lock_response, &packet.data[1], 1);
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief sign a buffer using private key in given slot, stuff the signature
 *  \param[in] slot
 *  \param[in] msg should point to a 32 byte buffer
 *  \param[out] signature of msg. signature should point to buffer SIGN_RSP_SIZE big
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_sign(uint16_t slot, const uint8_t *msg, uint8_t *signature)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint8_t randomnum[64];

	if ( !_gDevice )
		return ATCA_GEN_FAIL;

	do {
		if ( (status = atcab_random(randomnum)) != ATCA_SUCCESS ) break;
		if ( (status = atcab_challenge( msg )) != ATCA_SUCCESS ) break;

		// build sign command
		packet.param1 = SIGN_MODE_EXTERNAL;
		packet.param2 = slot;
		if ( (status = atSign( _gCommandObj, &packet )) != ATCA_SUCCESS )
			break;

		execution_time = atGetExecTime( _gCommandObj, CMD_SIGN);

		if ( (status != atcab_wakeup()) != ATCA_SUCCESS ) break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &(packet.rxsize))) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		// check for response
		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

		memcpy( signature, &packet.data[1], ATCA_SIG_SIZE );
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief Issues a GenDig command to SHA256 hash the source data indicated by zone with the
 *  contents of TempKey.  See the CryptoAuth datasheet for your chip to see what the values of zone
 *  correspond to.
 *  \param[in] zone - designates the source of the data to hash with TempKey
 *  \param[in] key_id - indicates the key, OTP block or message order for shared nonce mode
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_gendig(uint8_t zone, uint16_t key_id)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t otherDat[GENDIG_OTHER_DATA_SIZE] = { 0 };

	do {
		// Verify that we a valid device is present
		if (!_gDevice) return ATCA_GEN_FAIL;

		// Call the atcab_gendig_host() function
		if ((status = atcab_gendig_host(zone, key_id, otherDat, GENDIG_OTHER_DATA_SIZE)) != ATCA_SUCCESS ) BREAK(status, "GenDig failed");

	} while (0);
	return status;
}

/** \brief Similar to atcab_gendig except this method does the operation in software on the host.
 *  \param[in] zone - designates the source of the data to hash with TempKey
 *  \param[in] key_id - indicates the key, OTP block or message order for shared nonce mode
 *  \param[in] other_data - pointer to 4 or 32 bytes of data depending upon the mode
 *  \param[in] len - length of data
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_gendig_host(uint8_t zone, uint16_t key_id, uint8_t *other_data, uint8_t len)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	bool hasMACKey = 0;

	if ( !_gDevice || other_data == NULL )
		return ATCA_GEN_FAIL;

	do {

		// build gendig command
		packet.param1 = zone;
		packet.param2 = key_id;

		if ( packet.param1 == 0x03 && len == 0x20)
			memcpy(&packet.data[0], &other_data[0], ATCA_WORD_SIZE);
		else if ( packet.param1 == 0x02 && len == 0x20) {
			memcpy(&packet.data[0], &other_data[0], ATCA_WORD_SIZE);
			hasMACKey = true;
		}

		if ( (status = atGenDig( _gCommandObj, &packet, hasMACKey)) != ATCA_SUCCESS )
			break;

		execution_time = atGetExecTime( _gCommandObj, CMD_GENDIG);

		if ( (status != atcab_wakeup()) != ATCA_SUCCESS ) break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &(packet.rxsize))) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		// check for response
		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief reads a signature found in one of slots 8 through F.
 *  \param[in] slot8toF - which slot to read
 *  \param[out] sig - pointer to the space to receive the signature found in the slot
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_read_sig(uint8_t slot8toF, uint8_t *sig)
{
	uint8_t ret = ATCA_GEN_FAIL;
	uint8_t read_buf[ATCA_BLOCK_SIZE];
	uint8_t block = 0;
	uint8_t offset = 0;
	uint8_t cpyIndex = 0;

	do {
		// Check the pointers
		if (sig == NULL) break;

		// Check the value of the slot
		if (slot8toF < 8 || slot8toF > 0xF) break;

		// Read the first block
		block = 0;
		if ( (ret = atcab_read_zone(ATCA_ZONE_DATA, slot8toF, block, offset, read_buf, ATCA_BLOCK_SIZE)) != ATCA_SUCCESS )
			break;

		// Copy.  first 32 bytes
		memcpy(&sig[cpyIndex], &read_buf[0], ATCA_BLOCK_SIZE);
		cpyIndex += ATCA_BLOCK_SIZE;

		// Read the second block
		block = 1;
		if ( (ret = atcab_read_zone(ATCA_ZONE_DATA, slot8toF, block, offset, read_buf, ATCA_BLOCK_SIZE)) != ATCA_SUCCESS )
			break;

		// Copy.  next 32 bytes
		memcpy(&sig[cpyIndex], &read_buf[0], ATCA_BLOCK_SIZE);
		cpyIndex += ATCA_BLOCK_SIZE;

	} while (0);

	return ret;
}

/** \brief returns a public key found in a designated slot.  The slot must be configured as a slot with a private key.
 *  This method will use GenKey t geenrate the corresponding public key from the private key in the given slot.
 *  \param[in] privSlotId ID of the private key slot
 *  \param[out] pubkey - pointer to space receiving the contents of the public key that was generated
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_calc_pubkey(uint8_t privSlotId, uint8_t *pubkey)
{
	return atcab_get_pubkey(privSlotId, pubkey);
}

/** \brief returns a public key found in a designated slot.  The slot must be configured as a slot with a private key.
 *  This method will use GenKey t geenrate the corresponding public key from the private key in the given slot.
 *  \param[in] privSlotId ID of the private key slot
 *  \param[out] pubkey - pointer to space receiving the contents of the public key that was generated
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_get_pubkey(uint8_t privSlotId, uint8_t *pubkey)
{
	ATCAPacket packet;
	uint16_t execution_time = 0;
	ATCA_STATUS status = ATCA_GEN_FAIL;

	do {
		// build a genkey command
		packet.param1 = GENKEY_MODE_PUBLIC;
		packet.param2 = (uint16_t)(privSlotId);

		if ( (status = atGenKey( _gCommandObj, &packet, false )) != ATCA_SUCCESS ) break;

		execution_time = atGetExecTime( _gCommandObj, CMD_GENKEY);

		if ( (status = atcab_wakeup()) != ATCA_SUCCESS ) break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &(packet.rxsize) )) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

		// copy the response public key data
		memcpy(pubkey, &packet.data[1], 64 );
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief write a P256 private key in given slot using mac computation
 *  \param[in] slot
 *  \param[in] priv_key first 4 bytes of 36 bytes should be zero
 *  \param[in] write_key_slot slot to make a session key
 *  \param[in] write_key key to make a session key
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_priv_write(uint8_t slot, const uint8_t priv_key[36], uint8_t write_key_slot, const uint8_t write_key[32])
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	atca_nonce_in_out_t nonceParam;
	atca_gen_dig_in_out_t genDigParam;
	atca_write_mac_in_out_t hostMacParam;
	atca_temp_key_t tempkey;
	uint8_t numin[NONCE_NUMIN_SIZE] = { 0 };
	uint8_t randout[RANDOM_NUM_SIZE] = { 0 };
	uint8_t cipher_text[36] = { 0 };
	uint8_t host_mac[MAC_SIZE] = { 0 };
	uint16_t execution_time = 0;
	uint8_t privKey[36];
	uint8_t writeKey[32];

	if (slot > 15 || priv_key == NULL)
		return ATCA_BAD_PARAM;

	do {

		if (write_key == NULL) {
			// Caller requested an unencrypted PrivWrite, which is only allowed when the data zone is unlocked
			// build an PrivWrite command
			packet.param1 = 0x00;                   // Mode is unencrypted write
			packet.param2 = slot;                   // Key ID
			memcpy(&packet.data[0], priv_key, 36);  // Private key
			memset(&packet.data[36], 0, 32);        // MAC (ignored for unencrypted write)
		}else {
			// Copy the buffers to honor the const designation
			memcpy(privKey, priv_key, 36);
			memcpy(writeKey, write_key, 32);

			// Send the random Nonce command
			if ((status = atcab_nonce_rand(numin, randout)) != ATCA_SUCCESS)
				break;

			// Calculate Tempkey
			nonceParam.mode = NONCE_MODE_SEED_UPDATE;
			nonceParam.num_in = numin;
			nonceParam.rand_out = randout;
			nonceParam.temp_key = &tempkey;
			if ((status = atcah_nonce(&nonceParam)) != ATCA_SUCCESS)
				break;

			// Send the GenDig command
			if ((status = atcab_gendig_host(GENDIG_ZONE_DATA, write_key_slot, tempkey.value, 32)) != ATCA_SUCCESS)
				break;

			// Calculate Tempkey
			genDigParam.zone = GENDIG_ZONE_DATA;
			genDigParam.key_id = write_key_slot;
			genDigParam.stored_value = writeKey;
			genDigParam.temp_key = &tempkey;
			if ((status = atcah_gen_dig(&genDigParam)) != ATCA_SUCCESS)
				break;

			// Calculate Auth MAC and cipher text
			hostMacParam.zone = PRIVWRITE_MODE_ENCRYPT;
			hostMacParam.key_id = slot;
			hostMacParam.encryption_key = &privKey[4];
			hostMacParam.input_data = privKey;
			hostMacParam.encrypted_data = cipher_text;
			hostMacParam.auth_mac = host_mac;
			hostMacParam.temp_key = &tempkey;
			if ((status = atcah_privwrite_auth_mac(&hostMacParam)) != ATCA_SUCCESS)
				break;

			// build a write command for encrypted writes
			packet.param1 = PRIVWRITE_MODE_ENCRYPT; // Mode is encrypted write
			packet.param2 = slot;                   // Key ID
			memcpy(&packet.data[0], cipher_text, sizeof(cipher_text));
			memcpy(&packet.data[36], host_mac, sizeof(host_mac));
		}

		if ((status = atPrivWrite(_gCommandObj, &packet)) != ATCA_SUCCESS)
			break;

		execution_time = atGetExecTime(_gCommandObj, CMD_PRIVWRITE);

		if ( (status = atcab_wakeup()) != ATCA_SUCCESS ) break;

		// send the command
		if ((status = atsend(_gIface, (uint8_t*)&packet, packet.txsize)) != ATCA_SUCCESS)
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ((status = atreceive(_gIface, packet.data, &packet.rxsize)) != ATCA_SUCCESS)
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief Writes a pub key from to a data slot
 *  \param[in] slot8toF Slot number to write, expected value is 0x8 through 0xF
 *  \param[out] pubkey The public key to write into the slot specified
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_write_pubkey(uint8_t slot8toF, uint8_t *pubkey)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t write_block[ATCA_BLOCK_SIZE];
	uint8_t block = 0;
	uint8_t offset = 0;
	uint8_t cpyIndex = 0;
	uint8_t cpySize = 0;
	uint8_t writeIndex = 0;

	do {
		// Check the pointers
		if (pubkey == NULL) {
			status = ATCA_BAD_PARAM;
			break;
		}
		// Check the value of the slot
		if (slot8toF < 8 || slot8toF > 0xF) {
			status = ATCA_BAD_PARAM;
			break;
		}
		// The 64 byte P256 public key gets written to a 72 byte slot in the following pattern
		// | Block 1                     | Block 2                                      | Block 3       |
		// | Pad: 4 Bytes | PubKey[0:27] | PubKey[28:31] | Pad: 4 Bytes | PubKey[32:55] | PubKey[56:63] |

		// Setup the first write block accounting for the 4 byte pad
		block = 0;
		writeIndex = ATCA_PUB_KEY_PAD;
		memset(write_block, 0, sizeof(write_block));
		cpySize = ATCA_BLOCK_SIZE - ATCA_PUB_KEY_PAD;
		memcpy(&write_block[writeIndex], &pubkey[cpyIndex], cpySize);
		cpyIndex += cpySize;
		// Write the first block
		status = atcab_write_zone(ATCA_ZONE_DATA, slot8toF, block, offset, write_block, ATCA_BLOCK_SIZE);
		if (status != ATCA_SUCCESS) break;

		// Setup the second write block accounting for the 4 byte pad
		block = 1;
		writeIndex = 0;
		memset(write_block, 0, sizeof(write_block));
		// Setup for write 4 bytes starting at 0
		cpySize = ATCA_PUB_KEY_PAD;
		memcpy(&write_block[writeIndex], &pubkey[cpyIndex], cpySize);
		cpyIndex += cpySize;
		// Setup for write skip 4 bytes and fill the remaining block
		writeIndex += cpySize + ATCA_PUB_KEY_PAD;
		cpySize = ATCA_BLOCK_SIZE - writeIndex;
		memcpy(&write_block[writeIndex], &pubkey[cpyIndex], cpySize);
		cpyIndex += cpySize;
		// Write the second block
		status = atcab_write_zone(ATCA_ZONE_DATA, slot8toF, block, offset, write_block, ATCA_BLOCK_SIZE);
		if (status != ATCA_SUCCESS) break;

		// Setup the third write block
		block = 2;
		writeIndex = 0;
		memset(write_block, 0, sizeof(write_block));
		// Setup for write 8 bytes starting at 0
		cpySize = ATCA_PUB_KEY_PAD + ATCA_PUB_KEY_PAD;
		memcpy(&write_block[writeIndex], &pubkey[cpyIndex], cpySize);
		// Write the third block
		status = atcab_write_zone(ATCA_ZONE_DATA, slot8toF, block, offset, write_block, ATCA_BLOCK_SIZE);
		if (status != ATCA_SUCCESS) break;

	} while (0);

	return status;
}

/** \brief reads a pub key from a readable data slot versus atcab_get_pubkey which generates a pubkey from a private key slot
 *  \param[in] slot8toF - slot number to read, expected value is 0x8 through 0xF
 *  \param[out] pubkey - space to receive read pubkey
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_read_pubkey(uint8_t slot8toF, uint8_t *pubkey)
{
	uint8_t ret = ATCA_GEN_FAIL;
	uint8_t read_buf[ATCA_BLOCK_SIZE];
	uint8_t block = 0;
	uint8_t offset = 0;
	uint8_t cpyIndex = 0;
	uint8_t cpySize = 0;
	uint8_t readIndex = 0;

	// Check the pointers
	if (pubkey == NULL)
		return ATCA_BAD_PARAM;
	// Check the value of the slot
	if (slot8toF < 8 || slot8toF > 0xF)
		return ATCA_BAD_PARAM;

	do {
		// The 64 byte P256 public key gets written to a 72 byte slot in the following pattern
		// | Block 1                     | Block 2                                      | Block 3       |
		// | Pad: 4 Bytes | PubKey[0:27] | PubKey[28:31] | Pad: 4 Bytes | PubKey[32:55] | PubKey[56:63] |

		// Read the block
		block = 0;
		if ( (ret = atcab_read_zone(ATCA_ZONE_DATA, slot8toF, block, offset, read_buf, ATCA_BLOCK_SIZE)) != ATCA_SUCCESS )
			break;

		// Copy.  Account for 4 byte pad
		cpySize = ATCA_BLOCK_SIZE - ATCA_PUB_KEY_PAD;
		readIndex = ATCA_PUB_KEY_PAD;
		memcpy(&pubkey[cpyIndex], &read_buf[readIndex], cpySize);
		cpyIndex += cpySize;

		// Read the next block
		block = 1;
		if ( (ret = atcab_read_zone(ATCA_ZONE_DATA, slot8toF, block, offset, read_buf, ATCA_BLOCK_SIZE)) != ATCA_SUCCESS )
			break;

		// Copy.  First four bytes
		cpySize = ATCA_PUB_KEY_PAD;
		readIndex = 0;
		memcpy(&pubkey[cpyIndex], &read_buf[readIndex], cpySize);
		cpyIndex += cpySize;
		// Copy.  Skip four bytes
		readIndex = ATCA_PUB_KEY_PAD + ATCA_PUB_KEY_PAD;
		cpySize = ATCA_BLOCK_SIZE - readIndex;
		memcpy(&pubkey[cpyIndex], &read_buf[readIndex], cpySize);
		cpyIndex += cpySize;

		// Read the next block
		block = 2;
		if ( (ret = atcab_read_zone(ATCA_ZONE_DATA, slot8toF, block, offset, read_buf, ATCA_BLOCK_SIZE)) != ATCA_SUCCESS )
			break;

		// Copy.  The remaining 8 bytes
		cpySize = ATCA_PUB_KEY_PAD + ATCA_PUB_KEY_PAD;
		readIndex = 0;
		memcpy(&pubkey[cpyIndex], &read_buf[readIndex], cpySize);

	} while (0);

	return ret;
}

/** \brief write data into given slot of data zone with offset address
 *  \param[in] slot to write data
 *  \param[in] offset of pointed slot
 *  \param[in] data pointer to write data
 *  \param[in] data length corresponding to data
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_write_bytes_slot(uint8_t slot, uint16_t offset, const uint8_t *data, uint8_t len)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	uint16_t currAddress = offset;
	uint8_t currBlock = currAddress / ATCA_BLOCK_SIZE;
	uint8_t prevBlock = currBlock;
	uint8_t currOffset = (currAddress - (currBlock * ATCA_BLOCK_SIZE)) / ATCA_WORD_SIZE;
	uint16_t writeIdx = 0;

	if (data == NULL || slot > 15)
		return ATCA_BAD_PARAM;

	do {
		status = atcab_write_zone(ATCA_ZONE_DATA, slot, currBlock, currOffset, &data[writeIdx], ATCA_BLOCK_SIZE);
		if (status != ATCA_SUCCESS) break;

		currAddress += ATCA_BLOCK_SIZE;
		currBlock = currAddress / ATCA_BLOCK_SIZE;

		if ( prevBlock == currBlock)
			currOffset++;
		else {
			currOffset = 0;
			prevBlock = currBlock;
		}

		writeIdx += ATCA_BLOCK_SIZE;
	} while (0);

	return status;
}

/** \brief write data into config, otp or data zone with given zone and offset
 *  \param[in] dev_type  to identify device
 *  \param[in] zone      to write data
 *  \param[in] address   to pointed zone
 *  \param[in] data      pointer of to write data
 *  \param[in] len       data length corresponding to data
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_write_bytes_zone(ATCADeviceType dev_type, uint8_t zone, uint16_t address, const uint8_t *data, uint8_t len)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	uint8_t dataSlot = 0;
	uint16_t currAddress = address, writeIdx = 0;
	uint8_t currBlock, prevBlock, currOffset;
	const static uint16_t slot8_addr = 288, slot9_addr = 704;

	if (data == NULL || zone > ATCA_ZONE_DATA)
		return ATCA_BAD_PARAM;

	if (zone == ATCA_ZONE_CONFIG) {

		currBlock = currAddress / ATCA_BLOCK_SIZE;
		prevBlock = currBlock;
		currOffset = (currAddress - (currBlock * ATCA_BLOCK_SIZE)) / ATCA_WORD_SIZE;

		while ( writeIdx < len) {

			if (!(currBlock == 0 && (currOffset == 0 || currOffset == 1 || currOffset == 2 || currOffset == 3))
			    && !(currBlock == 2 && (currOffset == 5 || currOffset == 6))
			    ) {
				status = atcab_write_zone(zone, 0, currBlock, currOffset, &data[writeIdx], ATCA_WORD_SIZE);
				if (status != ATCA_SUCCESS) break;
			}

			currAddress += ATCA_WORD_SIZE;
			currBlock = currAddress / ATCA_BLOCK_SIZE;

			if ( prevBlock == currBlock)
				currOffset++;
			else {
				currOffset = 0;
				prevBlock = currBlock;
			}

			writeIdx += ATCA_WORD_SIZE;
		}

	} else if (zone == ATCA_ZONE_OTP) {

		currBlock = currAddress / ATCA_BLOCK_SIZE;
		prevBlock = currBlock;
		currOffset = (currAddress - (currBlock * ATCA_BLOCK_SIZE)) / ATCA_WORD_SIZE;

		while ( writeIdx < len) {

			status = atcab_write_zone(zone, 0, currBlock, currOffset, &data[writeIdx], ATCA_WORD_SIZE);
			if (status != ATCA_SUCCESS) break;

			currAddress += ATCA_WORD_SIZE;
			currBlock = currAddress / ATCA_BLOCK_SIZE;

			if ( prevBlock == currBlock)
				currOffset++;
			else {
				currOffset = 0;
				prevBlock = currBlock;
			}

			writeIdx += ATCA_WORD_SIZE;
		}

	} else {

		if (dev_type == ATECC508A || dev_type == ATECC108A ) {

			if (currAddress < slot8_addr) {
				dataSlot = currAddress / 36;
				currBlock = (currAddress - (dataSlot * 36)) / ATCA_BLOCK_SIZE;
				currOffset = (currAddress - (dataSlot * ATCA_BLOCK_SIZE)) / ATCA_WORD_SIZE;
				currAddress = currAddress - (dataSlot * 36);
			} else if (currAddress < slot9_addr) {
				dataSlot = 8;
				currBlock = (currAddress - slot8_addr) / ATCA_BLOCK_SIZE;
				currOffset = (currAddress - slot8_addr) / ATCA_WORD_SIZE;
				currAddress = currAddress - slot8_addr;
			} else {
				dataSlot = (currAddress - slot9_addr) / 72;
				currBlock = (currAddress - slot9_addr) / ATCA_BLOCK_SIZE;
				currOffset = (currAddress - slot9_addr) / ATCA_WORD_SIZE;
				currAddress = currAddress - slot9_addr;
			}

			prevBlock = currBlock;

		} else {

			dataSlot = currAddress / ATCA_BLOCK_SIZE;
			currBlock = (currAddress - (dataSlot * ATCA_BLOCK_SIZE)) / ATCA_BLOCK_SIZE;
			currOffset = (currAddress - (dataSlot * ATCA_BLOCK_SIZE)) / ATCA_WORD_SIZE;
			currAddress = currAddress - (dataSlot * ATCA_BLOCK_SIZE);
			prevBlock = currBlock;
		}

		while ( writeIdx < len) {

			status = atcab_write_zone(ATCA_ZONE_DATA, dataSlot, currBlock, currOffset, &data[writeIdx], ATCA_WORD_SIZE);
			if (status != ATCA_SUCCESS) break;

			currAddress += ATCA_WORD_SIZE;
			currBlock = currAddress / ATCA_BLOCK_SIZE;

			if ( prevBlock == currBlock)
				currOffset++;
			else {
				currOffset = 0;
				prevBlock = currBlock;
			}

			writeIdx += ATCA_WORD_SIZE;
		}

	}

	return status;
}

/** \brief read data from config, otp or data zone with given zone, offset and len
 *  \param[in]  dev_type  to identify device
 *  \param[in]  zone      to write data
 *  \param[in]  address   of pointed zone
 *  \param[in]  len       length to be read
 *  \param[out] data      buffer to be read data
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_read_bytes_zone(ATCADeviceType dev_type, uint8_t zone, uint16_t address, uint8_t len, uint8_t *data)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	uint8_t dataSlot = 0;
	uint16_t currAddress = address, readIdx = 0;
	uint8_t currBlock, prevBlock, currOffset;
	const static uint16_t slot8_addr = 288, slot9_addr = 704;

	if (data == NULL || zone > ATCA_ZONE_DATA)
		return ATCA_BAD_PARAM;

	if (zone == ATCA_ZONE_CONFIG || zone == ATCA_ZONE_OTP) {

		currBlock = currAddress / ATCA_BLOCK_SIZE;
		prevBlock = currBlock;
		currOffset = (currAddress - (currBlock * ATCA_BLOCK_SIZE)) / ATCA_WORD_SIZE;

		while ( readIdx < len) {

			status = atcab_read_zone(zone, 0, currBlock, currOffset, &data[readIdx], ATCA_WORD_SIZE);
			if (status != ATCA_SUCCESS) break;

			currAddress += ATCA_WORD_SIZE;
			currBlock = currAddress / ATCA_BLOCK_SIZE;

			if ( prevBlock == currBlock)
				currOffset++;
			else {
				currOffset = 0;
				prevBlock = currBlock;
			}

			readIdx += ATCA_WORD_SIZE;
		}

	} else {

		if (dev_type == ATECC508A || dev_type == ATECC108A ) {

			if (currAddress < slot8_addr) {
				dataSlot = currAddress / 36;
				currBlock = (currAddress - (dataSlot * 36)) / ATCA_BLOCK_SIZE;
				currOffset = (currAddress - (dataSlot * ATCA_BLOCK_SIZE)) / ATCA_WORD_SIZE;
				currAddress = currAddress - (dataSlot * 36);
			} else if (currAddress < slot9_addr) {
				dataSlot = 8;
				currBlock = (currAddress - slot8_addr) / ATCA_BLOCK_SIZE;
				currOffset = (currAddress - slot8_addr) / ATCA_WORD_SIZE;
				currAddress = currAddress - slot8_addr;
			} else {
				dataSlot = (currAddress - slot9_addr) / 72;
				currBlock = (currAddress - slot9_addr) / ATCA_BLOCK_SIZE;
				currOffset = (currAddress - slot9_addr) / ATCA_WORD_SIZE;
				currAddress = currAddress - slot9_addr;
			}

			prevBlock = currBlock;

		} else {

			dataSlot = currAddress / ATCA_BLOCK_SIZE;
			currBlock = (currAddress - (dataSlot * ATCA_BLOCK_SIZE)) / ATCA_BLOCK_SIZE;
			currOffset = (currAddress - (dataSlot * ATCA_BLOCK_SIZE)) / ATCA_WORD_SIZE;
			currAddress = currAddress - (dataSlot * ATCA_BLOCK_SIZE);
			prevBlock = currBlock;
		}

		while ( readIdx < len) {

			status = atcab_read_zone(ATCA_ZONE_DATA, dataSlot, currBlock, currOffset, &data[readIdx], ATCA_WORD_SIZE);
			if (status != ATCA_SUCCESS) break;

			currAddress += ATCA_WORD_SIZE;
			currBlock = currAddress / ATCA_BLOCK_SIZE;

			if ( prevBlock == currBlock)
				currOffset++;
			else {
				currOffset = 0;
				prevBlock = currBlock;
			}

			readIdx += ATCA_WORD_SIZE;
		}

	}

	return status;
}


/** \brief Get a 32 byte MAC from the CryptoAuth device given a key ID and a challenge
 *	\param[in]  mode       Controls which fields within the device are used in the message
 *	\param[in]  key_id     The key in the CryptoAuth device to use for the MAC
 *	\param[in]  challenge  The 32 byte challenge number
 *	\param[out] digest     The response of the MAC command using the given challenge
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_mac( uint8_t mode, uint16_t key_id, const uint8_t* challenge, uint8_t* digest )
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	do {

		// Verify the inputs
		if ( challenge == NULL || digest == NULL ) {
			status = ATCA_BAD_PARAM;
			break;
		}

		// build mac command
		packet.param1 = mode;
		packet.param2 = key_id;
		memcpy( &packet.data[0], challenge, 32 );  // a 32-byte challenge

		if ( (status = atMAC( _gCommandObj, &packet )) != ATCA_SUCCESS )
			break;

		execution_time = atGetExecTime( _gCommandObj, CMD_MAC);

		if ( (status != atcab_wakeup()) != ATCA_SUCCESS ) break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &(packet.rxsize))) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		// check for response
		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

		memcpy( digest, &packet.data[ATCA_RSP_DATA_IDX], MAC_SIZE );

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief Compares a MAC response with input values
 *	\param[in] mode Controls which fields within the device are used in the message
 *	\param[in] key_id The key in the CryptoAuth device to use for the MAC
 *	\param[in] challenge The 32 byte challenge number
 *	\param[in] response The 32 byte mac response number
 *	\param[in] other_data The 13 byte other data number
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_checkmac( uint8_t mode, uint16_t key_id, const uint8_t *challenge, const uint8_t *response, const uint8_t *other_data)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	do {

		// Verify the inputs
		if ( challenge == NULL || response == NULL || other_data == NULL ) {
			status = ATCA_BAD_PARAM;
			break;
		}

		// build checkmac command
		packet.param1 = mode;
		packet.param2 = key_id;
		memcpy( &packet.data[0], challenge, CHECKMAC_CLIENT_CHALLENGE_SIZE );
		memcpy( &packet.data[32], response, CHECKMAC_CLIENT_RESPONSE_SIZE );
		memcpy( &packet.data[64], other_data, CHECKMAC_OTHER_DATA_SIZE );

		if ( (status = atCheckMAC( _gCommandObj, &packet )) != ATCA_SUCCESS )
			break;

		execution_time = atGetExecTime( _gCommandObj, CMD_CHECKMAC);

		if ( (status != atcab_wakeup()) != ATCA_SUCCESS ) break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms( execution_time );

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &(packet.rxsize))) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		// check for response
		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief Initialize SHA-256 calculation engine
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_sha_start(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	do {

		// build checkmac command
		packet.param1 = SHA_SHA256_START_MASK;
		packet.param2 = 0;

		if ( (status = atSHA( _gCommandObj, &packet )) != ATCA_SUCCESS )
			break;

		execution_time = atGetExecTime( _gCommandObj, CMD_SHA);

		if ( (status != atcab_wakeup()) != ATCA_SUCCESS )
			break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms( execution_time );

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &(packet.rxsize))) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		// check for response
		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief Adds the message to be digested
 *	\param[in] length The number of bytes in the Message parameter
 *	\param[in] message up to 64 bytes of data to be included into the hash operation.
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_sha_update(uint16_t length, const uint8_t *message)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	do {

		// Verify the inputs
		if ( message == NULL || length > SHA_BLOCK_SIZE ) {
			status = ATCA_BAD_PARAM;
			break;
		}

		// build checkmac command
		packet.param1 = SHA_SHA256_UPDATE_MASK;
		packet.param2 = length;
		memcpy(&packet.data[0], message, length);

		if ( (status = atSHA( _gCommandObj, &packet )) != ATCA_SUCCESS )
			break;

		execution_time = atGetExecTime( _gCommandObj, CMD_SHA);

		if ( (status != atcab_wakeup()) != ATCA_SUCCESS ) break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms( execution_time );

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &(packet.rxsize))) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		// check for response
		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief The SHA-256 calculation is complete
 *	\param[out] digest The SHA256 digest that is calculated
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_sha_end(uint8_t *digest, uint16_t length, const uint8_t *message)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	if ( length > 63 || digest == NULL )
		return ATCA_BAD_PARAM;

	if ( length > 0 && message == NULL )
		return ATCA_BAD_PARAM;

	do {

		// Verify the inputs
		if ( digest == NULL ) {
			status = ATCA_BAD_PARAM;
			break;
		}

		// build SHA command
		packet.param1 = SHA_SHA256_END_MASK;
		packet.param2 = length;
		if ( length > 0 )
			memcpy(&packet.data[0], message, length);

		if ( (status = atSHA( _gCommandObj, &packet )) != ATCA_SUCCESS )
			break;

		execution_time = atGetExecTime( _gCommandObj, CMD_SHA);

		if ( (status != atcab_wakeup()) != ATCA_SUCCESS ) break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms( execution_time );

		// receive the response
		if ( (status = atreceive( _gIface, packet.data, &(packet.rxsize))) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		// check for response
		if ( (status = isATCAError(packet.data)) != ATCA_SUCCESS )
			break;

		memcpy( digest, &packet.data[ATCA_RSP_DATA_IDX], ATCA_SHA_DIGEST_SIZE );

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief Computes a SHA-256 digest
 *	\param[in] length The number of bytes in the message parameter
 *	\param[in] message - pointer to variable length message
 *	\param[out] digest The SHA256 digest
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_sha(uint16_t length, const uint8_t *message, uint8_t *digest)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	int blocks = 0, remainder = 0, msgIndex = 0;

	if ( length == 0 || message == NULL || digest == NULL )
		return ATCA_BAD_PARAM;

	do {

		blocks = length / SHA_BLOCK_SIZE;
		remainder = length % SHA_BLOCK_SIZE;

		status = atcab_sha_start();
		if ( status != ATCA_SUCCESS )
			break;

		while ( blocks-- > 0 ) {
			status = atcab_sha_update(SHA_BLOCK_SIZE, &message[msgIndex]);
			if ( status != ATCA_SUCCESS )
				break;
			msgIndex += SHA_BLOCK_SIZE;
		}

		status = atcab_sha_end(digest, remainder, &message[msgIndex]);

		if ( status != ATCA_SUCCESS )
			break;

	} while (0);

	return status;
}
