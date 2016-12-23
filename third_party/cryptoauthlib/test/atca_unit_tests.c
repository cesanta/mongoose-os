/**
 * \file
 * \brief  Unit tests for CryptoAuthLib.  These tests are based on the Unity C unit test framework.
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

#include "unity.h"
#include "cryptoauthlib.h"
#include "basic/atca_basic.h"
#include "atca_unit_tests.h"
#include "host/atca_host.h"

// gCfg must point to one of the cfg_ structures for any unit test to work.  this allows
// the command console to switch device types at runtime.
ATCAIfaceCfg *gCfg = NULL;

const uint8_t g_slot4_key[32] = {
    0x37, 0x80, 0xe6, 0x3d, 0x49, 0x68, 0xad, 0xe5, 0xd8, 0x22, 0xc0, 0x13, 0xfc, 0xc3, 0x23, 0x84,
    0x5d, 0x1b, 0x56, 0x9f, 0xe7, 0x05, 0xb6, 0x00, 0x06, 0xfe, 0xec, 0x14, 0x5a, 0x0d, 0xb1, 0xe3
};

// test runner
int atca_unit_tests(ATCADeviceType deviceType)
{
	UnityBegin("atca_unit_tests.c");

	// do this set of tests regardless of which device type is requested

	RUN_TEST(test_objectNew);
	RUN_TEST(test_objectDelete);

	switch ( deviceType ) {
	case ATSHA204A:
		#ifdef ATCA_HAL_I2C
		gCfg = &cfg_sha204a_i2c_default;
		#elif defined(ATCA_HAL_SWI)
		gCfg = &cfg_sha204a_swi_default;
		#endif
		atca_sha204a_unit_tests(deviceType);
		break;
	case ATECC108A:
		#ifdef ATCA_HAL_I2C
		gCfg = &cfg_ateccx08a_i2c_default;
		#elif defined(ATCA_HAL_SWI)
		gCfg = &cfg_ateccx08a_swi_default;
		#endif
		gCfg->devtype = ATECC108A;
		atca_ecc108a_unit_tests(deviceType);
		break;
	case ATECC508A:
		#ifdef ATCA_HAL_I2C
		gCfg = &cfg_ateccx08a_i2c_default;
		#elif defined(ATCA_HAL_SWI)
		gCfg = &cfg_ateccx08a_swi_default;
		#elif defined(ATCA_HAL_KIT_CDC)
		gCfg = &cfg_ecc508_kitcdc_default;
		#elif defined(ATCA_HAL_KIT_HID)
		gCfg = &cfg_ecc508_kithid_default;
		#endif
		gCfg->devtype = ATECC508A;
		atca_ecc508a_unit_tests(deviceType);
		break;
	default:
		TEST_FAIL_MESSAGE("Unhandled device type");
		break;
	}

	UnityEnd();
	return ATCA_SUCCESS;
}

int atcau_get_addr(uint8_t zone, uint8_t slot, uint8_t block, uint8_t offset, uint16_t* addr)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	if (addr == NULL) return ATCA_BAD_PARAM;
	if (zone != ATCA_ZONE_CONFIG && zone != ATCA_ZONE_DATA && zone != ATCA_ZONE_OTP) {
		return ATCA_BAD_PARAM;;
	}
	*addr = 0;
	offset = offset & (uint8_t)0x07;

	if ((zone == ATCA_ZONE_CONFIG) || (zone == ATCA_ZONE_OTP)) {
		*addr = block << 3;
		*addr |= offset;
	}else if (zone == ATCA_ZONE_DATA) {
		*addr = slot << 3;
		*addr  |= offset;
		*addr |= block << 8;
	}else
		status = ATCA_BAD_PARAM;
	return status;
}

static ATCA_STATUS test_command(ATCACommand commandObj, ATCAIface iface, ATCA_CmdMap cmd, ATCAPacket* packet)
{
    ATCA_STATUS status = ATCA_SUCCESS;
    uint16_t execution_time = atGetExecTime(commandObj, cmd);
    
    // Wakeup
    status = atwake(iface);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Send command
    status = atsend(iface, (uint8_t*)packet, packet->txsize);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Delay the appropriate amount of time for command to execute
    atca_delay_ms(execution_time);
    
    // Receive the response
    status = atreceive(iface, packet->info, &(packet->rxsize));
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Check for command errors
    status = isATCAError(packet->info);
    
    // Go back to idle
    atidle(iface);
    
    return status;
}

int atcau_is_locked(uint8_t zone, uint8_t *lock_state)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status = ATCA_GEN_FAIL;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint8_t word_data[ATCA_WORD_SIZE];
	uint8_t zone_idx = 2;

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	atsleep(iface);

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// build an read command
	packet.param1 = 0x00;
	packet.param2 = 0x15;
	status = atRead( commandObj, &packet );

	execution_time = atGetExecTime( commandObj, CMD_READMEM);

	// send the command
	status = atsend(iface, (uint8_t*)&packet, packet.txsize);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	memcpy( word_data, &packet.info[1], sizeof(word_data));

	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);
	TEST_ASSERT_NULL( device );

	// Determine the index into the word_data based on the zone we are querying for
	if (zone == LOCK_ZONE_DATA) zone_idx = 2;
	if (zone == LOCK_ZONE_CONFIG) zone_idx = 3;

	// Set the locked return variable base on the value.
	if (word_data[zone_idx] == 0)
		*lock_state = true;
	else
		*lock_state = false;

	return status;
}

void test_lock_zone(void)
{
	ATCA_STATUS status;
	uint8_t isLocked = false;

	status = atcau_is_locked( ATCA_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcau_is_locked( ATCA_ZONE_DATA, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_objectNew(void)
{
	ATCADevice device;

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	deleteATCADevice(&device);
	TEST_ASSERT_NULL( device ); // see test_objectDelete for explanation
}

void test_objectDelete(void)
{
	ATCADevice device;

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	                            // so it can be tested, ATCA objects are already pointers, so this
	                            // is passing a double indirect
	TEST_ASSERT_NULL( device );
}

void test_wake_sleep(void)
{
	ATCADevice device;
	ATCAIface iface;
	ATCA_STATUS status;

	device = newATCADevice(gCfg);

	iface = atGetIFace(device);

	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	TEST_ASSERT_NULL( device );
}

void test_wake_idle(void)
{
	ATCADevice device;
	ATCAIface iface;
	ATCA_STATUS status;

	device = newATCADevice(gCfg);

	iface = atGetIFace(device);

	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atidle(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	TEST_ASSERT_NULL( device );
}


void test_crcerror(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;

	ATCA_STATUS status;
	ATCAPacket packet;

	if (gCfg->iface_type == ATCA_HID_IFACE)
		TEST_IGNORE_MESSAGE("Kit protocol corrects CRC errors.");

	uint16_t execution_time = 0;

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build an info command
	packet.param1 = INFO_MODE_REVISION;   // these tests are for communication testing mainly,
	// but if testing the entire chip, would need to go through all the modes.
	// this tests version mode only
	status = atInfo( commandObj, &packet );
	execution_time = atGetExecTime( commandObj, CMD_INFO);

	// hack up the packet so CRC is broken
	packet.info[0] = 0xff;
	packet.info[1] = 0xff;

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// test to make sure CRC error is in the packet
	TEST_ASSERT_EQUAL_INT8_MESSAGE( 0x04, packet.info[0], "Failed error response length test");
	TEST_ASSERT_EQUAL_INT8_MESSAGE( 0xff, packet.info[1], "Failed bad CRC test");

	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );

}

void test_checkmac(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint16_t keyID = 0x0004;
	static uint8_t response_mac[MAC_RSP_SIZE];              // Make the response buffer the size of a MAC response.
	static uint8_t other_data[CHECKMAC_OTHER_DATA_SIZE];    // First four bytes of Mac command are needed for CheckMac command.
	uint8_t isLocked = false;

	status = atcau_is_locked( ATCA_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	status = atcau_is_locked( ATCA_ZONE_DATA, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed.");

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	if (gCfg->devtype == ATSHA204A)
		keyID = 0x0001;
	else
		keyID = 0x0004;
	// build a mac command
	packet.param1 = MAC_MODE_CHALLENGE;
	packet.param2 = keyID;
	memset( packet.info, 0x55, 32 );  // a 32-byte challenge

	status = atMAC( commandObj, &packet );
	execution_time = atGetExecTime( commandObj, CMD_MAC);
	TEST_ASSERT_EQUAL( ATCA_RSP_SIZE_32, packet.rxsize );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &packet.rxsize);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	memcpy(response_mac, packet.info, sizeof(response_mac));

	// sleep or idle
	status = atidle(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build a checkmac command
	packet.param1 = MAC_MODE_CHALLENGE;
	packet.param2 = keyID;
	memset( packet.info, 0x55, 32 );  // a 32-byte challenge
	memcpy(&packet.info[32], &response_mac[1], 32);
	memset(other_data, 0, sizeof(other_data));
	other_data[0] = ATCA_MAC;
	other_data[2] = (uint8_t)keyID;
	memcpy(&packet.info[64], other_data, sizeof(other_data));

	status = atCheckMAC( commandObj, &packet );
	execution_time = atGetExecTime( commandObj, CMD_CHECKMAC);
	TEST_ASSERT_EQUAL( CHECKMAC_RSP_SIZE, packet.rxsize );

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL_INT8_MESSAGE( 0x00, packet.info[ATCA_RSP_DATA_IDX], "Failed CheckMac test");

	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );
}

void test_counter(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;

	ATCA_STATUS status;
	ATCAPacket packet;

	uint16_t execution_time = 0;
	uint8_t increased_bin_val[4] = { 0x00 };

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build a counter command
	packet.param1 = COUNTER_MODE_INCREASE;
	packet.param2 = 0x0000;
	status = atCounter( commandObj, &packet );
	TEST_ASSERT_EQUAL( COUNTER_RSP_SIZE, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_COUNTER );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	memcpy(increased_bin_val, &packet.info[ATCA_RSP_DATA_IDX], sizeof(increased_bin_val));

	// build a counter command
	packet.param1 = COUNTER_MODE_READ;
	packet.param2 = 0x0000;
	status = atCounter( commandObj, &packet );
	TEST_ASSERT_EQUAL( COUNTER_RSP_SIZE, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_COUNTER );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(increased_bin_val, &packet.info[ATCA_RSP_DATA_IDX], 4, "Failed increment the counter");

	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );

}

void test_derivekey(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint8_t isLocked = false;
	uint16_t keyID = 0x0006;

	// This command has tested with SlotConfig[KeySlot6]: A666, KeyConfig[KeySlot6]: 1C00
	TEST_IGNORE_MESSAGE("Configuration data must be changed for this test to succeed.");

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	atsleep(iface);  // start from known state

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcau_is_locked( ATCA_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	//build a nonce command
	packet.param1 = NONCE_MODE_SEED_UPDATE;
	packet.param2 = 0x0000;
	memset( packet.info, 0x00, 32 );

	status = atNonce( commandObj, &packet );
	TEST_ASSERT_EQUAL_INT( NONCE_COUNT_SHORT, packet.txsize );
	TEST_ASSERT_EQUAL_INT( NONCE_RSP_SIZE_LONG, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_NONCE);

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	status = isATCAError( packet.info );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build a deriveKey command (Roll Key operation)
	packet.param1 = 0x0000;
	packet.param2 = keyID;

	status = atDeriveKey( commandObj, &packet, true );

	execution_time = atGetExecTime( commandObj, CMD_DERIVEKEY);
	TEST_ASSERT_EQUAL( DERIVE_KEY_RSP_SIZE, packet.rxsize );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	status = isATCAError( packet.info );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// check for derive key response if it's success or not
	TEST_ASSERT_EQUAL_INT8( ATCA_SUCCESS, packet.info[1] );

	// sleep or idle
	status = atidle(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );
}

void test_ecdh(void)
{
    ATCADevice device;
    ATCACommand commandObj;
    ATCAIface iface;
    ATCA_STATUS status;
    ATCAPacket packet;
    uint8_t is_locked = false;
    uint8_t sn[9];
    uint16_t private_key_id_bob = 0;
    uint8_t public_key_bob[ATCA_PUB_KEY_SIZE];
    uint8_t pms_bob[ATCA_KEY_SIZE];
    uint16_t pms_read_key_id_bob = 4;
    uint16_t private_key_id_alice = 2;
    uint8_t public_key_alice[ATCA_PUB_KEY_SIZE];
    uint8_t pms_alice[ATCA_KEY_SIZE];
    uint8_t num_in[NONCE_NUMIN_SIZE];
    uint8_t rand_out[RANDOM_NUM_SIZE];
    atca_temp_key_t temp_key;
    atca_nonce_in_out_t nonce_params;
    atca_gen_dig_in_out_t gen_dig_params;
    int i;
    
    status = atcau_is_locked( ATCA_ZONE_DATA, &is_locked );
    TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
    if ( !is_locked )
        TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed.");
    
    device = newATCADevice(gCfg);

    TEST_ASSERT_NOT_NULL( device );
    commandObj = atGetCommands( device );
    iface = atGetIFace(device);
    
    // Put device into known state in case it's still awake
    atsleep(iface);
    
    // Read SN
    packet.param1 = ATCA_ZONE_CONFIG | ATCA_ZONE_READWRITE_32;
    packet.param2 = 0;
    status = atRead(commandObj, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    status = test_command(commandObj, iface, CMD_WRITEMEM, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    TEST_ASSERT(packet.rxsize >= packet.info[ATCA_COUNT_IDX]);
    TEST_ASSERT_EQUAL(32+3, packet.info[ATCA_COUNT_IDX]);
    memcpy(&sn[0], &packet.info[ATCA_RSP_DATA_IDX], 4);
    memcpy(&sn[4], &packet.info[ATCA_RSP_DATA_IDX+8], 5);
    
    // Generate key pair for bob
    packet.param1 = GENKEY_MODE_PRIVATE;
    packet.param2 = private_key_id_bob;
    status = atGenKey(commandObj, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    status = test_command(commandObj, iface, CMD_GENKEY, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    TEST_ASSERT(packet.rxsize >= packet.info[ATCA_COUNT_IDX]);
    TEST_ASSERT_EQUAL(ATCA_PUB_KEY_SIZE+3, packet.info[ATCA_COUNT_IDX]);
    memcpy(public_key_bob, &packet.info[ATCA_RSP_DATA_IDX], ATCA_PUB_KEY_SIZE);
    
    // Generate key pair for alice
    packet.param1 = GENKEY_MODE_PRIVATE;
    packet.param2 = private_key_id_alice;
    status = atGenKey(commandObj, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    status = test_command(commandObj, iface, CMD_GENKEY, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    TEST_ASSERT(packet.rxsize >= packet.info[ATCA_COUNT_IDX]);
    TEST_ASSERT_EQUAL(ATCA_PUB_KEY_SIZE+3, packet.info[ATCA_COUNT_IDX]);
    memcpy(public_key_alice, &packet.info[ATCA_RSP_DATA_IDX], ATCA_PUB_KEY_SIZE);
    
    // Perform ECDH operation on bob's side
    packet.param1 = ECDH_PREFIX_MODE;
    packet.param2 = private_key_id_bob;
    memcpy(packet.info, public_key_alice, sizeof(public_key_alice));
    status = atECDH(commandObj, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    status = test_command(commandObj, iface, CMD_ECDH, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    TEST_ASSERT(packet.rxsize >= packet.info[ATCA_COUNT_IDX]);
    TEST_ASSERT_EQUAL(4, packet.info[ATCA_COUNT_IDX]);
    
    // Bob's PMS is written to the next slot, read that value
    
    //packet.param1 = ATCA_ZONE_DATA | ATCA_ZONE_READWRITE_32;
    //packet.param2 = 4 << 3;
    //memcpy(packet.info, g_slot4_key, 32);
    //status = atWrite(commandObj, &packet);
    //TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    //status = test_command(commandObj, iface, CMD_WRITEMEM, &packet);
    //TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Random nonce
    memset(&temp_key, 0, sizeof(temp_key));
    memset(num_in, 0, sizeof(num_in));
    nonce_params.mode = NONCE_MODE_SEED_UPDATE;
    nonce_params.num_in = num_in;
    nonce_params.rand_out = rand_out;
    nonce_params.temp_key = &temp_key;
    packet.param1 = nonce_params.mode;
    packet.param2 = 0;
    memcpy(packet.info, nonce_params.num_in, NONCE_NUMIN_SIZE);
    status = atNonce(commandObj, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    status = test_command(commandObj, iface, CMD_NONCE, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    TEST_ASSERT(packet.rxsize >= packet.info[ATCA_COUNT_IDX]);
    TEST_ASSERT_EQUAL(RANDOM_NUM_SIZE+3, packet.info[ATCA_COUNT_IDX]);
    memcpy(nonce_params.rand_out, &packet.info[ATCA_RSP_DATA_IDX], RANDOM_NUM_SIZE);
    
    // Perform host-side nonce calculation
    status = atcah_nonce(&nonce_params);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // GenDig with Bob's PMS Read Key
    gen_dig_params.zone = ATCA_ZONE_DATA;
    gen_dig_params.key_id = pms_read_key_id_bob;
    gen_dig_params.sn = sn;
    gen_dig_params.stored_value = g_slot4_key;
    gen_dig_params.temp_key = &temp_key;
    packet.param1 = gen_dig_params.zone;
    packet.param2 = gen_dig_params.key_id;
    status = atGenDig(commandObj, &packet, false);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    status = test_command(commandObj, iface, CMD_GENDIG, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    TEST_ASSERT(packet.rxsize >= packet.info[ATCA_COUNT_IDX]);
    
    // Perform host-side nonce calculation
    status = atcah_gen_dig(&gen_dig_params);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Encrypted read
    packet.param1 = ATCA_ZONE_DATA | ATCA_ZONE_READWRITE_32;
    packet.param2 = (private_key_id_bob + 1) << 3;
    status = atRead(commandObj, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    status = test_command(commandObj, iface, CMD_READMEM, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    TEST_ASSERT(packet.rxsize >= packet.info[ATCA_COUNT_IDX]);
    TEST_ASSERT_EQUAL(ATCA_KEY_SIZE+3, packet.info[ATCA_COUNT_IDX]);
    
    // Decrypt bob's PMS
    for (i = 0; i < ATCA_KEY_SIZE; i++)
        pms_bob[i] = packet.info[ATCA_RSP_DATA_IDX+i] ^ temp_key.value[i];
        
    // Perform ECDH operation on alice's side
    packet.param1 = ECDH_PREFIX_MODE;
    packet.param2 = private_key_id_alice;
    memcpy(packet.info, public_key_bob, sizeof(public_key_bob));
    status = atECDH(commandObj, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    status = test_command(commandObj, iface, CMD_ECDH, &packet);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    TEST_ASSERT(packet.rxsize >= packet.info[ATCA_COUNT_IDX]);
    TEST_ASSERT_EQUAL(ATCA_KEY_SIZE+3, packet.info[ATCA_COUNT_IDX]);
    
    // Alice's PMS is returned in the clear
    memcpy(pms_alice, &packet.info[ATCA_RSP_DATA_IDX], ATCA_KEY_SIZE);
    
    TEST_ASSERT_EQUAL_MEMORY(pms_bob, pms_alice, ATCA_KEY_SIZE);
    
    deleteATCADevice(&device);
}

void test_gendig(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint16_t keyID = 0x0004;
	uint8_t isLocked = false;

	status = atcau_is_locked( ATCA_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	//build a nonce command (pass through mode)
	packet.param1 = NONCE_MODE_PASSTHROUGH;
	packet.param2 = 0x0000;
	memset( packet.info, 0x55, 32 );  // a 32-byte nonce

	status = atNonce( commandObj, &packet );
	TEST_ASSERT_EQUAL_INT( NONCE_COUNT_LONG, packet.txsize );
	TEST_ASSERT_EQUAL_INT( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_NONCE);
	TEST_ASSERT_EQUAL( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	status = isATCAError( packet.info );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// check for nonce response for pass through mode
	TEST_ASSERT_EQUAL_INT8( ATCA_SUCCESS, packet.info[1] );

	// idle so tempkey will remain valid
	status = atidle(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	//build a gendig command
	packet.param1 = GENDIG_ZONE_DATA;
	packet.param2 = keyID;

	status = atGenDig( commandObj, &packet, false );
	TEST_ASSERT_EQUAL_INT( GENDIG_COUNT, packet.txsize );
	TEST_ASSERT_EQUAL_INT( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_GENDIG);

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL_INT8_MESSAGE( 0x00, packet.info[ATCA_RSP_DATA_IDX], "Failed GenDig test");

	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );
}

/** \brief this test assumes a specific configuration and locked config zone
 * test will generate a private key if data zone is unlocked and return a public key
 * test will generate a public key based on the private key if data zone is locked
 */

void test_genkey(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint16_t keyID = 0;
	uint8_t isLocked = false;

	status = atcau_is_locked( ATCA_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build a genkey command
	packet.param1 = 0x04;   // a random private key is generated and stored in slot keyID
	packet.param2 = keyID;
	status = atGenKey( commandObj, &packet );
	TEST_ASSERT_EQUAL( GENKEY_RSP_SIZE_LONG, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_GENKEY);

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL_MESSAGE( 67, packet.info[0], "Configuration zone must be locked for this test to succeed");

	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );

}

void test_hmac(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint16_t keyID = 0x01;
	uint8_t isLocked = false;

	status = atcau_is_locked( ATCA_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	atsleep(iface);

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	//-- Start Optionally run GenDig command
	packet.param1 = NONCE_MODE_PASSTHROUGH;
	packet.param2 = 0x0000;
	memset( packet.info, 0x55, 32 );

	status = atNonce( commandObj, &packet );
	TEST_ASSERT_EQUAL_INT( NONCE_COUNT_LONG, packet.txsize );
	TEST_ASSERT_EQUAL_INT( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_NONCE);
	TEST_ASSERT_EQUAL( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	atca_delay_ms(execution_time);

	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	status = isATCAError( packet.info );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	TEST_ASSERT_EQUAL_INT8( ATCA_SUCCESS, packet.info[1] );

	status = atidle(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	packet.param1 = GENDIG_ZONE_DATA;
	packet.param2 = keyID;

	status = atGenDig( commandObj, &packet, false );
	TEST_ASSERT_EQUAL_INT( GENDIG_COUNT, packet.txsize );
	TEST_ASSERT_EQUAL_INT( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_GENDIG);

	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	atca_delay_ms(execution_time);

	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atidle(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	//-- Option Test End

	// build a random command
	packet.param1 = RANDOM_SEED_UPDATE;
	packet.param2 = 0x0000;
	status = atRandom( commandObj, &packet );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL( ATCA_RSP_SIZE_32, packet.rxsize );
	execution_time = atGetExecTime( commandObj, CMD_RANDOM);

	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &packet.rxsize);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atidle(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	//build a nonce command
	packet.param1 = NONCE_MODE_SEED_UPDATE;
	packet.param2 = 0x0000;
	memset( packet.info, 0x55, 32 );  // a 32-byte nonce

	status = atNonce( commandObj, &packet );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	execution_time = atGetExecTime( commandObj, CMD_NONCE);

	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atidle(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build a HMAC command
	packet.param1 = ATCA_ZONE_DATA;
	packet.param2 = keyID;
	status = atHMAC( commandObj, &packet );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	execution_time = atGetExecTime( commandObj, CMD_HMAC );

	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// check if the response has the 32 bytes HMAC digest
	TEST_ASSERT_EQUAL_MESSAGE( ATCA_RSP_SIZE_32, packet.rxsize, "Failed HMAC test");

	atca_delay_ms(1);
	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );
}

void test_info(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;

	ATCA_STATUS status;
	ATCAPacket packet;

	uint16_t execution_time = 0;
	uint8_t revbytes[] = { 0x00, 0x00, 0x50, 0x00 }; // 508A rev bytes

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	atsleep(iface);

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build an info command
	packet.param1 = INFO_MODE_REVISION;   // these tests are for communication testing mainly,
	// but if testing the entire chip, would need to go through all the modes.
	// this tests version mode only
	status = atInfo( commandObj, &packet );
	TEST_ASSERT_EQUAL( ATCA_RSP_SIZE_4, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_INFO);

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// this was specified to be an ATECC508A device, so check for rev number
	TEST_ASSERT_EQUAL_INT8_ARRAY( revbytes, &packet.info[1], 4 );

	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );
}

void test_mac(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint16_t keyID = 0x01;
	uint8_t isLocked = false;

	status = atcau_is_locked( ATCA_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	atsleep(iface);  // start from known state

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build a mac command
	packet.param1 = MAC_MODE_CHALLENGE;
	packet.param2 = keyID;
	memset( packet.info, 0x55, 32 );  // a 32-byte challenge

	//memcpy(packet.info, challenge, sizeof(challenge));
	status = atMAC( commandObj, &packet );
	execution_time = atGetExecTime( commandObj, CMD_MAC);
	TEST_ASSERT_EQUAL( ATCA_RSP_SIZE_32, packet.rxsize );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &packet.rxsize);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	atca_delay_ms(1);
	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );
}

void test_nonce_passthrough(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	atsleep(iface);  // start from known state

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	//build a nonce command (pass through mode)
	packet.param1 = NONCE_MODE_PASSTHROUGH;
	packet.param2 = 0x0000;
	memset( packet.info, 0x55, 32 );  // a 32-byte nonce

	status = atNonce( commandObj, &packet );
	TEST_ASSERT_EQUAL_INT( NONCE_COUNT_LONG, packet.txsize );
	TEST_ASSERT_EQUAL_INT( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_NONCE);
	TEST_ASSERT_EQUAL( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	status = isATCAError( packet.info );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// check for nonce response for pass through mode
	TEST_ASSERT_EQUAL_INT8( ATCA_SUCCESS, packet.info[1] );

	// sleep or idle
	status = atidle(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );
}

void test_pause(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	atsleep(iface);  // start from known state

	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build a pause command
	packet.param1 = 0x00;
	packet.param2 = 0x0000;

	status = atPause( commandObj, &packet );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL_INT( PAUSE_COUNT, packet.txsize );
	TEST_ASSERT_EQUAL_INT( PAUSE_RSP_SIZE, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_NONCE);
	TEST_ASSERT_EQUAL( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &packet.rxsize);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL_INT8_MESSAGE( 0x00, packet.info[ATCA_RSP_DATA_IDX], "Failed Pause test");

	atca_delay_ms(1);
	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	TEST_ASSERT_NULL( device );
}

void test_privwrite(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint8_t isLocked = false;

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	status = atcau_is_locked( ATCA_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Config zone must be locked for this test to succeed.");

	status = atcau_is_locked( ATCA_ZONE_DATA, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be unlocked for this test to succeed.");

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build an PrivWrite command
	packet.param1 = 0x00;
	packet.param2 = 0x0000;
	memset(&packet.info[4], 0x55, 32);

	status = atPrivWrite( commandObj, &packet );
	TEST_ASSERT_EQUAL( PRIVWRITE_RSP_SIZE, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_PRIVWRITE);

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &packet.rxsize);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL_INT8_MESSAGE( 0x00, packet.info[ATCA_RSP_DATA_IDX], "Failed PrivWrite test");

	atca_delay_ms(1);
	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );
}

void test_random(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	atsleep(iface);  // start from known state

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build an random command
	packet.param1 = RANDOM_SEED_UPDATE;
	packet.param2 = 0x0000;
	status = atRandom( commandObj, &packet );
	execution_time = atGetExecTime( commandObj, CMD_RANDOM);
	TEST_ASSERT_EQUAL( ATCA_RSP_SIZE_32, packet.rxsize );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &packet.rxsize);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	atca_delay_ms(1);
	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );
}

void test_read(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	atsleep(iface);  // start from known state

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build read command
	packet.param1 = ATCA_ZONE_CONFIG;
	packet.param2 = 0x0400;

	status = atRead( commandObj, &packet );
	execution_time = atGetExecTime( commandObj, CMD_READMEM);

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &packet.rxsize);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_NOT_EQUAL( 0x0f, packet.info[1] );
	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );

}

void test_sha(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint8_t sha_success = 0x00;
	uint8_t sha_digest_out[ATCA_SHA_DIGEST_SIZE];

	device = newATCADevice( gCfg );

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	atsleep(iface);  // start from known state

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// initialize SHA calculation engine, initializes TempKey
	packet.param1 = SHA_MODE_SHA256_START;
	packet.param2 = 0x0000;

	status = atSHA( commandObj, &packet );
	execution_time = atGetExecTime( commandObj, CMD_SHA);
	TEST_ASSERT_EQUAL( SHA_RSP_SIZE_SHORT, packet.rxsize );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &packet.rxsize);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// check the response, if error then TempKey not initialized
	TEST_ASSERT_EQUAL_INT8( sha_success,  packet.info[1]);

	// Compute the SHA 256 digest if TempKey is loaded correctly
	packet.param1 = SHA_MODE_SHA256_END;
	packet.param2 = 0x0000;

	status = atSHA( commandObj, &packet );
	execution_time = atGetExecTime( commandObj, CMD_SHA);
	TEST_ASSERT_EQUAL( SHA_RSP_SIZE_LONG, packet.rxsize );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &packet.rxsize);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// Copy the response into digest_out
	memcpy(&sha_digest_out[0], &packet.info[1], ATCA_SHA_DIGEST_SIZE);

	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );
}

void test_sign(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint16_t keyID = 0;
	uint8_t isLocked = false;

	status = atcau_is_locked( ATCA_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);


	// set up message to sign
	//build a nonce command (pass through mode)
	packet.param1 = NONCE_MODE_PASSTHROUGH;
	packet.param2 = 0x0000;
	memset( packet.info, 0x55, 32 );  // a 32-byte nonce

	status = atNonce( commandObj, &packet );
	TEST_ASSERT_EQUAL_INT( NONCE_COUNT_LONG, packet.txsize );
	TEST_ASSERT_EQUAL_INT( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_NONCE);
	TEST_ASSERT_EQUAL( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	status = isATCAError( packet.info );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// check for nonce response for pass through mode
	TEST_ASSERT_EQUAL_INT8( ATCA_SUCCESS, packet.info[1] );

	// idle so tempkey will remain valid
	status = atidle(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build a sign command
	packet.param1 = SIGN_MODE_EXTERNAL;
	packet.param2 = keyID;
	status = atSign( commandObj, &packet );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	execution_time = atGetExecTime( commandObj, CMD_SIGN );

	// since sign is a relatively long execution time, do wake right before command send otherwise
	// chip could watchdog timeout before the last bytes of the response are received (depends
	// upon bus speed and watchdog timeout configuration.

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = isATCAError( packet.info );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );
}

void test_updateExtra(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint8_t isLocked = false;

	status = atcau_is_locked( ATCA_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	device = newATCADevice( gCfg );

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build a UpdateExtra command
	packet.param1 = UPDATE_MODE_SELECTOR;
	packet.param2 = 0x0000;

	status = atUpdateExtra( commandObj, &packet );
	execution_time = atGetExecTime( commandObj, CMD_UPDATEEXTRA);
	TEST_ASSERT_EQUAL( UPDATE_RSP_SIZE, packet.rxsize );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &packet.rxsize);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL_INT8_MESSAGE( 0x00, packet.info[1], "Failed to update the value");

	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );

}

void test_verify(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint16_t keyID = 0x00;
	uint8_t public_key[ATCA_PUB_KEY_SIZE];
	uint8_t signature[VERIFY_256_SIGNATURE_SIZE];
	uint8_t isLocked = false;

	status = atcau_is_locked( ATCA_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if (!isLocked)
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build a genkey command
	packet.param1 = 0x04;   // a random private key is generated and stored in slot keyID
	packet.param2 = keyID;
	status = atGenKey( commandObj, &packet );
	TEST_ASSERT_EQUAL( GENKEY_RSP_SIZE_LONG, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_GENKEY);

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	atidle(iface);

	// copy the data response into the public key
	memcpy(&public_key[0], &packet.info[ATCA_RSP_DATA_IDX], ATCA_PUB_KEY_SIZE);

	// build a random command
	packet.param1 = RANDOM_SEED_UPDATE;
	packet.param2 = 0x0000;
	status = atRandom( commandObj, &packet );
	execution_time = atGetExecTime( commandObj, CMD_RANDOM);
	TEST_ASSERT_EQUAL( ATCA_RSP_SIZE_32, packet.rxsize );

	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &packet.rxsize);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	atidle(iface);

	// build a nonce command (pass through mode)
	packet.param1 = NONCE_MODE_PASSTHROUGH;
	packet.param2 = 0x0000;

	memset( packet.info, 0x55, 32 );  // a 32-byte nonce

	status = atNonce( commandObj, &packet );
	TEST_ASSERT_EQUAL_INT( NONCE_COUNT_LONG, packet.txsize );
	TEST_ASSERT_EQUAL_INT( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_NONCE);
	TEST_ASSERT_EQUAL( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// idle so tempkey will remain valid
	status = atidle(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build a sign command
	packet.param1 = SIGN_MODE_EXTERNAL; //verify the signature
	packet.param2 = keyID;

	status = atSign( commandObj, &packet );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	execution_time = atGetExecTime( commandObj, CMD_SIGN );

	// since sign is a relatively long execution time, do wake right before command send otherwise
	// chip could watchdog timeout before the last bytes of the response are received (depends
	// upon bus speed and watchdog timeout configuration.

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// copy the data response into the signature
	memcpy(&signature[0], &packet.info[ATCA_RSP_DATA_IDX], ATCA_SIG_SIZE);

	// build an random command
	packet.param1 = RANDOM_SEED_UPDATE;
	packet.param2 = 0x0000;
	status = atRandom( commandObj, &packet );
	execution_time = atGetExecTime( commandObj, CMD_RANDOM);
	TEST_ASSERT_EQUAL( ATCA_RSP_SIZE_32, packet.rxsize );

	// send the random command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &packet.rxsize);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atidle(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build a nonce command (pass through mode)
	packet.param1 = NONCE_MODE_PASSTHROUGH;
	packet.param2 = 0x0000;
	memset( packet.info, 0x55, 32 );  // a 32-byte nonce

	status = atNonce( commandObj, &packet );
	TEST_ASSERT_EQUAL_INT( NONCE_COUNT_LONG, packet.txsize );
	TEST_ASSERT_EQUAL_INT( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_NONCE);
	TEST_ASSERT_EQUAL( NONCE_RSP_SIZE_SHORT, packet.rxsize );

	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atidle(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build a verify command
	packet.param1 = VERIFY_MODE_EXTERNAL; //verify the signature
	packet.param2 = VERIFY_KEY_P256;
	memcpy( &packet.info[0], signature, sizeof(signature));
	memcpy( &packet.info[64], public_key, sizeof(public_key));

	status = atVerify( commandObj, &packet );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	execution_time = atGetExecTime( commandObj, CMD_VERIFY );

	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL_INT8_MESSAGE( 0x00, packet.info[ATCA_RSP_DATA_IDX], "Failed Verify test");

	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );
}

void test_write(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;
	ATCA_STATUS status;
	ATCAPacket packet;
	uint16_t execution_time = 0;
	uint8_t cfgZoneLocked = false, dataZoneLocked = false;
	uint8_t zone;
	uint16_t addr = 0x00;

	device = newATCADevice(gCfg);

	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	status = atcau_is_locked( ATCA_ZONE_CONFIG, &cfgZoneLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !cfgZoneLocked )
		TEST_IGNORE_MESSAGE("Config zone must be locked for this test to succeed");

	// config zone locked
	status = atcau_is_locked( ATCA_ZONE_DATA, &dataZoneLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	if ( dataZoneLocked )
		TEST_IGNORE_MESSAGE("Data zone must not be locked for this test to succeed");

	zone = ATCA_ZONE_DATA | ATCA_ZONE_READWRITE_32;
	addr = 0x40; // slot 08 - slot 0E

	// build a write command to the data zone
	packet.param1 = zone;
	packet.param2 = addr;
	memset( packet.info, 0x00, sizeof(packet.info) );
    memcpy( packet.info, g_slot4_key, 32 );

	status = atWrite( commandObj, &packet, false );
	execution_time = atGetExecTime( commandObj, CMD_WRITEMEM);

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL_INT8_MESSAGE( 0x00, packet.info[ATCA_RSP_DATA_IDX], "Failed Write test");

	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );
}

void test_devRev(void)
{
	ATCADevice device;
	ATCACommand commandObj;
	ATCAIface iface;

	ATCA_STATUS status;
	ATCAPacket packet;

	uint16_t execution_time = 0;
	uint8_t revbytes[] = { 0x00, 0x02, 0x00, 0x08 };

	device = newATCADevice(gCfg);
	TEST_ASSERT_NOT_NULL( device );
	commandObj = atGetCommands( device );
	iface = atGetIFace(device);

	atsleep(iface);

	// wakeup
	status = atwake(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// build an info command
	packet.param1 = INFO_MODE_REVISION;   // must be zero
	status = atInfo( commandObj, &packet );
	TEST_ASSERT_EQUAL( ATCA_RSP_SIZE_4, packet.rxsize );

	execution_time = atGetExecTime( commandObj, CMD_INFO);

	// send the command
	status = atsend( iface, (uint8_t*)&packet, packet.txsize );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// delay the appropriate amount of time for command to execute
	atca_delay_ms(execution_time);

	// receive the response
	status = atreceive( iface, packet.info, &(packet.rxsize) );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// this was specified to be an ATSAH204A device, so check for rev number
	TEST_ASSERT_EQUAL_INT8_ARRAY( revbytes, &packet.info[1], 4 );

	// sleep or idle
	status = atsleep(iface);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	deleteATCADevice(&device);  // destructor will reinitialize given ptr to a NULL,
	// so it can be tested, ATCA objects are already pointers, so this
	// is passing a double indirect
	TEST_ASSERT_NULL( device );

}

int atca_sha204a_unit_tests(ATCADeviceType deviceType)
{
	RUN_TEST(test_wake_sleep);
	RUN_TEST(test_wake_idle);

	RUN_TEST(test_devRev); //only SHA204A
	RUN_TEST(test_random);
	RUN_TEST(test_checkmac);
	RUN_TEST(test_derivekey);
	RUN_TEST(test_gendig);
	RUN_TEST(test_hmac);
	RUN_TEST(test_mac);
	RUN_TEST(test_nonce_passthrough);
	RUN_TEST(test_pause);
	RUN_TEST(test_read);
	RUN_TEST(test_updateExtra);
	RUN_TEST(test_write);

	return ATCA_SUCCESS;
}

int atca_ecc108a_unit_tests(ATCADeviceType deviceType)
{
	RUN_TEST(test_wake_sleep);
	RUN_TEST(test_wake_idle);
	RUN_TEST(test_pause);
	RUN_TEST(test_info);
	RUN_TEST(test_random);
	RUN_TEST(test_sha);
	RUN_TEST(test_crcerror);
	RUN_TEST(test_nonce_passthrough);
	RUN_TEST(test_counter);
	RUN_TEST(test_privwrite);

	/* --- This line is to divide up lock condition --- */
	RUN_TEST(test_genkey);
	RUN_TEST(test_sign);

	RUN_TEST(test_verify);
	RUN_TEST(test_mac);
	RUN_TEST(test_checkmac);
	RUN_TEST(test_gendig);
	RUN_TEST(test_updateExtra);
	RUN_TEST(test_hmac);
	RUN_TEST(test_ecdh);
	RUN_TEST(test_write);
	RUN_TEST(test_derivekey);

	return ATCA_SUCCESS;
}

int atca_ecc508a_unit_tests(ATCADeviceType deviceType)
{
	// 508a tests are a superset of 108a tests
	// any 508a commands that are the same as 108a should be added to the ecc108a unit test runner
	atca_ecc108a_unit_tests( deviceType );

	// add only 508a RUN_TEST invocations here
	// RUN_TEST( test_ecdh );

	return ATCA_SUCCESS;
}

void RunAllCertDataTests(void);

int certdata_unit_tests(void)
{
	const char* argv[] = { "manual", "-v" };

	UnityMain(sizeof(argv) / sizeof(char*), argv, RunAllCertDataTests);

	return ATCA_SUCCESS;
}

void RunAllCertIOTests(void);
int certio_unit_tests(void)
{
	const char* argv[] = { "manual", "-v" };

	UnityMain(sizeof(argv) / sizeof(char*), argv, RunAllCertIOTests);

	return ATCA_SUCCESS;
}
