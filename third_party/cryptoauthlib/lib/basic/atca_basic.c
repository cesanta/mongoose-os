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

char atca_version[] = { "20161123" };  // change for each release, yyyymmdd

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

ATCA_STATUS atcab_info(uint8_t *revision )
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
		// Check the inputs
		if (revision == NULL)
		{
			status = ATCA_BAD_PARAM;
			BREAK(status, "atcab_info: Null inputs");
		}
		if ( (status = atInfo( _gCommandObj, &packet )) != ATCA_SUCCESS )
			BREAK(status, "Failed to construct Info command");

		execution_time = atGetExecTime( _gCommandObj, CMD_INFO);

		if ( (status = atcab_wakeup()) != ATCA_SUCCESS )
			BREAK(status, "Failed to wakeup");

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
			BREAK(status, "Failed to send Info command");

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ( (status = atreceive( _gIface, &(packet.info[0]), &(packet.rxsize) )) != ATCA_SUCCESS )
			BREAK(status, "Failed to receive Info command");

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			BREAK(status, "Info command returned error code or no resonse");
		}

		if ( (status = isATCAError(packet.info)) != ATCA_SUCCESS )
			BREAK(status, "Failed to construct Info command");

		memcpy( revision, &packet.info[1], 4 );  // don't include the receive length, only payload
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
    
	do {
        // build an random command
        packet.param1 = RANDOM_SEED_UPDATE;
        packet.param2 = 0x0000;
        if ( (status = atRandom( _gCommandObj, &packet )) != ATCA_SUCCESS )
            break;
        
        execution_time = atGetExecTime( _gCommandObj, CMD_RANDOM);
        
		if ( (status = atcab_wakeup()) != ATCA_SUCCESS )
			break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS)
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ( (status = atreceive( _gIface, packet.info, &packet.rxsize)) != ATCA_SUCCESS)
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ( (status = isATCAError(packet.info)) != ATCA_SUCCESS )
			break;
        
        if (packet.rxsize < packet.info[ATCA_COUNT_IDX] || packet.info[ATCA_COUNT_IDX] != RANDOM_RSP_SIZE)
        {
            status = ATCA_RX_FAIL;
            break;
        }            
        
        if (rand_out)
		    memcpy( rand_out, &packet.info[ATCA_RSP_DATA_IDX], RANDOM_NUM_SIZE );
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief This command can generate a private key, compute a public key,
 *         and/or compute a digest of of a public key.
 *
 * \param[in]  mode        Mode bit mask determines what operations the GenKey
 *                         command performs. Bit 4 overrides bits 2 and 3.
 *                         Bit 2 will create a new random private key in
 *                           key_id and return the public key. Leave 0 to just
 *                           calculate the public key from an existing private
 *                           key.
 *                         Bit 3 will create a PubKey digest from the
 *                           calculated public key and store it in TempKey.
 *                         Bit 4 will create a PubKey digest from the public
 *                           key stored in key_id and store it in TempKey.
 * \param[in]  key_id      Slot to perform the GenKey command on.
 * \param[in]  other_data  If bit 4 of mode is set, these 3 bytes replace
 *                         Param1 and Param2 in the PubKey digest calculation.
 *                         Can be set to NULL otherwise.
 * \param[out] public_key  If the mode indicates a public key will be
 *                         calculated, it will be returned here. Format will
 *                         be the X and Y integers in big-endian format.
 *                         64 bytes for P256 curve. Set to NULL if public key
 *                         isn't required.
 * 
 * \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_genkey_base(uint8_t mode, uint16_t key_id, const uint8_t* other_data, uint8_t* public_key)
{
    ATCAPacket packet;
    uint16_t execution_time = 0;
    ATCA_STATUS status = ATCA_GEN_FAIL;

    if ( !_gDevice )
        return ATCA_GEN_FAIL;
    
    do {
        // Build GenKey command
        packet.param1 = mode;
        packet.param2 = key_id;
        if (other_data)
            memcpy(packet.info, other_data, GENKEY_OTHER_DATA_SIZE);
        if ((status = atGenKey( _gCommandObj, &packet)) != ATCA_SUCCESS)
            break;

        execution_time = atGetExecTime(_gCommandObj, CMD_GENKEY);

        if ((status = atcab_wakeup()) != ATCA_SUCCESS)
            break;

        // send the command
        if ((status = atsend(_gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS)
            break;

        // delay the appropriate amount of time for command to execute
        atca_delay_ms(execution_time);

        // receive the response
        if ((status = atreceive(_gIface, packet.info, &(packet.rxsize))) != ATCA_SUCCESS)
            break;

        // Check response size
        if (packet.rxsize < 4) {
            if (packet.rxsize > 0)
                status = ATCA_RX_FAIL;
            else
                status = ATCA_RX_NO_RESPONSE;
            break;
        }

        if ((status = isATCAError(packet.info)) != ATCA_SUCCESS)
            break;

        if (public_key && packet.info[ATCA_COUNT_IDX] > 4)
            memcpy(public_key, &packet.info[ATCA_RSP_DATA_IDX], packet.info[ATCA_COUNT_IDX]-3);
    } while (0);

    _atcab_exit();
    return status;
}

/** \brief Generate a new random private key and return the public key.
 *
 * \param[in]  key_id      Slot where an ECC private key is configured.
 * \param[out] public_key  Public key will be returned here. Format will be
 *                         the X and Y integers in big-endian format.
 *                         64 bytes for P256 curve. Set to NULL if public key
 *                         isn't required.
 *
 * \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_genkey(uint16_t key_id, uint8_t *public_key)
{
    return atcab_genkey_base(GENKEY_MODE_PRIVATE, key_id, NULL, public_key);
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
		memcpy( packet.info, challenge, 32 );

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
		if ((status = atreceive( _gIface, packet.info, &(packet.rxsize))) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ( (status = isATCAError(packet.info)) != ATCA_SUCCESS )
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
		memcpy( packet.info, seed, 20 );

		if ((status = atNonce(_gCommandObj, &packet)) != ATCA_SUCCESS) break;

		execution_time = atGetExecTime(_gCommandObj, CMD_NONCE);

		if ((status = atcab_wakeup()) != ATCA_SUCCESS ) break;

		// send the command
		if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize)) != ATCA_SUCCESS ) break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ((status = atreceive(_gIface, packet.info, &(packet.rxsize))) != ATCA_SUCCESS) break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ((status = isATCAError(packet.info)) != ATCA_SUCCESS) break;

		memcpy(&rand_out[0], &packet.info[ATCA_RSP_DATA_IDX], 32);

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief send a challenge to the device (a non-seed update nonce)
*  \param[in] seed - pointer to 32 bytes of data which will be sent as the challenge
*  \param[out] rand_out - points to space to receive random number
*  \return ATCA_STATUS
*/
ATCA_STATUS atcab_nonce_base(uint8_t mode, const uint8_t *num_in, uint8_t* rand_out)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	do {

		// build a nonce command (pass through mode)
		packet.param1 = mode;
		packet.param2 = 0x0000;
		if (mode == NONCE_MODE_PASSTHROUGH)
			memcpy(packet.info, num_in, 32);
		else
			memcpy(packet.info, num_in, 20);


		if ((status = atNonce(_gCommandObj, &packet)) != ATCA_SUCCESS) break;

		execution_time = atGetExecTime(_gCommandObj, CMD_NONCE);

		if ((status = atcab_wakeup()) != ATCA_SUCCESS) break;

		// send the command
		if ((status = atsend(_gIface, (uint8_t*)&packet, packet.txsize)) != ATCA_SUCCESS) break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ((status = atreceive(_gIface, packet.info, &(packet.rxsize))) != ATCA_SUCCESS) break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ((status = isATCAError(packet.info)) != ATCA_SUCCESS) break;

		if ((rand_out != NULL) && (packet.rxsize >= 35))
			memcpy(&rand_out[0], &packet.info[ATCA_RSP_DATA_IDX], 32);

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
	uint8_t status = ATCA_GEN_FAIL;
	uint8_t read_buf[ATCA_BLOCK_SIZE];
    
    if (!serial_number)
        return ATCA_BAD_PARAM;
    
	do {
		if ( (status = atcab_read_zone(ATCA_ZONE_CONFIG, 0, 0, 0, read_buf, ATCA_BLOCK_SIZE)) != ATCA_SUCCESS )
			break;
        memcpy(&serial_number[0], &read_buf[0], 4);
        memcpy(&serial_number[4], &read_buf[8], 5);
	} while (0);
    
	return status;
}

/** \brief The Verify command takes an ECDSA [R,S] signature and verifies that
 *         it is correctly generated from a given message and public key. In
 *         all cases, the signature is an input to the command.
 *
 * For the Stored, External, and ValidateExternal Modes, the contents of
 * TempKey should contain the SHA-256 digest of the message.
 *
 * \param[in] mode        Verify command mode: Stored(0), ValidateExternal(1),
 *                        External(2), Validate(3), or Invalidate(7)
 * \param[in] key_id      Stored Mode - The slot containing the public key to
 *                          be used for the verification.
 *                        ValidateExternal Mode - The slot containing the
 *                          public key to be validated.
 *                        External Mode - KeyID contains the curve type to be
 *                          used to Verify the signature.
 *                        Validate or Invalidate Mode - The slot containing
 *                          the public key to be (in)validated.
 * \param[in] signature   Signature to be verified. R and S integers in
 *                        big-endian format. 64 bytes for P256 curve.
 * \param[in] public_key  If mode is External, the public key to be used for
 *                        verification. X and Y integers in big-endian format.
 *                        64 bytes for P256 curve. NULL for all other modes.
 * \param[in] other_data  If mode is Validate, the bytes used to generate the
 *                        message for the validation (19 bytes). NULL for all
 *                        other modes.
 *
 * \return ATCA_SUCCESS
 */
ATCA_STATUS atcab_verify(uint8_t mode, uint16_t key_id, const uint8_t* signature, const uint8_t* public_key, const uint8_t* other_data)
{
    ATCA_STATUS status = ATCA_GEN_FAIL;
    ATCAPacket packet;
    uint16_t execution_time = 0;

    if ( !_gDevice )
        return ATCA_GEN_FAIL;
    
    do 
    {
        if (signature == NULL)
            return ATCA_BAD_PARAM;
        if (mode == VERIFY_MODE_EXTERNAL && public_key == NULL)
            return ATCA_BAD_PARAM;
        if (mode == VERIFY_MODE_VALIDATE && other_data == NULL)
            return ATCA_BAD_PARAM;
            
        // Build the verify command
        packet.param1 = mode;
        packet.param2 = key_id;
        memcpy( &packet.info[0], signature, ATCA_SIG_SIZE);
        if (mode == VERIFY_MODE_EXTERNAL)
            memcpy(&packet.info[ATCA_SIG_SIZE], public_key, ATCA_PUB_KEY_SIZE);
        else if (other_data)
            memcpy(&packet.info[ATCA_SIG_SIZE], other_data, VERIFY_OTHER_DATA_SIZE);

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
        if ( (status = atreceive( _gIface, packet.info, &(packet.rxsize) )) != ATCA_SUCCESS )
            break;

        // Check response size
        if (packet.rxsize < 4) {
            if (packet.rxsize > 0)
                status = ATCA_RX_FAIL;
            else
                status = ATCA_RX_NO_RESPONSE;
            break;
        }

        status = isATCAError(packet.info);
    } while (false);
    
    _atcab_exit();
    return status;
}

/** \brief Verify a signature (ECDSA verify operation) with all components
 *         (message, signature, and public key) supplied. Uses the
 *         CryptoAuthentication hardware instead of software.
 *
 * \param[in]  message      32 byte message to be verified. Typically the
 *                            SHA256 hash of the full message.
 * \param[in]  signature    Signature to be verified. R and S integers in
 *                            big-endian format. 64 bytes for P256 curve.
 * \param[in]  public_key   The public key to be used for verification. X and
 *                            Y integers in big-endian format. 64 bytes for
 *                            P256 curve.
 * \param[out] is_verified  Boolean whether or not the message, signature, 
 *                            public key verified.
 *
 * \return ATCA_SUCCESS on verification success or failure, because the
 *         command still completed successfully.
 */
ATCA_STATUS atcab_verify_extern(const uint8_t *message, const uint8_t *signature, const uint8_t *public_key, bool *is_verified)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	if (signature == NULL || message == NULL || public_key == NULL || is_verified == NULL)
		return ATCA_BAD_PARAM;

	do {
		// nonce passthrough
		if ( (status = atcab_challenge(message)) != ATCA_SUCCESS )
			break;

		status = atcab_verify(VERIFY_MODE_EXTERNAL, VERIFY_KEY_P256, signature, public_key, NULL);
		*is_verified = (status == ATCA_SUCCESS);
		if (status == ATCA_CHECKMAC_VERIFY_FAILED)
			status = ATCA_SUCCESS; // Verify failed, but command succeeded
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief Verify a signature and message (ECDSA verify operation) against a
 *         public key stored in a specified slot.
 *
 * \param[in]  message      32 byte message to be verified. Typically the
 *                            SHA256 hash of the full message.
 * \param[in]  signature    Signature to be verified. R and S integers in
 *                            big-endian format. 64 bytes for P256 curve.
 * \param[in]  key_id       Slot containing the public key to be used in the
 *                            verification.
 * \param[out] is_verified  Boolean whether or not the message, signature, 
 *                            public key verified.
 *
 * \return ATCA_SUCCESS on verification success or failure, because the
 *         command still completed successfully.
 */
ATCA_STATUS atcab_verify_stored(const uint8_t *message, const uint8_t *signature, uint16_t key_id, bool *is_verified)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	if (key_id > 15)
		return ATCA_BAD_PARAM;
	if (signature == NULL || message == NULL || is_verified == NULL)
		return ATCA_BAD_PARAM;

	do {
		// nonce passthrough
		if ( (status = atcab_challenge(message)) != ATCA_SUCCESS )
			break;

		status = atcab_verify(VERIFY_MODE_STORED, key_id, signature, NULL, NULL);
		*is_verified = (status == ATCA_SUCCESS);
		if (status == ATCA_CHECKMAC_VERIFY_FAILED)
			status = ATCA_SUCCESS; // Verify failed, but command succeeded
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief Validate a public key stored in a slot.
 *
 * This command can only be run after GenKey has been used to create a PubKey
 * digest of the public key to be validated in TempKey (mode=0x10).
 * 
 * \param[in]  key_id       Slot containing the public key to be validated.
 * \param[in]  signature    Signature to be verified. R and S integers in
 *                            big-endian format. 64 bytes for P256 curve.
 * \param[in]  other_data   19 bytes of data used to build the verification
 *                            message.
 * \param[out] is_verified  Boolean whether or not the message, signature,
 *                            validation public key verified.
 *
 * \return ATCA_SUCCESS on verification success or failure, because the
 *         command still completed successfully.
 */
ATCA_STATUS atcab_verify_validate(uint16_t key_id, const uint8_t *signature, const uint8_t *other_data, bool *is_verified)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	if (key_id > 15)
		return ATCA_BAD_PARAM;
	if (signature == NULL || other_data == NULL || is_verified == NULL)
		return ATCA_BAD_PARAM;

	status = atcab_verify(VERIFY_MODE_VALIDATE, key_id, signature, NULL, other_data);
	*is_verified = (status == ATCA_SUCCESS);
	if (status == ATCA_CHECKMAC_VERIFY_FAILED)
		status = ATCA_SUCCESS; // Verify failed, but command succeeded

	return status;
}

/** \brief Invalidate a public key stored in a slot.
*
* This command can only be run after GenKey has been used to create a PubKey
* digest of the public key to be invalidated in TempKey (mode=0x10).
*
* \param[in]  key_id       Slot containing the public key to be invalidated.
* \param[in]  signature    Signature to be verified. R and S integers in
*                            big-endian format. 64 bytes for P256 curve.
* \param[in]  other_data   19 bytes of data used to build the verification
*                            message.
* \param[out] is_verified  Boolean whether or not the message, signature,
*                            validation public key verified.
*
* \return ATCA_SUCCESS on verification success or failure, because the
*         command still completed successfully.
*/
ATCA_STATUS atcab_verify_invalidate(uint16_t key_id, const uint8_t *signature, const uint8_t *other_data, bool *is_verified)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	if (key_id > 15)
		return ATCA_BAD_PARAM;
	if (signature == NULL || other_data == NULL || is_verified == NULL)
		return ATCA_BAD_PARAM;

	status = atcab_verify(VERIFY_MODE_INVALIDATE, key_id, signature, NULL, other_data);
	*is_verified = (status == ATCA_SUCCESS);
	if (status == ATCA_CHECKMAC_VERIFY_FAILED)
		status = ATCA_SUCCESS; // Verify failed, but command succeeded

	return status;
}

/** \brief ECDH command with premaster secret returned in the response.
 *
 *  \param[in] key_id     Slot of key for ECDH computation
 *  \param[in] pubkey     Public key input to ECDH calculation. X and Y
 *                        integers in big-endian format. 64 bytes for P256
 *                        key.
 *  \param[out] pms       Computed ECDH premaster secret is returned here.
 *                        32 bytes.
 *
 *  \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_ecdh(uint16_t key_id, const uint8_t* pubkey, uint8_t* pms)
{
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	do {
		if (pubkey == NULL || pms == NULL) {
			status = ATCA_BAD_PARAM;
			break;
		}
		memset(pms, 0, ATCA_KEY_SIZE);

		// build a ecdh command
		packet.param1 = ECDH_PREFIX_MODE;
		packet.param2 = key_id;
		memcpy( packet.info, pubkey, ATCA_PUB_KEY_SIZE );

		if ( (status = atECDH( _gCommandObj, &packet )) != ATCA_SUCCESS ) break;

		execution_time = atGetExecTime( _gCommandObj, CMD_ECDH);

		if ( (status = atcab_wakeup()) != ATCA_SUCCESS ) break;

		if ( (status = atsend(_gIface, (uint8_t*)&packet, packet.txsize)) != ATCA_SUCCESS ) break;

		atca_delay_ms(execution_time);

		if ((status = atreceive(_gIface, packet.info, &packet.rxsize)) != ATCA_SUCCESS) break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ( (status = isATCAError(packet.info)) != ATCA_SUCCESS ) break;

		// The ECDH command may return a single byte. Then the CRC is copied into indices [1:2]
		memcpy(pms, &packet.info[ATCA_RSP_DATA_IDX], ATCA_KEY_SIZE);

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief ECDH command with premaster secret read (encrypted) from next slot.
 *
 *  \param[in] key_id     Slot of key for ECDH computation
 *  \param[in] pubkey     Public key input to ECDH calculation. X and Y
 *                        integers in big-endian format. 64 bytes for P256
 *                        key.
 *  \param[out] pms       Computed ECDH premaster secret is returned here.
 *                        32 bytes.
 *  \param[in]  enckey    Read key for the premaster secret slot (slotid+1).
 *  \param[in]  enckeyid  Read key slot for enckey.
 *
 *  \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_ecdh_enc(uint16_t key_id, const uint8_t* pubkey, uint8_t* pms, const uint8_t* enckey, uint16_t enckeyid)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t cmpBuf[ATCA_WORD_SIZE];
	uint8_t block = 0;

	do {
		// Check the inputs
		if (pubkey == NULL || pms == NULL || enckey == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Bad input parameters");
		}
		// Send the ECDH command with the public key provided
		if ((status = atcab_ecdh(key_id, pubkey, pms)) != ATCA_SUCCESS) BREAK(status, "ECDH Failed");

		// ECDH may return a key or a single byte.  The atcab_ecdh() function performs a memset to FF on ecdhRsp. 
		memset(cmpBuf, 0xFF, ATCA_WORD_SIZE);

		// Compare arbitrary bytes to see if they are 00
		if (memcmp(cmpBuf, &pms[18], ATCA_WORD_SIZE) == 0) {
			// There is no ecdh key, check the value of the first byte for success
			if (pms[0] != CMD_STATUS_SUCCESS) BREAK(status, "ECDH Command Execution Failure");

			// ECDH succeeded, perform an encrypted read from the n+1 slot.
			if ((status = atcab_read_enc(key_id + 1, block, pms, enckey, enckeyid)) != ATCA_SUCCESS) BREAK(status, "Encrypte read failed");
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
ATCA_STATUS atcab_get_addr(uint8_t zone, uint16_t slot, uint8_t block, uint8_t offset, uint16_t* addr)
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

/** \brief Gets the size of the specified zone in bytes.
 *
 * \param[in]  zone  Zone to get size information from. Config(0), OTP(1), or
 *                   Data(2) which requires a slot.
 * \param[in]  slot  If zone is Data(2), the slot to query for size.
 * \param[out] size  Zone size is returned here.
 *
 * \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_get_zone_size(uint8_t zone, uint16_t slot, size_t* size)
{
    ATCA_STATUS status = ATCA_SUCCESS;
    
    if (size == NULL)
        return ATCA_BAD_PARAM;
    
    if (atgetifacecfg(_gIface)->devtype == ATSHA204A)
    {
        switch (zone)
        {
        case ATCA_ZONE_CONFIG: *size = 88; break;
        case ATCA_ZONE_OTP:    *size = 64; break;
        case ATCA_ZONE_DATA:   *size = 32; break;
        default: status = ATCA_BAD_PARAM; break;
        }        
    }
    else
    {
        switch (zone)
        {
            case ATCA_ZONE_CONFIG: *size = 128; break;
            case ATCA_ZONE_OTP:    *size = 64; break;
            case ATCA_ZONE_DATA:
                if (slot < 8)
                    *size = 36;
                else if (slot == 8)
                    *size = 416;
                else if (slot < 16)
                    *size = 72;
                else
                    status = ATCA_BAD_PARAM;
                break;
            default: status = ATCA_BAD_PARAM; break;
        }
    }
    
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

/**
 * \brief The Write command writes either one four byte word or an 8-word block of 32 bytes to one of the EEPROM
 * zones on the device. Depending upon the value of the WriteConfig byte for this slot, the data may be required
 * to be encrypted by the system prior to being sent to the device. This command cannot be used to write slots
 * configured as ECC private keys.
 *
 * \param[in] zone     Zone/Param1 for the write command.
 * \param[in] address  Addres/Param2 for the write command.
 * \param[in] value    Plain-text data to be written or cipher-text for encrypted writes. 32 or 4 bytes depending
 *                     on bit 7 in the zone.
 * \param[in] mac      MAC required for encrypted writes (32 bytes). Set to NULL if not required.
 *
 *  \return ATCA_SUCCESS
 */
ATCA_STATUS atcab_write(uint8_t zone, uint16_t address, const uint8_t *value, const uint8_t *mac)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	if (value == NULL)
		return ATCA_BAD_PARAM;

	do {
		// Build the write command
		packet.param1 = zone;
		packet.param2 = address;
		if (zone & ATCA_ZONE_READWRITE_32)
		{
			// 32-byte write
			memcpy(packet.info, value, 32);
			// Only 32-byte writes can have a MAC
			if (mac)
				memcpy(&packet.info[32], mac, 32);
		}
		else
		{
			// 4-byte write
			memcpy(packet.info, value, 4);
		}
        if ((status = atWrite(_gCommandObj, &packet, mac && (zone & ATCA_ZONE_READWRITE_32))) != ATCA_SUCCESS)
			break;
        
		execution_time = atGetExecTime(_gCommandObj, CMD_WRITEMEM);

		if ((status = atcab_wakeup()) != ATCA_SUCCESS)
			break;

		// send the command
		if ((status = atsend(_gIface, (uint8_t*)&packet, packet.txsize)) != ATCA_SUCCESS)
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ((status = atreceive(_gIface, packet.info, &(packet.rxsize))) != ATCA_SUCCESS)
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		status = isATCAError(packet.info);

	} while (0);

	_atcab_exit();
	return status;
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
ATCA_STATUS atcab_write_zone(uint8_t zone, uint16_t slot, uint8_t block, uint8_t offset, const uint8_t *data, uint8_t len)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint16_t addr;

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
        
		status = atcab_write(zone, addr, data, NULL);

	} while (0);
    
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
ATCA_STATUS atcab_read_zone(uint8_t zone, uint16_t slot, uint8_t block, uint8_t offset, uint8_t *data, uint8_t len)
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
		if ( (status = atreceive( _gIface, packet.info, &(packet.rxsize) )) != ATCA_SUCCESS )
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ( (status = isATCAError(packet.info)) != ATCA_SUCCESS )
			break;

		memcpy( data, &packet.info[1], len );
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief Read 32 bytes of data from the given slot.
 *		The function returns clear text bytes. Encrypted bytes are read over the wire, then subsequently decrypted
 *		Data zone must be locked and the slot configuration must be set to encrypted read for the block to be successfully read
 *  \param[in]  key_id    The slot id for the encrypted read
 *  \param[in]  block     The block id in the specified slot
 *  \param[out] data      The 32 bytes of clear text data that was read encrypted from the slot, then decrypted
 *  \param[in]  enckey    The key to encrypt with for writing
 *  \param[in]  enckeyid  The keyid of the parent encryption key
 *  returns ATCA_STATUS
 */
ATCA_STATUS atcab_read_enc(uint16_t key_id, uint8_t block, uint8_t *data, const uint8_t* enckey, const uint16_t enckeyid)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t zone = ATCA_ZONE_DATA | ATCA_ZONE_READWRITE_32;
	atca_nonce_in_out_t nonceParam;
	atca_gen_dig_in_out_t genDigParam;
	atca_temp_key_t tempkey;
    uint8_t sn[32];
	uint8_t numin[NONCE_NUMIN_SIZE] = { 0 };
	uint8_t randout[RANDOM_NUM_SIZE] = { 0 };
	int i = 0;

	do {
		// Verify inputs parameters
		if (data == NULL || enckey == NULL) {
			status = ATCA_BAD_PARAM;
			break;
		}
        
        // Read the device SN
        if ((status = atcab_read_zone(ATCA_ZONE_CONFIG, 0, 0, 0, sn, 32)) != ATCA_SUCCESS)
        break;
        // Make the SN continuous by moving SN[4:8] right after SN[0:3]
        memmove(&sn[4], &sn[8], 5);
        
		// Send the random Nonce command
		if ((status = atcab_nonce_rand(numin, randout)) != ATCA_SUCCESS) BREAK(status, "Nonce failed");

		// Calculate Tempkey
        nonceParam.mode = NONCE_MODE_SEED_UPDATE;
        nonceParam.num_in = (uint8_t*)&numin;
        nonceParam.rand_out = (uint8_t*)&randout;
        nonceParam.temp_key = &tempkey;
		if ((status = atcah_nonce(&nonceParam)) != ATCA_SUCCESS) BREAK(status, "Calc TempKey failed");
        
		// Send the GenDig command
		if ((status = atcab_gendig(GENDIG_ZONE_DATA, enckeyid, NULL, 0)) != ATCA_SUCCESS) BREAK(status, "GenDig failed");

		// Calculate Tempkey
        genDigParam.key_id = enckeyid;
        genDigParam.sn = sn;
        genDigParam.stored_value = enckey;
        genDigParam.zone = GENDIG_ZONE_DATA;
        genDigParam.temp_key = &tempkey;
		if ((status = atcah_gen_dig(&genDigParam)) != ATCA_SUCCESS) BREAK(status, "");

		// Read Encrypted
		if ((status = atcab_read_zone(zone, key_id, block, 0, data, ATCA_BLOCK_SIZE)) != ATCA_SUCCESS) BREAK(status, "Read encrypted failed");

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
 *  \param[in] key_id
 *  \param[in] block
 *  \param[in] data      The 32 bytes of clear text data to be written to the slot
 *  \param[in] enckey    The key to encrypt with for writing
 *  \param[in] enckeyid  The keyid of the parent encryption key
 *  returns ATCA_STATUS
 */
ATCA_STATUS atcab_write_enc(uint16_t key_id, uint8_t block, const uint8_t *data, const uint8_t* enckey, const uint16_t enckeyid)
{
    ATCA_STATUS status = ATCA_GEN_FAIL;
    uint8_t zone = ATCA_ZONE_DATA | ATCA_ZONE_READWRITE_32;
    atca_nonce_in_out_t nonceParam;
    atca_gen_dig_in_out_t genDigParam;
    atca_write_mac_in_out_t writeMacParam;
    atca_temp_key_t tempkey;
    uint8_t sn[32];
    uint8_t numin[NONCE_NUMIN_SIZE] = { 0 };
    uint8_t randout[RANDOM_NUM_SIZE] = { 0 };
    uint8_t cipher_text[ATCA_KEY_SIZE] = { 0 };
    uint8_t mac[WRITE_MAC_SIZE] = { 0 };
    uint16_t addr;

    do {
        // Verify inputs parameters
        if (data == NULL || enckey == NULL) {
            status = ATCA_BAD_PARAM;
            break;
        }
        
        // Read the device SN
        if ((status = atcab_read_zone(ATCA_ZONE_CONFIG, 0, 0, 0, sn, 32)) != ATCA_SUCCESS)
            break;
        // Make the SN continuous by moving SN[4:8] right after SN[0:3]
        memmove(&sn[4], &sn[8], 5);
        
        
        // Random Nonce inputs
        nonceParam.mode = NONCE_MODE_SEED_UPDATE;
        nonceParam.num_in = (uint8_t*)&numin;
        nonceParam.rand_out = (uint8_t*)&randout;
        nonceParam.temp_key = &tempkey;

        // Send the random Nonce command
        if ((status = atcab_nonce_rand(numin, randout)) != ATCA_SUCCESS) BREAK(status, "Nonce failed");

        // Calculate Tempkey
        if ((status = atcah_nonce(&nonceParam)) != ATCA_SUCCESS) BREAK(status, "Calc TempKey failed");
        
        // Send the GenDig command
        if ((status = atcab_gendig(GENDIG_ZONE_DATA, enckeyid, NULL, 0)) != ATCA_SUCCESS) BREAK(status, "GenDig failed");

        // Calculate Tempkey
        genDigParam.key_id = enckeyid;
        genDigParam.sn = sn;
        genDigParam.stored_value = (uint8_t*)enckey;
        genDigParam.zone = GENDIG_ZONE_DATA;
        genDigParam.temp_key = &tempkey;
        if ((status = atcah_gen_dig(&genDigParam)) != ATCA_SUCCESS) BREAK(status, "");
        
        // The get address function checks the remaining variables
        if ((status = atcab_get_addr(ATCA_ZONE_DATA, key_id, block, 0, &addr)) != ATCA_SUCCESS) BREAK(status, "Get address failed");

        writeMacParam.zone = zone;
        writeMacParam.key_id = addr;
        writeMacParam.sn = sn;
        writeMacParam.input_data = data;
        writeMacParam.encrypted_data = cipher_text;
        writeMacParam.auth_mac = mac;
        writeMacParam.temp_key = &tempkey;
        
        if ((status = atcah_write_auth_mac(&writeMacParam)) != ATCA_SUCCESS) BREAK(status, "Calculate Auth MAC failed");
        
        status = atcab_write(writeMacParam.zone, writeMacParam.key_id, writeMacParam.encrypted_data, writeMacParam.auth_mac);

    } while (0);
    
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

			memset(packet.info, 0x00, 130);

			// receive the response
			if ( (status = atreceive( _gIface, packet.info, &packet.rxsize)) != ATCA_SUCCESS )
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
			if ( (status = isATCAError(packet.info)) != ATCA_SUCCESS )
				break;

			// update the word address after reading each block
			++offset;
			if ((block == 2) && (offset > 7))
				block = 3;

			// copy the contents to a config data buffer
			memcpy(&config_data[index], &packet.info[1], ATCA_WORD_SIZE );
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

			memset(packet.info, 0x00, sizeof(packet.info));

			// receive the response
			if ( (status = atreceive( _gIface, packet.info, &packet.rxsize)) != ATCA_SUCCESS )
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
			if ( (status = isATCAError(packet.info)) != ATCA_SUCCESS )
				break;

			// update the word address after reading each block
			++block;

			// copy the contents to a config data buffer
			memcpy(&config_data[index], &packet.info[1], ATCA_BLOCK_SIZE );
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
				memset(packet.info, 0x00, 130);
				// read 4 bytes at once
				packet.param1 = ATCA_ZONE_CONFIG;
				// build a write command (write from the start)
				if ( (status = atcab_get_addr(zone, slot, block, offset, &addr)) != ATCA_SUCCESS )
					break;

				packet.param2 =  addr;
				memcpy(&packet.info[0], &config_data[index + 16], ATCA_WORD_SIZE);
				index += ATCA_WORD_SIZE;
				status = atWrite(_gCommandObj, &packet, false);
				execution_time = atGetExecTime( _gCommandObj, CMD_WRITEMEM);

				if ( (status = atcab_wakeup()) != ATCA_SUCCESS ) break;

				// send the command
				if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
					break;

				// delay the appropriate amount of time for command to execute
				atca_delay_ms(execution_time);

				// receive the response
				if ( (status = atreceive( _gIface, packet.info, &packet.rxsize)) != ATCA_SUCCESS )
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

				if ( (status = isATCAError(packet.info)) != ATCA_SUCCESS )
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
			memset(packet.info, 0x00, 130);
			// read 32 bytes at once
			packet.param1 = ATCA_ZONE_CONFIG | ATCA_ZONE_READWRITE_32;
			// build a write command (write from the start)
			atcab_get_addr(zone, slot, block, offset, &addr);
			packet.param2 =  addr;
			memcpy(&packet.info[0], &config_data[index + 16], ATCA_BLOCK_SIZE);
			index += ATCA_BLOCK_SIZE;
			if ( (status = atWrite(_gCommandObj, &packet, false)) != ATCA_SUCCESS )
				break;

			execution_time = atGetExecTime( _gCommandObj, CMD_WRITEMEM);

			if ( (status = atcab_wakeup()) != ATCA_SUCCESS ) break;

			// send the command
			if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
				break;

			// delay the appropriate amount of time for command to execute
			atca_delay_ms(execution_time);

			// receive the response
			if ( (status = atreceive( _gIface, packet.info, &packet.rxsize)) != ATCA_SUCCESS )
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

			if ( (status = isATCAError(packet.info)) != ATCA_SUCCESS )
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

		status = atcab_read_bytes_zone(ATCA_ZONE_CONFIG, 0, 0x00, config_data, ATCA_SHA_CONFIG_SIZE);
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

		status = atcab_write_bytes_zone(ATCA_ZONE_CONFIG, 0, 16, &config_data[16], ATCA_SHA_CONFIG_SIZE - 16);
		if ( status != ATCA_SUCCESS )
			break;

	} while (0);

	return status;
}

/** \brief given an SHA configuration zone buffer and dev type, read its parts from the device's config zone
 *  \param[out] config_data  pointer to buffer containing a contiguous set of bytes to write to the config zone
 *  \returns ATCA_STATUS
 */
ATCA_STATUS atcab_read_config_zone(uint8_t* config_data)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	do {

		// Verify the inputs
		if ( config_data == NULL ) {
			status = ATCA_BAD_PARAM;
			break;
		}

		if (atgetifacecfg(_gIface)->devtype == ATSHA204A)
			status = atcab_read_bytes_zone(ATCA_ZONE_CONFIG, 0, 0x00, config_data, ATCA_SHA_CONFIG_SIZE);
		else
			status = atcab_read_bytes_zone(ATCA_ZONE_CONFIG, 0, 0x00, config_data, ATCA_CONFIG_SIZE);

		if ( status != ATCA_SUCCESS )
			break;

	} while (0);

	return status;
}

/** \brief given an SHA configuration zone buffer and dev type, write its parts to the device's config zone
 *  \param[in] config_data pointer to buffer containing a contiguous set of bytes to write to the config zone
 *  \returns ATCA_STATUS
 */
ATCA_STATUS atcab_write_config_zone(const uint8_t* config_data)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	do {

		// Verify the inputs
		if ( config_data == NULL ) {
			status = ATCA_BAD_PARAM;
			break;
		}

		if (atgetifacecfg(_gIface)->devtype == ATSHA204A)
			status = atcab_write_bytes_zone(ATCA_ZONE_CONFIG, 0, 16, &config_data[16], ATCA_SHA_CONFIG_SIZE - 16);
		else
			status = atcab_write_bytes_zone(ATCA_ZONE_CONFIG, 0, 16, &config_data[16], ATCA_CONFIG_SIZE - 16);

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

/** \brief The Lock command prevents future modifications of the Configuration
 *         and/or Data and OTP zones. If the device is so configured, then
 *         this command can be used to lock individual data slots. This
 *         command fails if the designated area is already locked.
 *
 * \param[in]  mode           Zone, and/or slot, and summary check (bit 7).
 * \param[in]  summary        CRC of the config or data zones. Ignored for
 *                            slot locks or when mode bit 7 is set.
 * \param[out] lock_response  Command response is returned here. Can be NULL
 *                            if not required.
 *
 *  \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_lock(uint8_t mode, uint16_t summary, uint8_t* lock_response)
{
    ATCA_STATUS status = ATCA_GEN_FAIL;
    ATCAPacket packet;
    uint16_t execution_time = 0;

    // build command for lock zone and send
    memset(&packet, 0, sizeof(packet));
    packet.param1 = mode;
    packet.param2 = summary;

    do {
        if ((status = atLock(_gCommandObj, &packet)) != ATCA_SUCCESS) break;

        execution_time = atGetExecTime(_gCommandObj, CMD_LOCK);

        if ((status = atcab_wakeup()) != ATCA_SUCCESS) break;

        // send the command
        if ((status = atsend(_gIface, (uint8_t*)&packet, packet.txsize)) != ATCA_SUCCESS)
            break;

        // delay the appropriate amount of time for command to execute
        atca_delay_ms(execution_time);

        // receive the response
        if ((status = atreceive(_gIface, packet.info, &packet.rxsize)) != ATCA_SUCCESS)
            break;

        // Check response size
        if (packet.rxsize < 4) {
            if (packet.rxsize > 0)
                status = ATCA_RX_FAIL;
            else
                status = ATCA_RX_NO_RESPONSE;
            break;
        }

        if (lock_response != NULL)
            *lock_response = packet.info[ATCA_RSP_DATA_IDX];

        //check the response for error
        if ((status = isATCAError(packet.info)) != ATCA_SUCCESS)
            break;
    } while (0);

    _atcab_exit();
    return status;
}

/** \brief Unconditionally (no CRC required) lock the config zone.
 *
 *  \param[out] lock_response  Pointer to the lock response from the chip.
 *                             0 is successful lock.
 *
 *  \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_lock_config_zone(uint8_t* lock_response)
{
    if (lock_response == NULL)
        return ATCA_BAD_PARAM;

    return atcab_lock(LOCK_ZONE_NO_CRC | LOCK_ZONE_CONFIG, 0, lock_response);
}

/** \brief Lock the config zone with summary CRC.
 *
 *  The CRC is calculated over the entire config zone contents. 88 bytes for
 *  ATSHA devices, 128 bytes for ATECC devices. Lock will fail if the provided
 *  CRC doesn't match the internally calculated one.
 *
 *  \param[in] crc  Expected CRC over the config zone.
 *
 *  \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_lock_config_zone_crc(uint16_t crc)
{
    return atcab_lock(LOCK_ZONE_CONFIG, crc, NULL);
}

/** \brief Unconditionally (no CRC required) lock the data zone (slots and OTP). 
 *
 *	ConfigZone must be locked and DataZone must be unlocked for the zone to be successfully locked.
 *
 *  \param[out] lock_response  Pointer to the lock response from the chip.
 *                             0 is successful lock.
 *
 *  \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_lock_data_zone(uint8_t* lock_response)
{
    if (lock_response == NULL)
        return ATCA_BAD_PARAM;

    return atcab_lock(LOCK_ZONE_NO_CRC | LOCK_ZONE_DATA, 0, lock_response);
}

/** \brief Lock the data zone (slots and OTP) with summary CRC.
 *
 *  The CRC is calculated over the concatenated contents of all the slots and
 *  OTP at the end. Private keys (KeyConfig.Private=1) are skipped. Lock will
 *  fail if the provided CRC doesn't match the internally calculated one.
 *
 *  \param[in] crc  Expected CRC over the data zone.
 *
 *  \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_lock_data_zone_crc(uint16_t crc)
{
    return atcab_lock(LOCK_ZONE_DATA, crc, NULL);
}

/** \brief Lock an individual slot in the data zone on an ATECC device. Not
 *         available for ATSHA devices. Slot must be configured to be slot
 *         lockable (KeyConfig.Lockable=1).
 *
 *  \param[in]  slot           Slot to be locked in data zone.
 *  \param[out] lock_response  Pointer to the lock response from the chip.
 *                             0 is successful lock.
 *
 *  \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_lock_data_slot(uint8_t slot, uint8_t* lock_response)
{
    if (lock_response == NULL)
        return ATCA_BAD_PARAM;

    return atcab_lock((slot << 2) | LOCK_ZONE_DATA_SLOT, 0, lock_response);
}

/** \brief The Sign command generates a signature using the ECDSA algorithm.
 *
 * For external messages, it must be loaded into TempKey.
 *
 * \param[in]  mode       Bit mask indicating signing mode.
 *                          Bit 0: 1 for Verify(Invalidate),
 *                            0 for Verify(Validate) and all others
 *                          Bits 1-5: Must be 0
 *                          Bit 6: If bit 7 is 1, include SN[2:3] and SN[4:7]
 *                            in message, ignored otherwise
 *                          Bit 7: 0 for internal message, 1 for external
 *                            message
 * \param[in]  key_id     Private key slot used to sign the message.
 * \param[out] signature  Signature is returned here. Format is R and S
 *                          integers in big-endian format. 64 bytes for P256
 *                          curve.
 *
 * \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_sign_base(uint8_t mode, uint16_t key_id, uint8_t *signature)
{
    ATCA_STATUS status = ATCA_GEN_FAIL;
    ATCAPacket packet;
    uint16_t execution_time = 0;
    
    if (signature == NULL)
        return ATCA_BAD_PARAM;
    
    if ( !_gDevice )
        return ATCA_GEN_FAIL;
    
    do {
        // Build sign command
        packet.param1 = mode;
        packet.param2 = key_id;
        if ((status = atSign(_gCommandObj, &packet)) != ATCA_SUCCESS)
            break;

        execution_time = atGetExecTime(_gCommandObj, CMD_SIGN);

        if ((status != atcab_wakeup()) != ATCA_SUCCESS)
            break;

        // send the command
        if ((status = atsend(_gIface, (uint8_t*)&packet, packet.txsize)) != ATCA_SUCCESS)
            break;

        // delay the appropriate amount of time for command to execute
        atca_delay_ms(execution_time);

        // receive the response
        if ((status = atreceive(_gIface, packet.info, &(packet.rxsize))) != ATCA_SUCCESS)
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
        if ((status = isATCAError(packet.info)) != ATCA_SUCCESS)
            break;
        
        if (packet.info[ATCA_COUNT_IDX] > 4)
            memcpy(signature, &packet.info[ATCA_RSP_DATA_IDX], packet.info[ATCA_COUNT_IDX]-3);
    } while (0);

    _atcab_exit();
    return status;
}

/** \brief Sign a 32-byte message using the private key in the specified slot
 *
 *  \param[in]  key_id     Slot of the private key to be used to sign the
 *                           message.
 *  \param[in]  msg        32-byte message to be signed. Typically the
 *                           SHA256 hash of the full message.
 *  \param[out] signature  Signature is returned here. Format is R and S
 *                           integers in big-endian format. 64 bytes for P256
 *                           curve.
 *
 * \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_sign(uint16_t key_id, const uint8_t *msg, uint8_t *signature)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
    
	do {
        // Make sure RNG has updated its seed
		if ( (status = atcab_random(NULL)) != ATCA_SUCCESS )
            break;
        // Load message into TempKey
		if ( (status = atcab_challenge( msg )) != ATCA_SUCCESS )
            break;
        // Sign the message
        if ( (status = atcab_sign_base(SIGN_MODE_EXTERNAL, key_id, signature)) != ATCA_SUCCESS)
            break;
	} while (0);
    
	return status;
}

/** \brief Sign an internally generated message.
 *
 *  \param[in]  key_id         Slot of the private key to be used to sign the
 *                               message.
 *  \param[in]  is_invalidate  Set to true if the signature will be used with
 *                               the Verify(Invalidate) command. false for all
 *                               other cases.
 *  \param[in]  is_full_sn     Set to true if the message should incorporate
 *                               the device's full serial number.
 *  \param[out] signature      Signature is returned here. Format is R and S
 *                               integers in big-endian format. 64 bytes for
 *                               P256 curve.
 *
 * \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_sign_internal(uint16_t key_id, bool is_invalidate, bool is_full_sn, uint8_t *signature)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t mode = SIGN_MODE_INTERNAL;
    
	do {
		// Sign the message
        if (is_invalidate)
            mode |= SIGN_MODE_INVALIDATE;
        if (is_full_sn)
            mode |= SIGN_MODE_INCLUDE_SN;
        if ((status = atcab_sign_base(mode, key_id, signature)) != ATCA_SUCCESS)
            break;
	} while (0);
    
	return status;
}

/** \brief Issues a GenDig command, which performs a SHA256 hash on the source data indicated by zone with the
 *  contents of TempKey.  See the CryptoAuth datasheet for your chip to see what the values of zone
 *  correspond to.
 *  \param[in] zone             Designates the source of the data to hash with TempKey.
 *  \param[in] key_id           Indicates the key, OTP block, or message order for shared nonce mode.
 *  \param[in] other_data       Four bytes of data for SHA calculation when using a NoMac key, 32 bytes for
 *                              "Shared Nonce" mode, otherwise ignored (can be NULL).
 *  \param[in] other_data_size  Size of other_data in bytes.
 *  \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_gendig(uint8_t zone, uint16_t key_id, const uint8_t *other_data, uint8_t other_data_size)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	bool hasMACKey = 0;

	if ( !_gDevice)
		return ATCA_GEN_FAIL;
    if (other_data_size > 0 && other_data == NULL)
        return ATCA_BAD_PARAM;

	do {

		// build gendig command
		packet.param1 = zone;
		packet.param2 = key_id;

		if ( packet.param1 == GENDIG_ZONE_SHARED_NONCE && other_data_size >= ATCA_BLOCK_SIZE)
			memcpy(&packet.info[0], &other_data[0], ATCA_BLOCK_SIZE);
		else if ( packet.param1 == GENDIG_ZONE_DATA && other_data_size >= ATCA_WORD_SIZE) {
			memcpy(&packet.info[0], &other_data[0], ATCA_WORD_SIZE);
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
		if ( (status = atreceive( _gIface, packet.info, &(packet.rxsize))) != ATCA_SUCCESS )
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
		if ( (status = isATCAError(packet.info)) != ATCA_SUCCESS )
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
 *  This method will use GenKey to generate the corresponding public key from the private key in the given slot.
 *  \param[in] key_id ID of the private key slot
 *  \param[out] public_key - pointer to space receiving the contents of the public key that was generated
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_get_pubkey(uint16_t key_id, uint8_t *public_key)
{
    return atcab_genkey_base(GENKEY_MODE_PUBLIC, key_id, NULL, public_key);
}

/** \brief write a P256 private key in given slot using mac computation
 *  \param[in] key_id
 *  \param[in] priv_key first 4 bytes of 36 bytes should be zero for P256 curve
 *  \param[in] write_key_slot slot to make a session key
 *  \param[in] write_key key to make a session key
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_priv_write(uint16_t key_id, const uint8_t priv_key[36], uint8_t write_key_slot, const uint8_t write_key[32])
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	atca_nonce_in_out_t nonceParam;
	atca_gen_dig_in_out_t genDigParam;
	atca_write_mac_in_out_t hostMacParam;
	atca_temp_key_t tempkey;
    uint8_t sn[32]; // Buffer is larger than the 9 bytes required to make reads easier
	uint8_t numin[NONCE_NUMIN_SIZE] = { 0 };
	uint8_t randout[RANDOM_NUM_SIZE] = { 0 };
	uint8_t cipher_text[36] = { 0 };
	uint8_t host_mac[MAC_SIZE] = { 0 };
	uint16_t execution_time = 0;

	if (key_id > 15 || priv_key == NULL)
		return ATCA_BAD_PARAM;

	do {

		if (write_key == NULL) {
			// Caller requested an unencrypted PrivWrite, which is only allowed when the data zone is unlocked
			// build an PrivWrite command
			packet.param1 = 0x00;                   // Mode is unencrypted write
			packet.param2 = key_id;                   // Key ID
			memcpy(&packet.info[0], priv_key, 36);  // Private key
			memset(&packet.info[36], 0, 32);        // MAC (ignored for unencrypted write)
		}else {
            // Read the device SN
            if ((status = atcab_read_zone(ATCA_ZONE_CONFIG, 0, 0, 0, sn, 32)) != ATCA_SUCCESS)
                break;
            // Make the SN continuous by moving SN[4:8] right after SN[0:3]
            memmove(&sn[4], &sn[8], 5);
            
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
            if ((status = atcab_gendig(GENDIG_ZONE_DATA, write_key_slot, NULL, 0)) != ATCA_SUCCESS)
                break;

			// Calculate Tempkey
			genDigParam.zone = GENDIG_ZONE_DATA;
            genDigParam.sn = sn;
			genDigParam.key_id = write_key_slot;
			genDigParam.stored_value = write_key;
			genDigParam.temp_key = &tempkey;
			if ((status = atcah_gen_dig(&genDigParam)) != ATCA_SUCCESS)
				break;

			// Calculate Auth MAC and cipher text
			hostMacParam.zone = PRIVWRITE_MODE_ENCRYPT;
			hostMacParam.key_id = key_id;
            hostMacParam.sn = sn;
			hostMacParam.input_data = &priv_key[0];
			hostMacParam.encrypted_data = cipher_text;
			hostMacParam.auth_mac = host_mac;
			hostMacParam.temp_key = &tempkey;
			if ((status = atcah_privwrite_auth_mac(&hostMacParam)) != ATCA_SUCCESS)
				break;

			// build a write command for encrypted writes
			packet.param1 = PRIVWRITE_MODE_ENCRYPT; // Mode is encrypted write
			packet.param2 = key_id;                   // Key ID
			memcpy(&packet.info[0],                   cipher_text, sizeof(cipher_text));
			memcpy(&packet.info[sizeof(cipher_text)], host_mac, sizeof(host_mac));
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
		if ((status = atreceive(_gIface, packet.info, &packet.rxsize)) != ATCA_SUCCESS)
			break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ( (status = isATCAError(packet.info)) != ATCA_SUCCESS )
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
ATCA_STATUS atcab_write_pubkey(uint16_t slot8toF, const uint8_t *pubkey)
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
ATCA_STATUS atcab_read_pubkey(uint16_t slot8toF, uint8_t *pubkey)
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

/** \brief Write data into config, otp, or data zone with a given byte offset
 *         and length. Offset and length must be multiples of a word (4 bytes).
 *
 * Config zone must be unlocked for writes to that zone. Data zone must be
 * locked for writes to OTP and Data zones.
 *
 *  \param[in] zone          Zone to write data to: Config(0), OTP(1), or
 *                           Data(2).
 *  \param[in] slot          If zone is Data(2), the slot number to write to.
 *                           Ignored for all other zones.
 *  \param[in] offset_bytes  Byte offset within the zone to write to. Must be
 *                           a multiple of a word (4 bytes).
 *  \param[in] data          Data to be written.
 *  \param[in] length        Number of bytes to be written. Must be a multiple
 *                           of a word (4 bytes).
 *
 *  \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_write_bytes_zone(uint8_t zone, uint16_t slot, size_t offset_bytes, const uint8_t *data, size_t length)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
    size_t zone_size = 0;
    size_t data_idx = 0;
    size_t cur_block = 0;
    size_t cur_word = 0;
    
    if (zone != ATCA_ZONE_CONFIG && zone != ATCA_ZONE_OTP && zone != ATCA_ZONE_DATA)
        return ATCA_BAD_PARAM;
    if (zone == ATCA_ZONE_DATA && slot > 15)
        return ATCA_BAD_PARAM;
    if (length == 0)
        return ATCA_SUCCESS; // Always succeed writing 0 bytes
    if (data == NULL)
        return ATCA_BAD_PARAM;
    if (offset_bytes % ATCA_WORD_SIZE != 0 || length % ATCA_WORD_SIZE != 0)
        return ATCA_BAD_PARAM;
    
    do 
    {
        status = atcab_get_zone_size(zone, slot, &zone_size);
        if (status != ATCA_SUCCESS)
            break;
        if (offset_bytes + length > zone_size)
            return ATCA_BAD_PARAM;
            
        cur_block = offset_bytes / ATCA_BLOCK_SIZE;
        cur_word = (offset_bytes % ATCA_BLOCK_SIZE) / ATCA_WORD_SIZE;
        
        while (data_idx < length)
        {
            // The last item makes sure we handle the selector, user extra, and lock bytes in the config properly
            if (cur_word == 0 && length - data_idx >= ATCA_BLOCK_SIZE && !(zone == ATCA_ZONE_CONFIG && cur_block == 2))
            {
                status = atcab_write_zone(zone, slot, (uint8_t)cur_block, 0, &data[data_idx], ATCA_BLOCK_SIZE);
                if (status != ATCA_SUCCESS)
                    break;
                data_idx += ATCA_BLOCK_SIZE;
                cur_block += 1;
            }
            else
            {
                if (zone == ATCA_ZONE_CONFIG && cur_block == 2 && cur_word == 5)
                {
                    // These are the selector and user extra bytes, which require special handling
                    status = atcab_updateextra(UPDATE_MODE_USER_EXTRA, data[data_idx]);
                    if (status != ATCA_SUCCESS)
                        break;
                    status = atcab_updateextra(UPDATE_MODE_SELECTOR, data[data_idx + 1]);
                    if (status != ATCA_SUCCESS)
                        break;
                    // Lock bytes (config[86:87]) are ignored
                }
                else
                {
                    status = atcab_write_zone(zone, slot, (uint8_t)cur_block, (uint8_t)cur_word, &data[data_idx], ATCA_WORD_SIZE);
                    if (status != ATCA_SUCCESS)
                        break;
                }
                data_idx += ATCA_WORD_SIZE;
                cur_word += 1;
                if (cur_word == ATCA_BLOCK_SIZE/ATCA_WORD_SIZE)
                {
                    cur_block += 1;
                    cur_word = 0;
                }
            }
        }
    } while (false);
    
	return status;
}

/** \brief Read data from config, otp, or data zone with a given byte offset
 *         and length.
 *
 *  \param[in]  zone          Zone to read data from: Config(0), OTP(1), or
 *                            Data(2).
 *  \param[in]  slot          If zone is Data(2), the slot number to read from.
 *                            Ignored for all other zones.
 *  \param[in]  offset_bytes  Byte offset within the zone to read from.
 *  \param[out] data          Buffer to read data into.
 *  \param[in]  length        Number of bytes to read.
 *
 *  \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_read_bytes_zone(uint8_t zone, uint16_t slot, size_t offset_bytes, uint8_t *data, size_t length)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	size_t zone_size = 0;
    uint8_t read_buf[32];
	size_t data_idx = 0;
	size_t cur_block = 0;
    size_t cur_offset = 0;
    uint8_t read_size = ATCA_BLOCK_SIZE;
	size_t read_buf_idx = 0;
    size_t copy_length = 0;
    size_t read_offset = 0;
	
    if (zone != ATCA_ZONE_CONFIG && zone != ATCA_ZONE_OTP && zone != ATCA_ZONE_DATA)
        return ATCA_BAD_PARAM;
    if (zone == ATCA_ZONE_DATA && slot > 15)
        return ATCA_BAD_PARAM;
    if (length == 0)
        return ATCA_SUCCESS; // Always succeed reading 0 bytes
    if (data == NULL)
        return ATCA_BAD_PARAM;
    
	do
	{
    	status = atcab_get_zone_size(zone, slot, &zone_size);
    	if (status != ATCA_SUCCESS)
    	    break;
    	if (offset_bytes + length > zone_size)
    	    return ATCA_BAD_PARAM;
    	
        cur_block = offset_bytes / ATCA_BLOCK_SIZE;
        
        while (data_idx < length)
        {
            if (read_size == ATCA_BLOCK_SIZE && zone_size - cur_block*ATCA_BLOCK_SIZE < ATCA_BLOCK_SIZE)
            {
                // We have less than a block to read and can't read past the end of the zone, switch to word reads
                read_size = ATCA_WORD_SIZE;
                cur_offset = (offset_bytes / ATCA_WORD_SIZE) % (ATCA_BLOCK_SIZE / ATCA_WORD_SIZE);
            }

            status = atcab_read_zone(
                zone,
                slot,
                (uint8_t)cur_block,
                (uint8_t)cur_offset,
                read_buf,
                read_size);
            if (status != ATCA_SUCCESS)
                break;
            
            read_offset = cur_block*ATCA_BLOCK_SIZE + cur_offset*ATCA_WORD_SIZE;
            if (read_offset < offset_bytes)
                read_buf_idx = offset_bytes - read_offset;
            else
                read_buf_idx = 0;
            
            if (length - data_idx < read_size)
                copy_length = length - data_idx;
            else
                copy_length = read_size;
            
            memcpy(&data[data_idx], &read_buf[read_buf_idx], copy_length);
            data_idx += copy_length;
            if (read_size == ATCA_BLOCK_SIZE)
                cur_block += 1;
            else
                cur_offset += 1;
        }
	} while (false);
	
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
		if (digest == NULL ) {
			status = ATCA_BAD_PARAM;
			break;
		}

		// build mac command
		packet.param1 = mode;
		packet.param2 = key_id;
		if (!(mode & MAC_MODE_BLOCK2_TEMPKEY))
		{
			if (challenge == NULL)
				return ATCA_BAD_PARAM;
			memcpy(&packet.info[0], challenge, 32);  // a 32-byte challenge
		}

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
		if ( (status = atreceive( _gIface, packet.info, &(packet.rxsize))) != ATCA_SUCCESS )
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
		if ( (status = isATCAError(packet.info)) != ATCA_SUCCESS )
			break;

		memcpy( digest, &packet.info[ATCA_RSP_DATA_IDX], MAC_SIZE );

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

    // Verify the inputs
    if (response == NULL || other_data == NULL)
        return ATCA_BAD_PARAM;
    if (!(mode & CHECKMAC_MODE_BLOCK2_TEMPKEY) && challenge == NULL)
        return ATCA_BAD_PARAM;

	do {
		// build checkmac command
		packet.param1 = mode;
		packet.param2 = key_id;
        if (challenge != NULL)
            memcpy(&packet.info[0], challenge, CHECKMAC_CLIENT_CHALLENGE_SIZE);
        else
            memset(&packet.info[0], 0, CHECKMAC_CLIENT_CHALLENGE_SIZE);
		memcpy( &packet.info[32], response, CHECKMAC_CLIENT_RESPONSE_SIZE );
		memcpy( &packet.info[64], other_data, CHECKMAC_OTHER_DATA_SIZE );

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
		if ( (status = atreceive( _gIface, packet.info, &(packet.rxsize))) != ATCA_SUCCESS )
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
		if ( (status = isATCAError(packet.info)) != ATCA_SUCCESS )
			break;
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief The HMAC command computes an HMAC/SHA-256 digest of a key stored in the device, a challenge, and other
 *         information on the device.
 * The output of this command is the output of the HMAC algorithm computed over this
 * key and message. If the message includes the serial number of the device, the response is said to be
 * "diversified".
 *
 * \param[in]  mode    Controls which fields within the device are used in the message.
 * \param[in]  key_id  Which key is to be used to generate the response.
 *                     Bits 0:3 only are used to select a slot but all 16 bits are used in the HMAC message.
 * \param[out] digest  HMAC digest is returned in this buffer (32 bytes).
 *
 * \return ATCA_SUCCESS
 */
ATCA_STATUS atcab_hmac(uint8_t mode, uint16_t key_id, uint8_t *digest)
{
    ATCA_STATUS status = ATCA_GEN_FAIL;
    ATCAPacket packet;
    uint16_t execution_time = 0;
    
    do {
        if (digest == NULL) {
            status = ATCA_BAD_PARAM;
            break;
        }
        // build HMAC command
        packet.param1 = mode;
        packet.param2 = key_id;

        if ( (status = atHMAC( _gCommandObj, &packet )) != ATCA_SUCCESS )
            break;

        execution_time = atGetExecTime( _gCommandObj, CMD_DERIVEKEY);

        if ( (status != atcab_wakeup()) != ATCA_SUCCESS )
            break;

        // send the command
        if ( (status = atsend( _gIface, (uint8_t*)&packet, packet.txsize )) != ATCA_SUCCESS )
            break;

        // delay the appropriate amount of time for command to execute
        atca_delay_ms( execution_time );

        // receive the response
        if ( (status = atreceive( _gIface, packet.info, &(packet.rxsize))) != ATCA_SUCCESS )
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
        if ( (status = isATCAError(packet.info)) != ATCA_SUCCESS )
            break;
        
        if (packet.rxsize != HMAC_DIGEST_SIZE + 3)
        {
            status = ATCA_RX_FAIL; // Unexpected response size
            break;
        }        
        
        memcpy(digest, &packet.info[ATCA_RSP_DATA_IDX], HMAC_DIGEST_SIZE);

    } while (0);

    _atcab_exit();
    return status;
}

/** \brief Send a DeriveKey Command to the device 
 *
 *  \param[in] mode        Bit 2 must match the value in TempKey.SourceFlag
 *  \param[in] target_key  Key slot to be written
 *  \param[in] mac         Optional 32 byte MAC used to validate operation. NULL if not required.
 *
 *  \return ATCA_SUCCESS
 */
ATCA_STATUS atcab_derivekey(uint8_t mode, uint16_t target_key, uint8_t* mac)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	do {

		// build a deriveKey command (pass through mode)
		packet.param1 = mode;
		packet.param2 = target_key;
		if (mac != NULL)
			memcpy(packet.info, mac, 32);

		if ((status = atDeriveKey(_gCommandObj, &packet, mac != NULL)) != ATCA_SUCCESS) break;

		execution_time = atGetExecTime(_gCommandObj, CMD_DERIVEKEY);

		if ((status = atcab_wakeup()) != ATCA_SUCCESS) break;

		// send the command
		if ((status = atsend(_gIface, (uint8_t*)&packet, packet.txsize)) != ATCA_SUCCESS) break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ((status = atreceive(_gIface, packet.info, &(packet.rxsize))) != ATCA_SUCCESS) break;

		// Check response size
		if (packet.rxsize < 4) {
			if (packet.rxsize > 0)
				status = ATCA_RX_FAIL;
			else
				status = ATCA_RX_NO_RESPONSE;
			break;
		}

		if ((status = isATCAError(packet.info)) != ATCA_SUCCESS) break;

	} while (0);

	_atcab_exit();
	return status;
}

/** \brief The SHA command computes a SHA-256 digest for general purpose use
 *         by the system. Any message length can be accommodated. The system
 *         is responsible for sending the pad and length bytes with the last
 *         block for ATSHA devices.
 *
 * Only the Start(0) and Compute(1) modes are available for ATSHA devices.
 *
 * \param[in] mode     SHA command mode: Start(0), Update/Compute(1), End(2),
 *                       Public(3), HMACstart(4), MHACend(5)
 * \param[in] length   Number of bytes in the Message parameter. Must be 0 for
 *                       start command, 64 for update/compute commands, and 0
 *                       to 63 for end commands. KeySlot for the HMAC key if
 *                       Mode is "HMACstart".
 * \param[in] message  Message bytes to be hashed. Can be NULL if not required
 *                       by the mode.
 * \param[in] digest   Digest (32 bytes) is returned here for end commands (or
 *                       compute command for ATSHA devices).
 *
 * \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_sha_base(uint8_t mode, uint16_t length, const uint8_t* message, uint8_t* digest)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	if (length > 0 && message == NULL)
		return ATCA_BAD_PARAM;
	if (length > SHA_BLOCK_SIZE)
		return ATCA_BAD_PARAM; // Command can't handle more than a SHA block

	do {
		// Build SHA command
		packet.param1 = mode;
		packet.param2 = length;
		memcpy(packet.info, message, length);

		if ((status = atSHA(_gCommandObj, &packet)) != ATCA_SUCCESS)
			break;

		execution_time = atGetExecTime(_gCommandObj, CMD_SHA);

		if ((status != atcab_wakeup()) != ATCA_SUCCESS)
			break;

		// send the command
		if ((status = atsend(_gIface, (uint8_t*)&packet, packet.txsize)) != ATCA_SUCCESS)
			break;

		// delay the appropriate amount of time for command to execute
		atca_delay_ms(execution_time);

		// receive the response
		if ((status = atreceive(_gIface, packet.info, &(packet.rxsize))) != ATCA_SUCCESS)
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
		if ((status = isATCAError(packet.info)) != ATCA_SUCCESS)
			break;

		if (packet.rxsize != 4)
		{
			// Data was returned
			if (packet.rxsize != ATCA_SHA_DIGEST_SIZE + 3)
			{
				// Unexpected RX data count
				status = ATCA_RX_FAIL;
				break;
			}
			if (digest != NULL)
				memcpy(digest, &packet.info[ATCA_RSP_DATA_IDX], ATCA_SHA_DIGEST_SIZE);
		}
	} while (0);

	_atcab_exit();
	return status;
}

/** \brief Initialize SHA-256 calculation engine
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_sha_start(void)
{
	return atcab_sha_base(SHA_MODE_SHA256_START, 0, NULL, NULL);
}

/** \brief Adds the message to be digested
 *	\param[in] message  Add 64 bytes in the message parameter to the SHA context
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_sha_update(const uint8_t *message)
{
	return atcab_sha_base(SHA_MODE_SHA256_UPDATE, 64, message, NULL);
}

/** \brief The SHA-256 calculation is complete
 *	\param[out] digest   The SHA256 digest that is calculated
 *  \param[in]  length   Length of any remaining data to include in hash. Max 64 bytes.
 *  \param[in]  message  Remaining data to include in hash. NULL if length is 0.
 *  \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_sha_end(uint8_t *digest, uint16_t length, const uint8_t *message)
{
	return atcab_sha_base(SHA_MODE_SHA256_END, length, message, digest);
}

/** \brief Computes a SHA-256 digest
 *	\param[in] length The number of bytes in the message parameter
 *	\param[in] message - pointer to variable length message
 *	\param[out] digest The SHA256 digest
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcab_sha(uint16_t length, const uint8_t *message, uint8_t *digest)
{
	return atcab_hw_sha2_256(message, length, digest);
}

typedef struct {
	uint32_t total_msg_size;           //!< Total number of message bytes processed
	uint32_t block_size;               //!< Number of bytes in current block
	uint8_t block[SHA_BLOCK_SIZE * 2]; //!< Unprocessed message storage
} hw_sha256_ctx;

ATCA_STATUS atcab_hw_sha2_256_init(atca_sha256_ctx_t* ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	return atcab_sha_start();
}

ATCA_STATUS atcab_hw_sha2_256_update(atca_sha256_ctx_t* ctx, const uint8_t* data, size_t data_size)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint32_t block_count;
	uint32_t rem_size = SHA_BLOCK_SIZE - ctx->block_size;
	uint32_t copy_size = data_size > rem_size ? rem_size : data_size;
	uint32_t i = 0;

	// Copy data into current block
	memcpy(&ctx->block[ctx->block_size], data, copy_size);

	if (ctx->block_size + data_size < SHA_BLOCK_SIZE) {
		// Not enough data to finish off the current block
		ctx->block_size += data_size;
		return ATCA_SUCCESS;
	}

	// Process the current block
	status = atcab_sha_update(ctx->block);
	if (status != ATCA_SUCCESS)
		return status;

	// Process any additional blocks
	data_size -= copy_size; // Adjust to the remaining message bytes
	block_count = data_size / SHA_BLOCK_SIZE;
	for (i = 0; i < block_count; i++)
	{
		status = atcab_sha_update(&data[copy_size + i * SHA_BLOCK_SIZE]);
		if (status != ATCA_SUCCESS)
			return status;
	}

	// Save any remaining data
	ctx->total_msg_size += (block_count + 1) * SHA_BLOCK_SIZE;
	ctx->block_size = data_size % SHA_BLOCK_SIZE;
	memcpy(ctx->block, &data[copy_size + block_count * SHA_BLOCK_SIZE], ctx->block_size);

	return ATCA_SUCCESS;
}

ATCA_STATUS atcab_hw_sha2_256_finish(atca_sha256_ctx_t * ctx, uint8_t digest[ATCA_SHA2_256_DIGEST_SIZE])
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint32_t msg_size_bits;
	uint32_t pad_zero_count;

	if (atgetifacecfg(_gIface)->devtype == ATSHA204A)
	{
		// Calculate the total message size in bits
		ctx->total_msg_size += ctx->block_size;
		msg_size_bits = ctx->total_msg_size * 8;

		// Calculate the number of padding zero bytes required between the 1 bit byte and the SHA_BLOCK_SIZE bit message size in bits.
		pad_zero_count = (SHA_BLOCK_SIZE - ((ctx->block_size + 9) % SHA_BLOCK_SIZE)) % SHA_BLOCK_SIZE;

		// Append a single 1 bit
		ctx->block[ctx->block_size++] = 0x80;

		// Add padding zeros plus upper 4 bytes of total msg size in bits (only supporting 32bit message bit counts)
		memset(&ctx->block[ctx->block_size], 0, pad_zero_count + 4);
		ctx->block_size += pad_zero_count + 4;

		// Add the total message size in bits to the end of the current block. Technically this is
		// supposed to be 8 bytes. This shortcut will reduce the max message size to 536,870,911 bytes.
		ctx->block[ctx->block_size++] = (uint8_t)(msg_size_bits >> 24);
		ctx->block[ctx->block_size++] = (uint8_t)(msg_size_bits >> 16);
		ctx->block[ctx->block_size++] = (uint8_t)(msg_size_bits >> 8);
		ctx->block[ctx->block_size++] = (uint8_t)(msg_size_bits >> 0);

		status = atcab_sha_base(SHA_MODE_SHA256_UPDATE, SHA_BLOCK_SIZE, ctx->block, digest);
		if (status != ATCA_SUCCESS)
			return status;
		if (ctx->block_size > SHA_BLOCK_SIZE)
		{
			status = atcab_sha_base(SHA_MODE_SHA256_UPDATE, SHA_BLOCK_SIZE, &ctx->block[SHA_BLOCK_SIZE], digest);
			if (status != ATCA_SUCCESS)
				return status;
		}
	}
	else
	{
		status = atcab_sha_end(digest, ctx->block_size, ctx->block);
		if (status != ATCA_SUCCESS)
			return status;
	}

	return ATCA_SUCCESS;
}

ATCA_STATUS atcab_hw_sha2_256(const uint8_t * data, size_t data_size, uint8_t digest[ATCA_SHA2_256_DIGEST_SIZE])
{
	ATCA_STATUS status = ATCA_SUCCESS;
	atca_sha256_ctx_t ctx;

	status = atcab_hw_sha2_256_init(&ctx);
	if (status != ATCA_SUCCESS)
		return status;

	status = atcab_hw_sha2_256_update(&ctx, data, data_size);
	if (status != ATCA_SUCCESS)
		return status;

	status = atcab_hw_sha2_256_finish(&ctx, digest);
	if (status != ATCA_SUCCESS)
		return status;

	return ATCA_SUCCESS;
}

/** \brief The UpdateExtra command is used to update the values of the two
 *         extra bytes within the Configuration zone (bytes 84 and 85).
 *
 * Can also be used to decrement the limited use counter associated with the
 * key in slot NewValue.
 *
 * \param[in] mode       Bit 0: 0 = update user extra (config[84]),
 *                              1 = update selector (config[85])
 *                       Bit 1: Ignores bit 0 and decrements the limited use
 *                              counter associated with the key in new_value.
 * \param[in] new_value  When mode bit 1 is set, this the key for which the
 *                       limited use counter will be decremented.
 *
 * \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcab_updateextra(uint8_t mode, uint16_t new_value)
{
    ATCA_STATUS status = ATCA_GEN_FAIL;
    ATCAPacket packet;
    uint16_t execution_time = 0;

    do {
        // Build command
        memset(&packet, 0, sizeof(packet));
        packet.param1 = mode;
        packet.param2 = new_value;

        if ((status = atUpdateExtra(_gCommandObj, &packet)) != ATCA_SUCCESS)
            break;

        execution_time = atGetExecTime(_gCommandObj, CMD_UPDATEEXTRA);

        if ((status != atcab_wakeup()) != ATCA_SUCCESS)
            break;

        // send the command
        if ((status = atsend(_gIface, (uint8_t*)&packet, packet.txsize)) != ATCA_SUCCESS)
            break;

        // delay the appropriate amount of time for command to execute
        atca_delay_ms(execution_time);

        // receive the response
        if ((status = atreceive(_gIface, packet.info, &(packet.rxsize))) != ATCA_SUCCESS)
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
        if ((status = isATCAError(packet.info)) != ATCA_SUCCESS)
            break;
    } while (0);

    _atcab_exit();
    return status;
}