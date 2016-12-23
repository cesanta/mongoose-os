/**
 * \file
 * \brief Unity tests for the cryptoauthlib Basic API
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

#include "basic/atca_basic.h"
#include "test/atca_basic_tests.h"
#include "host/atca_host.h"
#include <stdlib.h>

extern ATCAIfaceCfg *gCfg;

uint8_t test_ecc_provisioning_configdata[ATCA_CONFIG_SIZE] = {
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xC0, 0x00, 0xAA, 0x00,
	0x8F, 0x20, 0xC4, 0x44,
	0x87, 0x20, 0xC4, 0x44,
	0x8F, 0x0F, 0x8F, 0x8F,
	0x9F, 0x8F, 0x82, 0x20,
	0xC4, 0x44, 0xC4, 0x44,
	0x0F, 0x0F, 0x0F, 0x0F,
	0x0F, 0x0F, 0x0F, 0x0F,
	0x0F, 0x0F, 0x0F, 0x0F,
	0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x55, 0x55,
	0xFF, 0xFF, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x33, 0x00, 0x5C, 0x00,
	0x13, 0x00, 0x5C, 0x00,
	0x3C, 0x00, 0x1C, 0x00,
	0x1C, 0x00, 0x33, 0x00,
	0x1C, 0x00, 0x1C, 0x00,
	0x3C, 0x00, 0x3C, 0x00,
	0x3C, 0x00, 0x3C, 0x00,
	0x1C, 0x00, 0x3C, 0x00
};

// Data to be written to each Address
uint8_t h12015_test_ecc_configdata[ATCA_CONFIG_SIZE] = {
	// block 0
	// Not Written: First 16 bytes are not written
	0x01, 0x23, 0x2F, 0x6C,
	0x00, 0x00, 0x50, 0x00,
	0xD9, 0x2C, 0xA5, 0x71,
	0xEE, 0xC0, 0x75, 0x00,
	// I2C, reserved, OtpMode, ChipMode
	0xC0, 0x00, 0x55, 0x00,
	// SlotConfig
	0x83, 0x20, 0x8F, 0x0F,
	0x8C, 0x20, 0xC1, 0x8F,
	0x84, 0x20, 0x84, 0x20,
	// block 1
	0x83, 0x20, 0x83, 0x20,
	0x0F, 0x0F, 0x0F, 0x0F,
	0x0F, 0x0F, 0x0F, 0x0F, // slot 10 0F 0F
	0x0F, 0x0F, 0x0F, 0x0F,
	0x0F, 0x0F, 0xAF, 0x0F,
	// Counters
	0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF,
	// block 2
	0x00, 0x00, 0x00, 0x00,
	// Last Key Use
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	// Not Written: UserExtra, Selector, LockValue, LockConfig (word offset = 5)
	0x00, 0x00, 0x55, 0x55, // Lockvalue - 0x55 (OTP,Data unlocked), LockConfig - 0x55 (config unlocked)
	// SlotLock[2], RFU[2]
	0xFF, 0xFF, 0x00, 0x00,
	// X.509 Format
	0x00, 0x00, 0x00, 0x00,
	// block 3
	// KeyConfig
	0x33, 0x00, 0x3C, 0x00,
	0x33, 0x00, 0x1C, 0x00,
	0x33, 0x00, 0x33, 0x00,
	0x33, 0x00, 0x33, 0x00,
	0x3C, 0x00, 0x3C, 0x00,
	0x3C, 0x00, 0x3C, 0x00,
	0x3C, 0x00, 0x3C, 0x00,
	0x3C, 0x00, 0x3C, 0x00
};

uint8_t test_ecc_configdata[ATCA_CONFIG_SIZE] = {
    0x01, 0x23, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00,  0x04, 0x05, 0x06, 0x07, 0xEE, 0x00, 0x01, 0x00,
    0xC0, 0x00, 0x55, 0x00, 0x8F, 0x2F, 0xC4, 0x44,  0x87, 0x20, 0xC4, 0xF4, 0x8F, 0x0F, 0x8F, 0x8F,
    0x9F, 0x8F, 0x83, 0x64, 0xC4, 0x44, 0xC4, 0x64,  0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF,  0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,  0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x33, 0x00, 0x1C, 0x00, 0x13, 0x00, 0x1C, 0x00,  0x3C, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0x33, 0x00,
    0x1C, 0x00, 0x1C, 0x00, 0x3C, 0x00, 0x30, 0x00,  0x3C, 0x00, 0x3C, 0x00, 0x32, 0x00, 0x30, 0x00
};

uint8_t sha204_default_config[ATCA_SHA_CONFIG_SIZE] = {
	// block 0
	// Not Written: First 16 bytes are not written
	0x01, 0x23, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xEE, 0x55, 0x00, 0x00,
	// I2C, TempOffset, OtpMode, ChipMode
	0xC8, 0x00, 0x55, 0x00,
	// SlotConfig
	0x8F, 0x80, 0x80, 0xA1,
	0x82, 0xE0, 0xA3, 0x60,
	0x84, 0x40, 0xA0, 0x85,
	// block 1
	0x86, 0x40, 0x87, 0x07,
	0x0F, 0x00, 0x89, 0xF2,
	0x8A, 0x7A, 0x0B, 0x8B,
	0x0C, 0x4C, 0xDD, 0x4D,
	0xC2, 0x42, 0xAF, 0x8F,
	// Use Flags
	0xFF, 0x00, 0xFF, 0x00,
	0xFF, 0x00, 0xFF, 0x00,
	0xFF, 0x00, 0xFF, 0x00,
	// block 2
	0xFF, 0x00, 0xFF, 0x00,
	// Last Key Use
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	// Not Written: UserExtra, Selector, LockData, LockConfig (word offset = 5)
	0x00, 0x00, 0x55, 0x55,
};

// from atca_unit_tests.c
extern const uint8_t g_slot4_key[];

/** \brief tests of the Basic Crypto API functionality
 */
void atca_basic_tests(ATCADeviceType deviceType)
{
	UnityBegin("atca_basic_tests.c");

	// do this set of tests regardless of which device type is requested
	// All devices should test helper functions
	atca_helper_tests();

	if (gCfg == NULL)
	{
		printf("No device selected.\r\n");
		return;
	}
	if (gCfg->devtype != deviceType)
	{
		printf("Wrong test for selected device type.\r\n");
		return;
	}

	switch ( deviceType ) {
	case ATSHA204A:
		atca_sha204a_basic_tests(deviceType);
		break;
	case ATECC108A:
		atca_ecc108a_basic_tests(deviceType);
		break;
	case ATECC508A:
		atca_ecc508a_basic_tests(deviceType);
		break;
	default:
		TEST_FAIL_MESSAGE("Unhandled device type");
		break;
	}

	UnityEnd();
}

/** \brief SHA204a tests using the Basic crypto API
 */
void atca_sha204a_basic_tests(ATCADeviceType deviceType)
{
	RUN_TEST(test_basic_init);
	RUN_TEST(test_basic_doubleinit);
	RUN_TEST(test_basic_info);
	RUN_TEST(test_basic_random);
	RUN_TEST(test_basic_challenge);
	//RUN_TEST(test_basic_write_config_zone);
	//RUN_TEST(test_basic_read_config_zone);
	RUN_TEST(test_basic_write_sha_config_zone);
	RUN_TEST(test_basic_read_sha_config_zone);
	RUN_TEST(test_basic_lock_config_zone);
	RUN_TEST(test_basic_write_otp_zone);
    RUN_TEST(test_basic_write_slot4_key);
	RUN_TEST(test_basic_lock_data_zone);
	RUN_TEST(test_basic_read_otp_zone);
	RUN_TEST(test_basic_write_data_zone);
    RUN_TEST(test_basic_write_enc);
	RUN_TEST(test_basic_read_data_zone);
	RUN_TEST(test_basic_gendig);
	RUN_TEST(test_basic_mac);
	RUN_TEST(test_basic_checkmac);
	RUN_TEST(test_basic_hmac);
	RUN_TEST(test_basic_hw_sha2_256_nist1);
	RUN_TEST(test_basic_hw_sha2_256_nist2);
	RUN_TEST(test_basic_hw_sha2_256_nist_short);
	//RUN_TEST(test_basic_hw_sha2_256_nist_long); // Takes a long time (~8 min)
	//RUN_TEST(test_basic_hw_sha2_256_nist_monte); // Takes even longer (haven't tried yet)
}

/** \brief ECC108a tests using the Basic crypto API
 */
void atca_ecc108a_basic_tests(ATCADeviceType deviceType)
{
	RUN_TEST(test_basic_version);
	RUN_TEST(test_basic_init );
	RUN_TEST(test_basic_doubleinit);
	RUN_TEST(test_basic_info );
	RUN_TEST(test_basic_random );
	RUN_TEST(test_basic_challenge );
	RUN_TEST(test_write_bytes_zone_config);
	RUN_TEST(test_basic_write_ecc_config_zone);
	RUN_TEST(test_basic_read_ecc_config_zone);
	RUN_TEST(test_basic_lock_config_zone);
	RUN_TEST(test_write_boundary_conditions);
	RUN_TEST(test_write_upper_slots);
	RUN_TEST(test_write_invalid_block);
	RUN_TEST(test_write_invalid_block_len);
	RUN_TEST(test_basic_priv_write_unencrypted);
	RUN_TEST(test_basic_write_slot4_key);
	RUN_TEST(test_basic_lock_data_zone);
	RUN_TEST(test_write_bytes_zone_slot8);
    RUN_TEST(test_basic_write_enc);
	RUN_TEST(test_basic_genkey);
	RUN_TEST(test_basic_sign);
    RUN_TEST(test_basic_sign_internal);
	RUN_TEST(test_read_sig);
	RUN_TEST(test_basic_lock_data_slot);
	RUN_TEST(test_create_key);
	RUN_TEST(test_get_pubkey);
	RUN_TEST(test_basic_verify_extern);
    RUN_TEST(test_basic_verify_stored);
	RUN_TEST(test_basic_verify_validate);
	RUN_TEST(test_basic_verify_invalidate);
	RUN_TEST(test_basic_gendig);
	RUN_TEST(test_basic_mac);
	RUN_TEST(test_basic_checkmac);
	RUN_TEST(test_basic_hmac);
	RUN_TEST(test_basic_priv_write_encrypted);
    RUN_TEST(test_basic_derivekey);
    RUN_TEST(test_basic_derivekey_mac);
	RUN_TEST(test_basic_sha);
	RUN_TEST(test_basic_sha_long);
	RUN_TEST(test_basic_sha_short);
	RUN_TEST(test_basic_hw_sha2_256_nist1);
	RUN_TEST(test_basic_hw_sha2_256_nist2);
	RUN_TEST(test_basic_hw_sha2_256_nist_short);
	//RUN_TEST(test_basic_hw_sha2_256_nist_long); // Takes a long time (~8 min)
	//RUN_TEST(test_basic_hw_sha2_256_nist_monte); // Takes even longer (haven't tried yet)
}

/** \brief ECC508a tests using the Basic crypto API
 */
void atca_ecc508a_basic_tests(ATCADeviceType deviceType)
{
	atca_ecc108a_basic_tests(deviceType);
	// add 508a specific tests here...for example,
	RUN_TEST(test_basic_ecdh);
}

extern ATCADevice _gDevice;

void test_basic_version(void)
{
	char verstr[20];
	ATCA_STATUS status = ATCA_GEN_FAIL;

	verstr[0] = '\0';
	status = atcab_version(verstr);

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL( 8, strlen(verstr) );
}

void test_basic_init(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_NOT_EQUAL( NULL, _gDevice );

	status = atcab_release();
	TEST_ASSERT_EQUAL( NULL, _gDevice );
}


void test_basic_doubleinit(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	// a double init should be benign
	status = atcab_init( gCfg );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_NOT_EQUAL( NULL, _gDevice );

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_NOT_EQUAL( NULL, _gDevice );

	status = atcab_release();
	TEST_ASSERT_EQUAL( NULL, _gDevice );
}

void test_basic_info(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t revision[4];

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_info( revision );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_random(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t randomnum[RANDOM_RSP_SIZE];

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_random( randomnum );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_challenge(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t random_number[32];

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_random( random_number );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_challenge( random_number );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}


void test_basic_write_zone(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// TODO - implement write zone basic api test

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_read_zone(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t data[32];
	uint8_t serial_prefix[] = { 0x01, 0x23 };
	uint8_t slot, block, offset;
	bool locked = false;

	slot = 0;
	block = 0;
	offset = 0;

	// initialize it with recognizable data
	memset( data, 0x77, sizeof(data) );

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// read config zone tests
	status = atcab_read_zone(ATCA_ZONE_CONFIG, slot, block, offset, data, sizeof(data));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL_UINT8_ARRAY(serial_prefix, data, 2 );

	// read data zone tests
	// data zone cannot be read unless the data zone is locked
	status = atcab_is_locked( LOCK_ZONE_DATA, &locked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

    status = atcab_read_zone(LOCK_ZONE_DATA, slot, block, offset, data, sizeof(data));
    TEST_ASSERT_EQUAL(locked ? ATCA_SUCCESS : ATCA_EXECUTION_ERROR, status);

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_write_ecc_config_zone(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = false;

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );

	if ((isLocked == false) && (status == ATCA_SUCCESS)) {
		// to test the extra basic function with the provisioning configuration(ECC)
		//status = atcab_write_ecc_config_zone(test_ecc_provisioning_configdata);

		status = atcab_write_ecc_config_zone(test_ecc_configdata);
		TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	}else {
		status = atcab_release();
		TEST_IGNORE_MESSAGE("Configuration zone must be unlocked for this test to succeed." );
	}

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_read_ecc_config_zone(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t config_data[ATCA_CONFIG_SIZE];

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_read_ecc_config_zone(config_data);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	TEST_ASSERT_EQUAL_UINT8_ARRAY(&config_data[16], &test_ecc_configdata[16], 70);
	// Ignore lock values, bytes 86 through 89
	TEST_ASSERT_EQUAL_UINT8_ARRAY(&config_data[90], &test_ecc_configdata[90], 38);
	
	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_write_config_zone(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = false;

	status = atcab_init( gCfg );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	if ( isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be unlocked for this test to succeed." );

	status = atcab_write_config_zone(sha204_default_config);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_read_config_zone(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = false;
	uint8_t config_data[ATCA_SHA_CONFIG_SIZE];

	status = atcab_init( gCfg );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_read_config_zone(config_data);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );

	if ((isLocked == false) && (status == ATCA_SUCCESS))
		TEST_ASSERT_EQUAL_UINT8_ARRAY(&config_data[16], &sha204_default_config[16], 68);

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_write_sha_config_zone(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = false;

	status = atcab_init( gCfg );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	if ( isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be unlocked for this test to succeed." );

	status = atcab_write_sha_config_zone(sha204_default_config);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_read_sha_config_zone(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = false;
	uint8_t config_data[ATCA_SHA_CONFIG_SIZE];

	status = atcab_init( gCfg );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_read_sha_config_zone(config_data);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );

	if ((isLocked == false) && (status == ATCA_SUCCESS))
		TEST_ASSERT_EQUAL_UINT8_ARRAY(&config_data[16], &sha204_default_config[16], 68);

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_lock_config_zone(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = false;
	uint8_t lock_success = 0;
	uint8_t lock_resp = 0;

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	if ( isLocked != 0 )
		TEST_IGNORE_MESSAGE("Configuration zone must be unlocked for this test to succeed." );

	status = atcab_lock_config_zone(&lock_resp);
	TEST_ASSERT_EQUAL( lock_resp, lock_success );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_lock_data_zone(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = false;
	uint8_t lock_resp = 0;

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	if ( isLocked != 0 )
		TEST_IGNORE_MESSAGE("Data zone must be unlocked for this test to succeed." );

	status = atcab_lock_data_zone(&lock_resp);

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_lock_data_slot(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = false;
	uint8_t lock_success = 0;
	uint8_t lock_resp = 0;

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	if ( isLocked != 1 )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed." );

	//reset lock
	isLocked = false;

	//check the lock status of the slot
	status = atcab_is_slot_locked(13, &isLocked);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	if ( isLocked != 0 )
		TEST_IGNORE_MESSAGE("Slot locked already." );

	//try to lock slot 10
	status = atcab_lock_data_slot(13, &lock_resp);
	TEST_ASSERT_EQUAL( lock_resp, lock_success );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}
/* write zone tests */

void test_write_boundary_conditions(void)
{
	bool isLocked = 0;
	uint8_t write_data[ATCA_BLOCK_SIZE];

	// test boundary conditions, len = 0, len = 416, fixed slot = 0x08
	ATCA_STATUS status = ATCA_SUCCESS;

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed." );

	isLocked = false;
	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
	if ( isLocked )
		TEST_IGNORE_MESSAGE( "Data zone must NOT be locked for this test to succeed.");

	memset(write_data, 0xA5, ATCA_BLOCK_SIZE);
	// test slot = 0, write block size
	status = atcab_write_zone(ATCA_ZONE_DATA, 0, 0, 0, write_data, 0);
	TEST_ASSERT_EQUAL( ATCA_BAD_PARAM, status);

	status = atcab_write_zone(ATCA_ZONE_DATA, 0, 0, 0, write_data, ATCA_BLOCK_SIZE);
	TEST_ASSERT_NOT_EQUAL( ATCA_SUCCESS, status);  // should fail because config has slot 0 as a key

	status = atcab_write_zone(ATCA_ZONE_DATA, 0, 0, 0, write_data, ATCA_BLOCK_SIZE + 1);
	TEST_ASSERT_EQUAL( ATCA_BAD_PARAM, status);

	// less than a block size (less than 32-bytes)
	status = atcab_write_zone(ATCA_ZONE_DATA, 10, 2, 0, write_data,  31);
	TEST_ASSERT_NOT_EQUAL( ATCA_SUCCESS, status );
	// less than a block size (less than 4-bytes)
	status = atcab_write_zone(ATCA_ZONE_DATA, 10, 2, 0, write_data, 3);
	TEST_ASSERT_NOT_EQUAL( ATCA_SUCCESS, status );
	// equal to block(4-bytes) size, this is not permitted bcos 4-byte writes are not allowed when zone unlocked
	status = atcab_write_zone(ATCA_ZONE_DATA, 10, 2, 0, write_data, ATCA_WORD_SIZE);
	TEST_ASSERT_NOT_EQUAL( ATCA_SUCCESS, status );
	// equal to block(32-bytes) size,
	status = atcab_write_zone(ATCA_ZONE_DATA, 10, 1, 0, write_data, ATCA_BLOCK_SIZE);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status); //pass for both locked and unlocked case

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_write_upper_slots(void)
{
	bool isLocked = 0;
	int slot;
	uint8_t data[32];

	// test writing slot 9-15

	ATCA_STATUS status = ATCA_SUCCESS;

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	isLocked = false;
	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be unlocked for this test to succeed.");

	// W25 config has slots 8 and 9 as secret
	for ( slot = 10; slot <= 15; slot++ ) {

		memset(data, (uint8_t)slot, sizeof(data));
		status = atcab_write_zone( ATCA_ZONE_DATA, (uint8_t)slot, 0, 0, data, sizeof(data));
		TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	}

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}


void test_write_invalid_block(void)
{
	bool isLocked = 0;
	uint8_t write_data[ATCA_BLOCK_SIZE];
	// invalid block

	ATCA_STATUS status = ATCA_SUCCESS;

	status = atcab_init(gCfg);

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	isLocked = false;
	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
	if ( isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be unlocked for this test to succeed.");

	// valid slot and last offset, invalid block
	status = atcab_write_zone(ATCA_ZONE_DATA, 8, 4, 7, write_data, ATCA_WORD_SIZE);
	TEST_ASSERT_NOT_EQUAL( ATCA_SUCCESS, status);
	// invalid slot, valid block and offset
	status = atcab_write_zone(ATCA_ZONE_DATA, 16, 0, 0, write_data, ATCA_BLOCK_SIZE);
	TEST_ASSERT_NOT_EQUAL( ATCA_SUCCESS, status);
	// valid slot, invalid block and offset
	status = atcab_write_zone(ATCA_ZONE_DATA, 10, 4, 8, write_data, ATCA_WORD_SIZE);
	TEST_ASSERT_NOT_EQUAL( ATCA_SUCCESS, status);
	// valid block(4-bytes size) and slot, invalid offset
	status = atcab_write_zone(ATCA_ZONE_DATA, 10, 2, 2, write_data, ATCA_WORD_SIZE);
	TEST_ASSERT_NOT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_write_invalid_block_len(void)
{
	bool isLocked = 0;
	uint8_t write_data[ATCA_BLOCK_SIZE];
	uint8_t write_data1[ATCA_BLOCK_SIZE];
	uint8_t write_data2[ATCA_BLOCK_SIZE];
	// invalid block and write word len combination

	ATCA_STATUS status = ATCA_SUCCESS;

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	memset(write_data, 0xAB, ATCA_BLOCK_SIZE);
	memset(write_data1, 0xAA, ATCA_BLOCK_SIZE);
	memset(write_data2, 0xBB, ATCA_BLOCK_SIZE);

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );

	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	isLocked = false;
	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );

	if ( isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be unlocked for this test to succeed.");

	//writing 4bytes into 32 byte slot size
	status = atcab_write_zone(ATCA_ZONE_DATA, 10, 0, 0, write_data1, ATCA_WORD_SIZE);
	// not success for unlocked case(4 byte write command not allowed for data zone unlocked case only 32 byte write), success for locked case
	TEST_ASSERT_NOT_EQUAL( ATCA_SUCCESS, status);
	//writing 32 bytes into 4bytes block => 32-byte Write command writes only 4 bytes and ignores the rest
	status = atcab_write_zone(ATCA_ZONE_DATA, 10, 2, 1, write_data, ATCA_BLOCK_SIZE);
	//pass for both locked and unlocked case
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_write_bytes_zone_config(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = 0;

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	if ( isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be unlocked for this test to succeed." );

	status = atcab_write_bytes_zone(ATCA_ZONE_CONFIG, 0, 96, &test_ecc_configdata[96], sizeof(test_ecc_configdata) - 96);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_write_otp_zone(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = 0;
	uint8_t write_data[ATCA_BLOCK_SIZE * 2];

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed." );

	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed." );

	memset(write_data, 0xA5, sizeof(write_data));

	status = atcab_write_bytes_zone(ATCA_ZONE_OTP, 0, 0x00, write_data, sizeof(write_data));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_read_otp_zone(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = 0;
	uint8_t read_data[ATCA_BLOCK_SIZE * 2];

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed." );

	status = atcab_read_bytes_zone(ATCA_ZONE_OTP, 0, 0x00, read_data, sizeof(read_data));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_write_slot4_key(void)
{
    ATCA_STATUS status = ATCA_SUCCESS;
    bool isLocked = 0;
    
    status = atcab_init( gCfg );
    TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
    
    status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
    if ( !isLocked )
        TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed." );

    status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
    if ( isLocked )
        TEST_IGNORE_MESSAGE("Data zone must be unlocked for this test to succeed." );
        
    status = atcab_write_zone(ATCA_ZONE_DATA, 4, 0, 0, g_slot4_key, 32);
    TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_write_data_zone(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = 0;
	uint8_t write_data[ATCA_BLOCK_SIZE*2];
	uint8_t read_data[sizeof(write_data)];

	status = atcab_init( gCfg );
    TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
    
	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed." );
        
    status = atcab_random(&write_data[0]);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    status = atcab_random(&write_data[ATCA_BLOCK_SIZE]);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
	status = atcab_write_bytes_zone(ATCA_ZONE_DATA, 10, 4, write_data, sizeof(write_data));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);
    
    status = atcab_read_bytes_zone(ATCA_ZONE_DATA, 10, 4, read_data, sizeof(read_data));
    TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

    TEST_ASSERT_EQUAL_MEMORY(write_data, read_data, sizeof(write_data));
    
	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_read_data_zone(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = 0;
	uint8_t read_data[ATCA_BLOCK_SIZE];

	status = atcab_init( gCfg );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
    
	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed." );

	status = atcab_read_bytes_zone(ATCA_ZONE_DATA, 10, 4, read_data, sizeof(read_data));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_write_bytes_zone_slot8(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = 0;
	uint8_t write_data[64];

	TEST_IGNORE_MESSAGE("Slot 8 is secret with W25 config, can't test this easily.");

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
    
	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed." );

	memset(write_data, 0xA5, sizeof(write_data));

	status = atcab_write_bytes_zone(ATCA_ZONE_DATA, 8, 72, write_data, sizeof(write_data));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_write_enc(void)
{
    ATCA_STATUS status = ATCA_SUCCESS;
    uint16_t key_id = 8;
    uint8_t block = 5;
    const uint8_t data[ATCA_KEY_SIZE] = {
        0xf7, 0x20, 0x19, 0xb8, 0xdc, 0xeb, 0x6e, 0x2b, 0xe5, 0x44, 0x7d, 0x7f, 0xdc, 0x38, 0x7f, 0xc8,
        0xf1, 0x1d, 0x99, 0x17, 0x71, 0x25, 0xf2, 0x4d, 0x75, 0x2a, 0xb0, 0x58, 0xdf, 0x72, 0xaa, 0x4c};
    uint8_t read_data[ATCA_KEY_SIZE];
    
    status = atcab_init( gCfg );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
    
    status = atcab_write_enc(key_id, block, data, g_slot4_key, 4);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
    
    status = atcab_read_enc(key_id, block, read_data, g_slot4_key, 4);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
    
    TEST_ASSERT_EQUAL_MEMORY(data, read_data, sizeof(data));
    
    status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}    

void test_basic_genkey(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t public_key[64];
	bool isLocked;
	uint8_t frag[4] = { 0x44, 0x44, 0x44, 0x44 };

	memset(public_key, 0x44, 64 );  // mark the key with bogus data

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	status = atcab_genkey(0, public_key);
	TEST_ASSERT_EQUAL_MESSAGE( ATCA_SUCCESS, status, "Key generation failed" );

	// spot check public key for bogus data, there should be none
	// pub key is random so can't check the full content anyway.
	TEST_ASSERT_NOT_EQUAL( 0, memcmp( public_key, frag, 4 ) );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_sign(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t msg[ATCA_SHA_DIGEST_SIZE];
    uint8_t public_key[ATCA_PUB_KEY_SIZE];
	uint8_t signature[ATCA_SIG_SIZE];
    uint16_t private_key_id = 0;
	bool is_locked = false;
    bool is_verified = false;

	status = atcab_init(gCfg);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &is_locked );
	if ( !is_locked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

    // Generate random message
	status = atcab_random(msg);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Generate key pair
    status = atcab_genkey(private_key_id, public_key);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Sign message
	status = atcab_sign(private_key_id, msg, signature);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Verify signature
    status = atcab_verify_extern(msg, signature, public_key, &is_verified);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    TEST_ASSERT_EQUAL(true, is_verified);

	status = atcab_release();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_basic_sign_internal(void)
{
	uint8_t  internal_key_id = 4; // Which slot to sign digest of (via GenDig)
    uint16_t private_key_id  = 0; // Slot with private key to do the signing
    
    ATCA_STATUS status = ATCA_SUCCESS;
	bool is_locked = false;
    uint8_t config[128];
    uint8_t sn[9];
    uint8_t public_key[ATCA_PUB_KEY_SIZE];
    uint8_t num_in[NONCE_NUMIN_SIZE];
    atca_temp_key_t temp_key;
    atca_nonce_in_out_t nonce_params;
    uint8_t rand_out[RANDOM_NUM_SIZE];
    atca_gen_dig_in_out_t gen_dig_params;
	uint8_t signature[ATCA_SIG_SIZE];
	atca_sign_internal_in_out_t sign_params;
	uint8_t msg[ATCA_SHA_DIGEST_SIZE];
    bool is_verified = false;

	status = atcab_init(gCfg);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcab_is_locked(LOCK_ZONE_CONFIG, &is_locked);
	if  (!is_locked)
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");
        
    // Read the config zone
    status = atcab_read_config_zone(config);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    memcpy(&sn[0], &config[0], 4);
    memcpy(&sn[4], &config[8], 5);
    
    // Generate key pair and get public key
    status = atcab_genkey(private_key_id, public_key);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Start with random nonce
    memset(&temp_key, 0, sizeof(temp_key));
    memset(&num_in, 0, sizeof(num_in));
    nonce_params.mode = NONCE_MODE_SEED_UPDATE;
    nonce_params.num_in = num_in;
    nonce_params.rand_out = rand_out;
    nonce_params.temp_key = &temp_key;
	status = atcab_nonce_rand(nonce_params.num_in, rand_out);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    status = atcah_nonce(&nonce_params);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Use GenDig to create an initial digest across the internal key to be signed
    gen_dig_params.zone = ATCA_ZONE_DATA;
    gen_dig_params.key_id = internal_key_id;
    gen_dig_params.stored_value = g_slot4_key;
    gen_dig_params.sn = sn;
    gen_dig_params.temp_key = &temp_key;
    status = atcab_gendig(gen_dig_params.zone, gen_dig_params.key_id, NULL, 0);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    status = atcah_gen_dig(&gen_dig_params);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Perform a internal data sign
	memset(&sign_params, 0, sizeof(sign_params));
	sign_params.mode = SIGN_MODE_INTERNAL | SIGN_MODE_INCLUDE_SN;
	sign_params.key_id = private_key_id;
	sign_params.sn = sn;
	sign_params.temp_key = &temp_key;
	sign_params.digest = msg;
    status = atcab_sign_internal(sign_params.key_id, sign_params.for_invalidate, sign_params.mode & SIGN_MODE_INCLUDE_SN, signature);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Build the message used by Sign(Internal)
	status = atcah_config_to_sign_internal(gCfg->devtype, &sign_params, config);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	status = atcah_sign_internal_msg(gCfg->devtype, &sign_params);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Verify the signature
    status = atcab_verify_extern(sign_params.digest, signature, public_key, &is_verified);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    TEST_ASSERT_EQUAL(true, is_verified);

	status = atcab_release();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_read_sig(void)
{
	TEST_IGNORE_MESSAGE("Pending");
}

void test_create_key(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t public_key[64];
	bool isLocked;
	uint8_t frag[4] = { 0x44, 0x44, 0x44, 0x44 };

	memset(public_key, 0x44, 64 );  // mark the key with bogus data

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	status = atcab_genkey(0, public_key);
	TEST_ASSERT_EQUAL_MESSAGE( ATCA_SUCCESS, status, "Key generation failed" );

	// spot check public key for bogus data, there should be none
	// pub key is random so can't check the full content anyway.
	TEST_ASSERT_NOT_EQUAL( 0, memcmp( public_key, frag, 4 ) );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_get_pubkey(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t public_key[64];
	bool isLocked;
	uint8_t frag[4] = { 0x44, 0x44, 0x44, 0x44 };

	memset(public_key, 0x44, 64 );  // mark the key with bogus data

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	status = atcab_get_pubkey(0, public_key);
	TEST_ASSERT_EQUAL_MESSAGE( ATCA_SUCCESS, status, "Key generation failed" );

	// spot check public key for bogus data, there should be none
	// pub key is random so can't check the full content anyway.
	TEST_ASSERT_NOT_EQUAL( 0, memcmp( public_key, frag, 4 ) );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_priv_write_unencrypted(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	static const uint8_t private_key[36] = {
		0x00, 0x00, 0x00, 0x00,
		0x87, 0x8F, 0x0A, 0xB6,0xA5,  0x26,	 0xD7,	0x11,  0x1C,  0x26,	 0xE6,	0x17,  0x08,  0x10,	 0x79,	0x6E,
		0x7B, 0x33, 0x00, 0x7F,0x83,  0x2B,	 0x8D,	0x64,  0x46,  0x7E,	 0xD6,	0xF8,  0x70,  0x53,	 0x7A,	0x19
	};
	static const uint8_t public_key_ref[64] = {
		0x8F, 0x8D, 0x18, 0x2B, 0xD8, 0x19, 0x04, 0x85, 0x82, 0xA9, 0x92, 0x7E, 0xA0, 0xC5, 0x6D, 0xEF,
		0xB4, 0x15, 0x95, 0x48, 0xE1, 0x1C, 0xA5, 0xF7, 0xAB, 0xAC, 0x45, 0xBB, 0xCE, 0x76, 0x81, 0x5B,
		0xE5, 0xC6, 0x4F, 0xCD, 0x2F, 0xD1, 0x26, 0x98, 0x54, 0x4D, 0xE0, 0x37, 0x95, 0x17, 0x26, 0x66,
		0x60, 0x73, 0x04, 0x61, 0x19, 0xAD, 0x5E, 0x11, 0xA9, 0x0A, 0xA4, 0x97, 0x73, 0xAE, 0xAC, 0x86
	};
	uint8_t public_key[64];
	bool is_locked = false;

	status = atcab_init(gCfg);

	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcab_is_locked(LOCK_ZONE_DATA, &is_locked);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	if (is_locked)
		TEST_IGNORE_MESSAGE("Data zone must be unlocked for this test to succeed.");

	status = atcab_priv_write(0, private_key, 0, NULL);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcab_get_pubkey(0, public_key);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	TEST_ASSERT_EQUAL_MEMORY(public_key_ref, public_key, sizeof(public_key_ref));

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

// This test can be worked using only a root module configuration of provisioning project without pointing authkey
void test_basic_priv_write_encrypted(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t write_key_id = 0x04;
	uint8_t public_key[64];
	bool is_locked = false;
	static const uint8_t private_key[36] = {
		0x00, 0x00, 0x00, 0x00,
		0x87, 0x8F, 0x0A, 0xB6,0xA5,  0x26,	 0xD7,	0x11,  0x1C,  0x26, 0xE6, 0x17, 0x08, 0x10, 0x79, 0x6E,
		0x7B, 0x33, 0x00, 0x7F,0x83,  0x2B,	 0x8D,	0x64,  0x46,  0x7E, 0xD6, 0xF8, 0x70, 0x53, 0x7A, 0x19
	};
	static const uint8_t public_key_ref[64] = {
		0x8F, 0x8D, 0x18, 0x2B, 0xD8, 0x19, 0x04, 0x85, 0x82, 0xA9, 0x92, 0x7E, 0xA0, 0xC5, 0x6D, 0xEF,
		0xB4, 0x15, 0x95, 0x48, 0xE1, 0x1C, 0xA5, 0xF7, 0xAB, 0xAC, 0x45, 0xBB, 0xCE, 0x76, 0x81, 0x5B,
		0xE5, 0xC6, 0x4F, 0xCD, 0x2F, 0xD1, 0x26, 0x98, 0x54, 0x4D, 0xE0, 0x37, 0x95, 0x17, 0x26, 0x66,
		0x60, 0x73, 0x04, 0x61, 0x19, 0xAD, 0x5E, 0x11, 0xA9, 0x0A, 0xA4, 0x97, 0x73, 0xAE, 0xAC, 0x86
	};

	status = atcab_init(gCfg);

	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcab_is_locked(LOCK_ZONE_DATA, &is_locked);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	if (!is_locked)
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed.");

	status = atcab_priv_write(0x07, private_key, write_key_id, g_slot4_key);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcab_get_pubkey(0x07, public_key);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	TEST_ASSERT_EQUAL_MEMORY(public_key_ref, public_key, sizeof(public_key_ref));

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_verify_extern(void)
{
	ATCA_STATUS status;
	bool verified = false;
	bool isLocked;
	uint8_t message[ATCA_KEY_SIZE];
	uint8_t signature[ATCA_SIG_SIZE];
	uint8_t pubkey[ATCA_PUB_KEY_SIZE];

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed." );

	status = atcab_get_pubkey(0, pubkey);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	status = atcab_random(message);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	// do sign and verify all on the same test device - normally you wouldn't do this, but it's handy
	// to test out verify
	status = atcab_sign(0, message, signature);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	status = atcab_verify_extern(message, signature, pubkey, &verified);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);
	TEST_ASSERT( verified );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_verify_stored(void)
{
    ATCA_STATUS status;
    bool is_verified = false;
    bool is_locked = false;
    const uint16_t private_key_id = 2;
    const uint16_t public_key_id = 11;
    uint8_t message[ATCA_KEY_SIZE];
    uint8_t signature[ATCA_SIG_SIZE];
    uint8_t public_key[72];
    
    status = atcab_init(gCfg);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Make sure data zone is locked
    status = atcab_is_locked(LOCK_ZONE_DATA, &is_locked);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    if (!is_locked)
        TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed." );
    
    // Generate new key pair
    status = atcab_genkey(private_key_id, public_key);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Reformat public key into padded format
    memmove(&public_key[40], &public_key[32], 32); // Move Y to padded position
    memset(&public_key[36], 0, 4);                 // Add Y padding bytes
    memmove(&public_key[4], &public_key[0], 32);   // Move X to padded position
    memset(&public_key[0], 0, 4);                  // Add X padding bytes
    
    // Write public key to slot
    status = atcab_write_bytes_zone(ATCA_ZONE_DATA, public_key_id, 0, public_key, 72);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Generate random message to be signed
    status = atcab_random(message);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Sign the message
    status = atcab_sign(private_key_id, message, signature);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Verify the signature
    is_verified = false;
    status = atcab_verify_stored(message, signature, public_key_id, &is_verified);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    TEST_ASSERT( is_verified );
    
    // Modify message to create failure
    message[0]++;
    
    // Verify with bad message, should fail
    is_verified = false;
    status = atcab_verify_stored(message, signature, public_key_id, &is_verified);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    TEST_ASSERT(!is_verified);

    status = atcab_release();
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_basic_verify_validate(void)
{
	const uint16_t public_key_id = 14;
	const uint16_t private_key_id = 0;
	const uint16_t validation_private_key_id = 2;

	ATCA_STATUS status = ATCA_SUCCESS;
	bool is_locked = false;
	uint8_t config[128];
	uint8_t sn[9];
	uint8_t validation_public_key[ATCA_PUB_KEY_SIZE];
	uint16_t validation_public_key_id = 0;
	uint8_t public_key[ATCA_PUB_KEY_SIZE];
	uint8_t test_msg[32];
	uint8_t test_signature[ATCA_SIG_SIZE];
	bool is_verified = false;
	uint8_t valid_buf[4];
	uint8_t nonce[32];
	uint8_t rand_out[ATCA_KEY_SIZE];
	atca_temp_key_t temp_key;
	atca_nonce_in_out_t nonce_params;
	uint8_t gen_key_other_data[3];
	atca_gen_key_in_out_t gen_key_params;
	uint8_t verify_other_data[19];
    uint8_t validation_msg[55];
	uint8_t validation_digest[32];
	atca_sign_internal_in_out_t sign_params;
	uint8_t validation_signature[ATCA_SIG_SIZE];

	status = atcab_init(gCfg);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Make sure data zone is locked
	status = atcab_is_locked(LOCK_ZONE_DATA, &is_locked);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	if (!is_locked)
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed.");

	// Read config zone
	status = atcab_read_config_zone(config);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	memcpy(&sn[0], &config[0], 4);
	memcpy(&sn[4], &config[8], 5);

	// SETUP: Initialize device data to support a validated public key test

	// Generate key pair for validation
	// Typically, the validation private key wouldn't be on the same device as its public key
	status = atcab_genkey(validation_private_key_id, validation_public_key);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Write validation public key
	// Typically, this would be locked into the device during initial programming.
	validation_public_key_id = config[20 + public_key_id * 2] & 0x0F; // Validation public key ID is the validated public key's ReadKey
	status = atcab_write_pubkey(validation_public_key_id, validation_public_key);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// RUN: Run through the validated public key update process
	// This process has two parties. First is the device with a validated public key slot.
	// Whenever that slot gets updated, a Validation Authority (which has the validation private
	// key) is required to validate the new public key before it can be used.

	// Validation Authority: Generate new key pair.
	status = atcab_genkey(private_key_id, public_key);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Create and sign some data for testing the new key pair
	status = atcab_random(test_msg);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	status = atcab_sign(private_key_id, test_msg, test_signature);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	status = atcab_verify_extern(test_msg, test_signature, public_key, &is_verified);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	TEST_ASSERT_EQUAL(true, is_verified);

	// Validated Device: Write the new public key to the validate public key slot
	status = atcab_write_pubkey(public_key_id, public_key);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Make sure the previous write invalidated the public key
	status = atcab_read_zone(ATCA_ZONE_DATA, public_key_id, 0, 0, valid_buf, 4);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	TEST_ASSERT_EQUAL(0xA, valid_buf[0] >> 4); // Validation status is the 4 upper-most bits in the slot

	// Additionally, check to make sure a verify(stored) command with it fails.
	status = atcab_verify_stored(test_msg, test_signature, public_key_id, &is_verified);
	TEST_ASSERT_EQUAL(ATCA_EXECUTION_ERROR, status);

	// Validated Device: Validation process needs to start with a nonce (random is most secure)
	// Not using random due to limitations with simulating the validated device and validation
	// authority on the same device.
	memset(nonce, 0, sizeof(nonce));
	memset(&temp_key, 0, sizeof(temp_key));
	nonce_params.mode = NONCE_MODE_PASSTHROUGH;
	nonce_params.num_in = nonce;
	nonce_params.rand_out = rand_out;
	nonce_params.temp_key = &temp_key;
	status = atcab_nonce(nonce_params.num_in);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Validation Authority: Calculate same nonce
	status = atcah_nonce(&nonce_params);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Validated Authority: GenKey format is then used to combine the nonce with the new public key to be validated
	memset(gen_key_other_data, 0, sizeof(gen_key_other_data));
	gen_key_params.mode = GENKEY_MODE_PUBKEY_DIGEST;
	gen_key_params.key_id = public_key_id;
	gen_key_params.public_key = public_key;
	gen_key_params.public_key_size = sizeof(public_key);
	gen_key_params.other_data = gen_key_other_data;
	gen_key_params.sn = sn;
	gen_key_params.temp_key = &temp_key;
	status = atcah_gen_key_msg(&gen_key_params);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Validation Authority: Build validation message which uses the Sign(Internal) format
	memset(&sign_params, 0, sizeof(sign_params));
	sign_params.sn = sn;
	sign_params.verify_other_data = verify_other_data;
    sign_params.message = validation_msg;
	sign_params.digest = validation_digest;
	sign_params.temp_key = &temp_key;
	status = atcah_sign_internal_msg(gCfg->devtype, &sign_params);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Validation Authority: Sign the validation message
	status = atcab_sign(validation_private_key_id, validation_digest, validation_signature);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// The previous sign cleared TempKey, so we have to reset it. This is because the unit test is trying
	// to perform the actions of the Validation Authority and the Validate Device on the same device.
	// This wouldn't be needed normally.
	status = atcab_nonce(nonce_params.num_in);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Validated Device: Combine the public key with the nonce
	status = atcab_genkey_base(gen_key_params.mode, gen_key_params.key_id, gen_key_params.other_data, NULL);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Validated Device: Use Verify(Validate) command to validate the new public key
	status = atcab_verify_validate(public_key_id, validation_signature, verify_other_data, &is_verified);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	TEST_ASSERT_EQUAL(true, is_verified);

	// Make sure public key is validated now
	status = atcab_read_zone(ATCA_ZONE_DATA, public_key_id, 0, 0, valid_buf, 4);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	TEST_ASSERT_EQUAL(0x5, valid_buf[0] >> 4); // Validation status is the 4 upper-most bits in the slot

	// Additionally, check to make sure a verify(stored) command works now.
	status = atcab_verify_stored(test_msg, test_signature, public_key_id, &is_verified);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	TEST_ASSERT_EQUAL(true, is_verified);

	status = atcab_release();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_basic_verify_invalidate(void)
{
	const uint16_t public_key_id = 14;
	const uint16_t private_key_id = 0;
	const uint16_t validation_private_key_id = 2;

	ATCA_STATUS status = ATCA_SUCCESS;
	bool is_locked = false;
	uint8_t config[128];
	uint8_t sn[9];
	//uint8_t validation_public_key[ATCA_PUB_KEY_SIZE];
	//uint16_t validation_public_key_id = 0;
	uint8_t public_key[ATCA_PUB_KEY_SIZE];
	uint8_t test_msg[32];
	uint8_t test_signature[ATCA_SIG_SIZE];
	bool is_verified = false;
	uint8_t valid_buf[4];
	uint8_t nonce[32];
	uint8_t rand_out[ATCA_KEY_SIZE];
	atca_temp_key_t temp_key;
	atca_nonce_in_out_t nonce_params;
	uint8_t gen_key_other_data[3];
	atca_gen_key_in_out_t gen_key_params;
	uint8_t verify_other_data[19];
	uint8_t validation_msg[55];
	uint8_t validation_digest[32];
	atca_sign_internal_in_out_t sign_params;
	uint8_t validation_signature[ATCA_SIG_SIZE];

	// We need to start with the slot validated. This test will do that.
	test_basic_verify_validate();

	status = atcab_init(gCfg);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Make sure data zone is locked
	status = atcab_is_locked(LOCK_ZONE_DATA, &is_locked);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	if (!is_locked)
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed.");

	// Read config zone
	status = atcab_read_config_zone(config);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	memcpy(&sn[0], &config[0], 4);
	memcpy(&sn[4], &config[8], 5);

	// RUN: Run through the public invalidation process
	// This process has two parties. First is the device with a validated public key slot.
	// Whenever that slot gets updated, a Validation Authority (which has the validation private
	// key) is required to validate the new public key before it can be used.

	// Validation Authority: Get the public key to be invalidated
	status = atcab_get_pubkey(private_key_id, public_key);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Create and sign some data for testing
	status = atcab_random(test_msg);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	status = atcab_sign(private_key_id, test_msg, test_signature);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	status = atcab_verify_extern(test_msg, test_signature, public_key, &is_verified);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	TEST_ASSERT_EQUAL(true, is_verified);

	// Make sure public key is currently validated
	status = atcab_read_zone(ATCA_ZONE_DATA, public_key_id, 0, 0, valid_buf, 4);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	TEST_ASSERT_EQUAL(0x5, valid_buf[0] >> 4); // Validation status is the 4 upper-most bits in the slot

	// Additionally, check to make sure a verify(stored) command works
	status = atcab_verify_stored(test_msg, test_signature, public_key_id, &is_verified);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	TEST_ASSERT_EQUAL(true, is_verified);

	// Validated Device: Invalidation process needs to start with a nonce (random is most secure)
	// Not using random due to limitations with simulating the validated device and validation
	// authority on the same device.
	memset(nonce, 0, sizeof(nonce));
	memset(&temp_key, 0, sizeof(temp_key));
	nonce_params.mode = NONCE_MODE_PASSTHROUGH;
	nonce_params.num_in = nonce;
	nonce_params.rand_out = rand_out;
	nonce_params.temp_key = &temp_key;
	status = atcab_nonce(nonce_params.num_in);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Validation Authority: Calculate same nonce
	status = atcah_nonce(&nonce_params);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Validated Authority: GenKey format is then used to combine the nonce with the new public key to be validated
	memset(gen_key_other_data, 0, sizeof(gen_key_other_data));
	gen_key_params.mode = GENKEY_MODE_PUBKEY_DIGEST;
	gen_key_params.key_id = public_key_id;
	gen_key_params.public_key = public_key;
	gen_key_params.public_key_size = sizeof(public_key);
	gen_key_params.other_data = gen_key_other_data;
	gen_key_params.sn = sn;
	gen_key_params.temp_key = &temp_key;
	status = atcah_gen_key_msg(&gen_key_params);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Validation Authority: Build validation message which uses the Sign(Internal) format
	memset(&sign_params, 0, sizeof(sign_params));
	sign_params.sn = sn;
	sign_params.verify_other_data = verify_other_data;
	sign_params.for_invalidate = true;
	sign_params.message = validation_msg;
	sign_params.digest = validation_digest;
	sign_params.temp_key = &temp_key;
	status = atcah_sign_internal_msg(gCfg->devtype, &sign_params);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Validation Authority: Sign the validation message
	status = atcab_sign(validation_private_key_id, validation_digest, validation_signature);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// The previous sign cleared TempKey, so we have to reset it. This is because the unit test is trying
	// to perform the actions of the Validation Authority and the Validate Device on the same device.
	// This wouldn't be needed normally.
	status = atcab_nonce(nonce_params.num_in);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Validated Device: Combine the public key with the nonce
	status = atcab_genkey_base(gen_key_params.mode, gen_key_params.key_id, gen_key_params.other_data, NULL);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Validated Device: Use Verify(Invalidate) command to invalidate the existing public key
	status = atcab_verify_invalidate(public_key_id, validation_signature, verify_other_data, &is_verified);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	TEST_ASSERT_EQUAL(true, is_verified);

	// Make sure the previous command invalidated the public key
	status = atcab_read_zone(ATCA_ZONE_DATA, public_key_id, 0, 0, valid_buf, 4);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	TEST_ASSERT_EQUAL(0xA, valid_buf[0] >> 4); // Validation status is the 4 upper-most bits in the slot

	// Additionally, check to make sure a verify(stored) command with it fails.
	status = atcab_verify_stored(test_msg, test_signature, public_key_id, &is_verified);
	TEST_ASSERT_EQUAL(ATCA_EXECUTION_ERROR, status);

	status = atcab_release();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_basic_ecdh(void)
{
	ATCA_STATUS status;
	struct atca_nonce_in_out nonce_param;
	struct atca_gen_dig_in_out gendig_param;
	struct atca_temp_key tempkey;
	uint8_t read_key_id = 0x04;
	uint8_t pub_alice[ATCA_PUB_KEY_SIZE], pub_bob[ATCA_PUB_KEY_SIZE];
	uint8_t pms_alice[ECDH_KEY_SIZE], pms_bob[ECDH_KEY_SIZE];
	uint8_t rand_out[ATCA_KEY_SIZE], cipher_text[ATCA_KEY_SIZE], read_key[ATCA_KEY_SIZE];
	uint8_t key_id_alice = 0, key_id_bob = 2;
	char displaystr[256];
	uint8_t frag[4] = { 0x44, 0x44, 0x44, 0x44 };
	uint8_t non_clear_response[3] = { 0x00, 0x03, 0x40 };
	bool isLocked;
	static uint8_t NUM_IN[20] = {
		0x50, 0xDF, 0xD7, 0x82, 0x5B, 0x10, 0x0F, 0x2D, 0x8C, 0xD2, 0x0A, 0x91, 0x15, 0xAC, 0xED, 0xCF,
		0x5A, 0xEE, 0x76, 0x94
	};
	int displen = sizeof(displaystr);
	uint8_t i;
    uint8_t sn[32];

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed." );

	// set to known values that should be overwritten, so these can be tested
	memset(pub_alice, 0x44, ATCA_PUB_KEY_SIZE);
	memset(pub_bob, 0x44, ATCA_PUB_KEY_SIZE);

	status = atcab_genkey( key_id_alice, pub_alice );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);
	atcab_bin2hex( pub_alice, ATCA_PUB_KEY_SIZE, displaystr, &displen);
	printf("alice slot %d pubkey:\r\n%s\r\n", key_id_alice, displaystr);

	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, memcmp(pub_alice, frag, sizeof(frag)), "Alice key not initialized");

	status = atcab_genkey( key_id_bob, pub_bob );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);
	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, memcmp(pub_bob, frag, sizeof(frag)), "Bob key not initialized");

	atcab_bin2hex( pub_bob, ATCA_PUB_KEY_SIZE, displaystr, &displen);
	printf("bob slot %d pubkey:\r\n%s\r\n", key_id_bob, displaystr);

	// slot 0 is a non-clear response - "Write Slot N+1" is in slot config for W25 config
	// generate premaster secret from alice's key and bob's pubkey
	status = atcab_ecdh( key_id_alice, pub_bob, pms_alice );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_NOT_EQUAL( 0, memcmp(pub_alice, frag, sizeof(frag)));
	TEST_ASSERT_EQUAL( 0, memcmp(pms_alice, non_clear_response, sizeof(non_clear_response)));

	//atcab_bin2hex(pms_alice, ECDH_KEY_SIZE, displaystr, &displen );
	//printf("alice's pms in slot N+1. Non-clear response:\r\n%s\r\n", displaystr);

	status = atcab_ecdh( key_id_bob, pub_alice, pms_bob );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_NOT_EQUAL( 0, memcmp(pms_bob, frag, sizeof(frag)));

	atcab_bin2hex(pms_bob, ECDH_KEY_SIZE, displaystr, &displen );
	printf("bob's pms:\r\n%s\r\n", displaystr);

	// TODO - do an encrypted read of slot 1 (Write into Slot 0 + 1 when ECDH) alice's premaster secret, then
	// memcmp it to bob's premaster secret - they should be identical
	//memset( read_key, 0xFF, sizeof(read_key) );
    memcpy(read_key, g_slot4_key, 32);
	status = atcab_write_zone(ATCA_ZONE_DATA, read_key_id, 0, 0, &read_key[0], ATCA_BLOCK_SIZE);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_challenge_seed_update(NUM_IN, rand_out);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	nonce_param.mode = NONCE_MODE_SEED_UPDATE;
	nonce_param.num_in = NUM_IN;
	nonce_param.rand_out = rand_out;
	nonce_param.temp_key = &tempkey;

	status = atcah_nonce(&nonce_param);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_gendig(GENDIG_ZONE_DATA, read_key_id, cipher_text, sizeof(cipher_text));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_read_zone(ATCA_ZONE_DATA, key_id_alice + 1, 0, 0, cipher_text, sizeof(cipher_text));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
    
    // Read the device SN
    status = atcab_read_zone(ATCA_ZONE_CONFIG, 0, 0, 0, sn, 32);
    TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
    // Make the SN continuous by moving SN[4:8] right after SN[0:3]
    memmove(&sn[4], &sn[8], 5);

	gendig_param.zone = GENDIG_ZONE_DATA;
	gendig_param.key_id = read_key_id;
    gendig_param.sn = sn;
	gendig_param.stored_value = read_key;
	gendig_param.temp_key = &tempkey;

	status = atcah_gen_dig(&gendig_param);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	for (i = 0; i < ATCA_KEY_SIZE; i++)
		pms_alice[i] = cipher_text[i] ^ tempkey.value[i];

	atcab_bin2hex(pms_alice, ECDH_KEY_SIZE, displaystr, &displen );
	printf("alice's pms:\r\n%s\r\n", displaystr);

	TEST_ASSERT_EQUAL_MEMORY(pms_alice, pms_bob, sizeof(pms_alice));

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_gendig(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t random_number[32];
	uint16_t key_id = 0x04;
	uint8_t dummy[32];
	bool isLocked;

	status = atcab_init( gCfg );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed." );

	status = atcab_random( random_number );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_challenge( random_number );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_gendig(GENDIG_ZONE_DATA, key_id, dummy, sizeof(dummy));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_mac(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t mode = MAC_MODE_CHALLENGE;
	uint16_t key_id = 0x0001;
	uint8_t challenge[32];
	uint8_t mac_response[MAC_SIZE];
	uint8_t seed[NONCE_NUMIN_SIZE];
	uint8_t rand_out[RANDOM_NUM_SIZE];
	bool isLocked;

	status = atcab_init( gCfg );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( ATCA_ZONE_DATA, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed." );

	status = atcab_random( challenge );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_mac( mode, key_id, challenge, mac_response );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	// Load random nonce
	memset(seed, 0, sizeof(seed));
	status = atcab_nonce_rand(seed, rand_out);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// MAC with key and TempKey, no challenge
	status = atcab_mac(MAC_MODE_BLOCK2_TEMPKEY, key_id, NULL, mac_response);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_checkmac(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t mode = MAC_MODE_CHALLENGE;
	uint16_t key_id = 0x0001;
	uint8_t challenge[RANDOM_NUM_SIZE];
	uint8_t response[MAC_SIZE];
	uint8_t other_data[CHECKMAC_OTHER_DATA_SIZE];
    atca_temp_key_t temp_key;
    uint8_t num_in[NONCE_NUMIN_SIZE];
    uint8_t rand_out[RANDOM_NUM_SIZE];
    atca_nonce_in_out_t nonce_params;
    uint8_t sn[ATCA_SERIAL_NUM_SIZE];
    atca_check_mac_in_out_t checkmac_params;
	bool isLocked;
    size_t i;

	status = atcab_init( gCfg );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( ATCA_ZONE_DATA, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed." );

	if (gCfg->devtype == ATSHA204A)
		key_id = 0x0001;
	else
		key_id = 0x0004;
	memset( challenge, 0x55, 32 );  // a 32-byte challenge

	status = atcab_mac( mode, key_id, challenge, response);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	memset(other_data, 0, sizeof(other_data));
	other_data[0] = ATCA_MAC;
	other_data[2] = (uint8_t)key_id;

	status = atcab_checkmac( mode, key_id, challenge, response, other_data );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

    // This next part tests the atcah_check_mac() function

    // Read SN
    status = atcab_read_serial_number(sn);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Perform random nonce
    memset(&temp_key, 0, sizeof(temp_key));
    memset(num_in, 0, sizeof(num_in));
    nonce_params.mode = NONCE_MODE_SEED_UPDATE;
    nonce_params.num_in = num_in;
    nonce_params.rand_out = rand_out;
    nonce_params.temp_key = &temp_key;
    status = atcab_nonce_rand(nonce_params.num_in, rand_out);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Calculate nonce value
    status = atcah_nonce(&nonce_params);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Calculate response
    for (i = 0; i < sizeof(other_data); i++)
        other_data[i] = (uint8_t)(i + 0xF0);
    checkmac_params.mode = CHECKMAC_MODE_BLOCK2_TEMPKEY;
    checkmac_params.key_id = key_id;
    checkmac_params.client_chal = NULL;
    checkmac_params.client_resp = response;
    checkmac_params.other_data = other_data;
    checkmac_params.sn = sn;
    checkmac_params.otp = NULL;
    checkmac_params.slot_key = g_slot4_key;
    checkmac_params.target_key = NULL;
    checkmac_params.temp_key = &temp_key;
    status = atcah_check_mac(&checkmac_params);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Perform CheckMac
    status = atcab_checkmac(
        checkmac_params.mode,
        checkmac_params.key_id,
        checkmac_params.client_chal,
        checkmac_params.client_resp,
        checkmac_params.other_data);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_hmac(void)
{
    ATCA_STATUS status = ATCA_GEN_FAIL;
    bool isLocked = false;
    uint8_t sn[ATCA_SERIAL_NUM_SIZE];
    uint8_t otp[ATCA_OTP_SIZE];
    uint8_t num_in[20];
    struct atca_temp_key temp_key;
    struct atca_nonce_in_out nonce_params;
    uint8_t rand_out[32];
    uint8_t hmac_digest[32];
    struct atca_hmac_in_out hmac_params;
    uint8_t hmac_digest_host[32];
    uint8_t modes[] = {
          HMAC_MODE_FLAG_TK_RAND,
          HMAC_MODE_FLAG_TK_RAND | HMAC_MODE_FLAG_OTP88,
          HMAC_MODE_FLAG_TK_RAND | HMAC_MODE_FLAG_OTP88 | HMAC_MODE_FLAG_OTP64,
          HMAC_MODE_FLAG_TK_RAND | HMAC_MODE_FLAG_OTP88 | HMAC_MODE_FLAG_OTP64 | HMAC_MODE_FLAG_FULLSN,
          HMAC_MODE_FLAG_TK_RAND | HMAC_MODE_FLAG_OTP88 | HMAC_MODE_FLAG_FULLSN,
          HMAC_MODE_FLAG_TK_RAND | HMAC_MODE_FLAG_OTP64,
          HMAC_MODE_FLAG_TK_RAND | HMAC_MODE_FLAG_OTP64 | HMAC_MODE_FLAG_FULLSN,
          HMAC_MODE_FLAG_TK_RAND | HMAC_MODE_FLAG_FULLSN,
    };
    size_t i = 0;
    
    status = atcab_init( gCfg );
    TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
    
    status = atcab_is_locked( ATCA_ZONE_DATA, &isLocked );
    TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);
    if ( !isLocked )
        TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed." );
        
    status = atcab_read_serial_number(sn);
    TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);
    
    status = atcab_read_bytes_zone(ATCA_ZONE_OTP, 0, 0, otp, ATCA_OTP_SIZE);
    TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);
    
    for (i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
    {
        if (gCfg->devtype == ATECC508A && (modes[i] & HMAC_MODE_FLAG_OTP88 || modes[i] & HMAC_MODE_FLAG_OTP64))
            continue; // ATECC508A doesn't support OTP mode bits
        
        memset(&temp_key, 0, sizeof(temp_key));
        memset(num_in, 0, sizeof(num_in));
        nonce_params.mode = NONCE_MODE_SEED_UPDATE;
        nonce_params.num_in = num_in;
        nonce_params.temp_key = &temp_key;
        nonce_params.rand_out = rand_out;
        status = atcab_nonce_rand(nonce_params.num_in, rand_out);
        TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);
        
        status = atcah_nonce(&nonce_params);
        TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);
        
        hmac_params.mode = modes[i];
        hmac_params.key_id = 4;
        hmac_params.key = g_slot4_key;
        hmac_params.otp = otp;
        hmac_params.sn = sn;
        hmac_params.response = hmac_digest_host;
        hmac_params.temp_key = &temp_key;
        status = atcab_hmac(hmac_params.mode, hmac_params.key_id, hmac_digest);
        TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);
        
        status = atcah_hmac(&hmac_params);
        TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);
        TEST_ASSERT_EQUAL_MEMORY(hmac_digest, hmac_params.response, sizeof(hmac_digest));
    }    
}

void test_basic_derivekey(void)
{
    ATCA_STATUS status = ATCA_GEN_FAIL;
    uint16_t target_key_id = 9;
    const uint8_t parent_key[32] = {
        0x55, 0xe1, 0xe1, 0x97, 0x53, 0xf8, 0xee, 0x0b, 0x20, 0x4b, 0x97, 0x09, 0xfd, 0xd0, 0xf0, 0xf9,
        0x75, 0x14, 0x60, 0x21, 0xcc, 0x5f, 0x96, 0x7d, 0xa1, 0xe1, 0x30, 0xfe, 0xed, 0xb0, 0xfe, 0x87};
    const uint8_t nonce_seed[20] = {
        0xe5, 0x1e, 0xb3, 0xcb, 0x5d, 0x27, 0x59, 0xfa, 0x03, 0xd8, 0x88, 0xbb, 0x54, 0x35, 0x35, 0xb6,
        0x74, 0x25, 0x10, 0x21};
    uint8_t sn[9];
    uint8_t rand_out[32];
    atca_temp_key_t temp_key_params;
    atca_nonce_in_out_t nonce_params;
    uint8_t derived_key[32];
    struct atca_derive_key_in_out derivekey_params;
    const uint8_t challenge[32] = {
        0x10, 0x04, 0xbb, 0x7b, 0xc7, 0xe2, 0x40, 0xd4, 0xca, 0x1d, 0x6b, 0x04, 0x73, 0x22, 0xd5, 0xfd,
        0xad, 0x69, 0x2a, 0x73, 0x39, 0x8e, 0xaa, 0xc3, 0x3a, 0x5a, 0xc4, 0x9e, 0x02, 0xb4, 0x8b, 0x5d};
    const uint8_t other_data[13] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t response[32];
    atca_check_mac_in_out_t checkmac_params;

    // Initialize device
    status = atcab_init(gCfg);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Read the device serial number
    status = atcab_read_serial_number(sn);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    // Initialize the slot with a known key
    status = atcab_write_enc(target_key_id, 0, parent_key, g_slot4_key, 4);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    memset(&temp_key_params, 0, sizeof(temp_key_params));
    
    // Use a random nonce for the derive key command
    nonce_params.mode = NONCE_MODE_SEED_UPDATE;
    nonce_params.num_in = nonce_seed;
    nonce_params.rand_out = rand_out;
    nonce_params.temp_key = &temp_key_params;
    status = atcab_nonce_rand(nonce_params.num_in, rand_out);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Calculate TempKey from nonce command
    status = atcah_nonce(&nonce_params);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Run the derive key command assuming target/roll mode
    derivekey_params.mode = 0; // Random nonce generated TempKey
    derivekey_params.target_key_id = target_key_id;
    derivekey_params.parent_key = parent_key;
    derivekey_params.sn = sn;
    derivekey_params.target_key = derived_key;
    derivekey_params.temp_key = &temp_key_params;
    status = atcab_derivekey(derivekey_params.mode, derivekey_params.target_key_id, NULL);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Calculate the derived key
    status = atcah_derive_key(&derivekey_params);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Generate new random nonce for validating derived key
    status = atcab_nonce_rand(nonce_params.num_in, rand_out);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Calculate TempKey from nonce command
    status = atcah_nonce(&nonce_params);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Calculate checkmac response for validation
    memset(&checkmac_params, 0, sizeof(checkmac_params));
    checkmac_params.mode = CHECKMAC_MODE_CHALLENGE; // Checkmac with challenge and random nonce
    checkmac_params.key_id = target_key_id;
    checkmac_params.client_chal = challenge;
    checkmac_params.client_resp = response;
    checkmac_params.other_data = other_data;
    checkmac_params.sn = sn;
    checkmac_params.slot_key = derived_key;
    checkmac_params.temp_key = &temp_key_params;
    status = atcah_check_mac(&checkmac_params);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Run the checkmac command to validate the derived key
    status = atcab_checkmac(checkmac_params.mode, checkmac_params.key_id, checkmac_params.client_chal, checkmac_params.client_resp, checkmac_params.other_data);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    status = atcab_release();
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_basic_derivekey_mac(void)
{
    ATCA_STATUS status = ATCA_GEN_FAIL;
    uint16_t target_key_id = 3;
    const uint8_t nonce_seed[20] = {
        0xe5, 0x1e, 0xb3, 0xcb, 0x5d, 0x27, 0x59, 0xfa, 0x03, 0xd8, 0x88, 0xbb, 0x54, 0x35, 0x35, 0xb6,
        0x74, 0x25, 0x10, 0x21};
    uint8_t sn[9];
    uint8_t rand_out[32];
    atca_temp_key_t temp_key_params;
    atca_nonce_in_out_t nonce_params;
    uint8_t mac[32];
    struct atca_derive_key_mac_in_out derivekey_mac_params;
    uint8_t derived_key[32];
    struct atca_derive_key_in_out derivekey_params;
    const uint8_t challenge[32] = {
        0x10, 0x04, 0xbb, 0x7b, 0xc7, 0xe2, 0x40, 0xd4, 0xca, 0x1d, 0x6b, 0x04, 0x73, 0x22, 0xd5, 0xfd,
        0xad, 0x69, 0x2a, 0x73, 0x39, 0x8e, 0xaa, 0xc3, 0x3a, 0x5a, 0xc4, 0x9e, 0x02, 0xb4, 0x8b, 0x5d};
    const uint8_t other_data[13] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t response[32];
    atca_check_mac_in_out_t checkmac_params;

    // Initialize device
    status = atcab_init(gCfg);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Read the device serial number
    status = atcab_read_serial_number(sn);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
    
    memset(&temp_key_params, 0, sizeof(temp_key_params));
    
    // Use a random nonce for the derive key command
    nonce_params.mode = NONCE_MODE_SEED_UPDATE;
    nonce_params.num_in = nonce_seed;
    nonce_params.rand_out = rand_out;
    nonce_params.temp_key = &temp_key_params;
    status = atcab_nonce_rand(nonce_params.num_in, rand_out);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Calculate TempKey from nonce command
    status = atcah_nonce(&nonce_params);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Calculate the DeriveKey MAC required
    derivekey_mac_params.mode = 0; // Random nonce generated TempKey
    derivekey_mac_params.target_key_id = target_key_id;
    derivekey_mac_params.sn = sn;
    derivekey_mac_params.parent_key = g_slot4_key;
    derivekey_mac_params.mac = mac;
    status = atcah_derive_key_mac(&derivekey_mac_params);

    // Run the derive key command assuming target/roll mode
    derivekey_params.mode = derivekey_mac_params.mode;
    derivekey_params.target_key_id = derivekey_mac_params.target_key_id;
    derivekey_params.parent_key = derivekey_mac_params.parent_key;
    derivekey_params.sn = derivekey_mac_params.sn;
    derivekey_params.target_key = derived_key;
    derivekey_params.temp_key = &temp_key_params;
    status = atcab_derivekey(derivekey_params.mode, derivekey_params.target_key_id, mac);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Calculate the derived key
    status = atcah_derive_key(&derivekey_params);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Generate new random nonce for validating derived key
    status = atcab_nonce_rand(nonce_params.num_in, rand_out);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Calculate TempKey from nonce command
    status = atcah_nonce(&nonce_params);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Calculate checkmac response for validation
    memset(&checkmac_params, 0, sizeof(checkmac_params));
    checkmac_params.mode = CHECKMAC_MODE_CHALLENGE; // Checkmac with challenge and random nonce
    checkmac_params.key_id = target_key_id;
    checkmac_params.client_chal = challenge;
    checkmac_params.client_resp = response;
    checkmac_params.other_data = other_data;
    checkmac_params.sn = sn;
    checkmac_params.slot_key = derived_key;
    checkmac_params.temp_key = &temp_key_params;
    status = atcah_check_mac(&checkmac_params);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    // Run the checkmac command to validate the derived key
    status = atcab_checkmac(checkmac_params.mode, checkmac_params.key_id, checkmac_params.client_chal, checkmac_params.client_resp, checkmac_params.other_data);
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

    status = atcab_release();
    TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_basic_sha(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t message[SHA_BLOCK_SIZE];
	uint8_t digest[ATCA_SHA_DIGEST_SIZE];
	char displaystr[256]; int len;
	uint8_t rightAnswer[] = { 0x1A, 0x3A, 0xA5, 0x45, 0x04, 0x94, 0x53, 0xAF,
		                      0xDF, 0x17, 0xE9, 0x89, 0xA4, 0x1F, 0xA0, 0x97,
		                      0x94, 0xA5, 0x1B, 0xD5, 0xDB, 0x91, 0x36, 0x37,
		                      0x67, 0x55, 0x0C, 0x0F, 0x0A, 0xF3, 0x27, 0xD4 };

	memset( message, 0xBC, sizeof(message) );

	status = atcab_init( gCfg );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_sha( sizeof(message), message, digest );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL_MEMORY(rightAnswer, digest, ATCA_SHA_DIGEST_SIZE);

	len = sizeof( displaystr );
	atcab_bin2hex(digest, ATCA_SHA_DIGEST_SIZE, displaystr, &len );
	printf("digest: %s\r\n", displaystr);

	memset( message, 0x5A, sizeof(message) );
	status = atcab_sha_start();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_sha_update( message );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_sha_update( message );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_sha_update( message );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_sha_end( digest, 0, NULL );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

/** \brief test HW SHA with a long message > SHA block size and not an exact SHA block-size increment
 *
 */
void test_basic_sha_long(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t message[SHA_BLOCK_SIZE + 63];  // just short of two blocks
	uint8_t digest[ATCA_SHA_DIGEST_SIZE];
	char displaystr[256]; int len;
	uint8_t rightAnswer[] = { 0xA9, 0x22, 0x18, 0x56, 0x43, 0x70, 0xA0, 0x57,
		                      0x27, 0x3F, 0xF4, 0x85, 0xA8, 0x07, 0x3F, 0x32,
		                      0xFC, 0x1F, 0x14, 0x12, 0xEC, 0xA2, 0xE3, 0x0B,
		                      0x81, 0xA8, 0x87, 0x76, 0x0B, 0x61, 0x31, 0x72 };

	memset( message, 0xBC, sizeof(message) );
	memset( digest, 0x00, ATCA_SHA_DIGEST_SIZE);
	status = atcab_init( gCfg );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_sha( sizeof(message), message, digest );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL_INT8_ARRAY(rightAnswer, digest, ATCA_SHA_DIGEST_SIZE);

	len = sizeof(displaystr);
	atcab_bin2hex(digest, sizeof(digest), displaystr, &len );
	printf("digest: %s\n\r", displaystr );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}


/** \brief test HW SHA with a short message < SHA block size and not an exact SHA block-size increment
 *
 */
void test_basic_sha_short(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t message[10];  // a short message to sha
	uint8_t digest[ATCA_SHA_DIGEST_SIZE];
	char displaystr[256]; int len;
	uint8_t rightAnswer[] = { 0x30, 0x3f, 0xf8, 0xba, 0x40, 0xa2, 0x06, 0xe7,
		                      0xa9, 0x50, 0x02, 0x1e, 0xf5, 0x10, 0x66, 0xd4,
		                      0xa0, 0x01, 0x54, 0x75, 0x32, 0x3e, 0xe9, 0xf2,
		                      0x4a, 0xc8, 0xc9, 0x63, 0x29, 0x8f, 0x34, 0xce };

	memset( message, 0xBC, sizeof(message) );
	memset( digest, 0x00, ATCA_SHA_DIGEST_SIZE);
	status = atcab_init( gCfg );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_sha( sizeof(message), message, digest );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	TEST_ASSERT_EQUAL_INT8_ARRAY(rightAnswer, digest, ATCA_SHA_DIGEST_SIZE);

	len = sizeof(displaystr);
	atcab_bin2hex(digest, sizeof(digest), displaystr, &len );
	printf("digest: %s\n\r", displaystr );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}


// Helper tests
void atca_helper_tests(void)
{
	// add helper specific tests here...for example,
	RUN_TEST(test_base64encode_decode);
}

void test_base64encode_decode(void)
{
	// Use an arbitrary buffer to encode and decode
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t byteArray[] = { 
		0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
		0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
		0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
		0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
		0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
		0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59 
	};
	size_t byteArrayLen = sizeof(byteArray);
	char encoded[512] = { 0 };
	size_t encodedLen = 512;
	uint8_t decoded[512] = { 0 };
	size_t decodedLen = 512;

	///////////////////////////////////////////////////////////////////////////
	// Use (% 3) boundry
	// Encode the bytes
	//status = atcab_base64encode_(byteArray, byteArrayLen, encoded, &encodedLen, false);
	encodedLen = 512;
	status = atcab_base64encode(byteArray, byteArrayLen, encoded, &encodedLen);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Decode the bytes
	decodedLen = 512;
	status = atcab_base64decode(encoded, encodedLen, decoded, &decodedLen);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Check the buffer sizes
	TEST_ASSERT_EQUAL(byteArrayLen, decodedLen);

	// Check that the buffer is what we stared with
	TEST_ASSERT_EQUAL_MEMORY(decoded, byteArray, decodedLen);

	///////////////////////////////////////////////////////////////////////////
	// Use ((% 3)-1) boundry
	// Encode the bytes
	//status = atcab_base64encode_(byteArray, byteArrayLen, encoded, &encodedLen, false);
	encodedLen = 512;
	status = atcab_base64encode(byteArray, byteArrayLen-1, encoded, &encodedLen);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Decode the bytes
	decodedLen = 512;
	status = atcab_base64decode(encoded, encodedLen, decoded, &decodedLen);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Check the buffer sizes
	TEST_ASSERT_EQUAL(byteArrayLen-1, decodedLen);

	// Check that the buffer is what we stared with
	TEST_ASSERT_EQUAL_MEMORY(decoded, byteArray, decodedLen);

	///////////////////////////////////////////////////////////////////////////
	// Use ((% 3)-2) boundry
	// Encode the bytes
	//status = atcab_base64encode_(byteArray, byteArrayLen, encoded, &encodedLen, false);
	encodedLen = 512;
	status = atcab_base64encode(byteArray, byteArrayLen-2, encoded, &encodedLen);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Decode the bytes
	decodedLen = 512;
	status = atcab_base64decode(encoded, encodedLen, decoded, &decodedLen);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Check the buffer sizes
	TEST_ASSERT_EQUAL(byteArrayLen-2, decodedLen);

	// Check that the buffer is what we stared with
	TEST_ASSERT_EQUAL_MEMORY(decoded, byteArray, decodedLen);

	///////////////////////////////////////////////////////////////////////////
	// Use ((% 3)-3) boundry
	// Encode the bytes
	//status = atcab_base64encode_(byteArray, byteArrayLen, encoded, &encodedLen, false);
	encodedLen = 512;
	status = atcab_base64encode(byteArray, byteArrayLen-3, encoded, &encodedLen);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Decode the bytes
	decodedLen = 512;
	status = atcab_base64decode(encoded, encodedLen, decoded, &decodedLen);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Check the buffer sizes
	TEST_ASSERT_EQUAL(byteArrayLen-3, decodedLen);

	// Check that the buffer is what we stared with
	TEST_ASSERT_EQUAL_MEMORY(decoded, byteArray, decodedLen);

}


static const uint8_t nist_hash_msg1[] = "abc";
static const uint8_t nist_hash_msg2[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
static const uint8_t nist_hash_msg3[] = "a";

void test_basic_hw_sha2_256_nist1(void)
{
	const uint8_t digest_ref[] = {
		0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA, 0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
		0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C, 0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD
	};
	uint8_t digest[ATCA_SHA2_256_DIGEST_SIZE];
	ATCA_STATUS status;

	status = atcab_init(gCfg);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	TEST_ASSERT_EQUAL(ATCA_SHA2_256_DIGEST_SIZE, sizeof(digest_ref));

	status = atcab_hw_sha2_256(nist_hash_msg1, sizeof(nist_hash_msg1) - 1, digest);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	TEST_ASSERT_EQUAL_MEMORY(digest_ref, digest, sizeof(digest_ref));

	status = atcab_release();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_basic_hw_sha2_256_nist2(void)
{
	const uint8_t digest_ref[] = {
		0x24, 0x8D, 0x6A, 0x61, 0xD2, 0x06, 0x38, 0xB8, 0xE5, 0xC0, 0x26, 0x93, 0x0C, 0x3E, 0x60, 0x39,
		0xA3, 0x3C, 0xE4, 0x59, 0x64, 0xFF, 0x21, 0x67, 0xF6, 0xEC, 0xED, 0xD4, 0x19, 0xDB, 0x06, 0xC1
	};
	uint8_t digest[ATCA_SHA2_256_DIGEST_SIZE];
	ATCA_STATUS status;

	status = atcab_init(gCfg);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	TEST_ASSERT_EQUAL(ATCA_SHA2_256_DIGEST_SIZE, sizeof(digest_ref));

	status = atcab_hw_sha2_256(nist_hash_msg2, sizeof(nist_hash_msg2) - 1, digest);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	TEST_ASSERT_EQUAL_MEMORY(digest_ref, digest, sizeof(digest_ref));

	status = atcab_release();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

#ifdef WIN32
static void hex_to_uint8(const char hex_str[2], uint8_t* num)
{
	*num = 0;

	if (hex_str[0] >= '0' && hex_str[0] <= '9')
		*num += (hex_str[0] - '0') << 4;
	else if (hex_str[0] >= 'A' && hex_str[0] <= 'F')
		*num += (hex_str[0] - 'A' + 10) << 4;
	else if (hex_str[0] >= 'a' && hex_str[0] <= 'f')
		*num += (hex_str[0] - 'a' + 10) << 4;
	else
		TEST_FAIL_MESSAGE("Not a hex digit.");

	if (hex_str[1] >= '0' && hex_str[1] <= '9')
		*num += (hex_str[1] - '0');
	else if (hex_str[1] >= 'A' && hex_str[1] <= 'F')
		*num += (hex_str[1] - 'A' + 10);
	else if (hex_str[1] >= 'a' && hex_str[1] <= 'f')
		*num += (hex_str[1] - 'a' + 10);
	else
		TEST_FAIL_MESSAGE("Not a hex digit.");
}

static void hex_to_data(const char* hex_str, uint8_t* data, size_t data_size)
{
	size_t i = 0;

	TEST_ASSERT_EQUAL_MESSAGE(data_size * 2, strlen(hex_str) - 1, "Hex string unexpected length.");

	for (i = 0; i < data_size; i++)
		hex_to_uint8(&hex_str[i * 2], &data[i]);
}

static int read_rsp_hex_value(FILE* file, const char* name, uint8_t* data, size_t data_size)
{
	char line[16384];
	char* str = NULL;
	size_t name_size = strlen(name);

	do {
		str = fgets(line, sizeof(line), file);
		if (str == NULL)
			continue;

		if (memcmp(line, name, name_size) == 0)
			str = &line[name_size];
		else
			str = NULL;
	} while (str == NULL && !feof(file));
	if (str == NULL)
		return ATCA_GEN_FAIL;
	hex_to_data(str, data, data_size);

	return ATCA_SUCCESS;
}

static int read_rsp_int_value(FILE* file, const char* name, int* value)
{
	char line[2048];
	char* str = NULL;
	size_t name_size = strlen(name);

	do {
		str = fgets(line, sizeof(line), file);
		if (str == NULL)
			continue;

		if (memcmp(line, name, name_size) == 0)
			str = &line[name_size];
		else
			str = NULL;
	} while (str == NULL && !feof(file));
	if (str == NULL)
		return ATCA_GEN_FAIL;
	*value = atoi(str);

	return ATCA_SUCCESS;
}

#endif

static void test_basic_hw_sha2_256_nist_simple(const char* filename)
{
#ifndef WIN32
	TEST_IGNORE_MESSAGE("Test only available under windows.");
#else
	FILE* rsp_file = NULL;
	uint8_t md_ref[ATCA_SHA2_256_DIGEST_SIZE];
	uint8_t md[sizeof(md_ref)];
	int len_bits = 0;
	uint8_t* msg = NULL;
	size_t count = 0;
	ATCA_STATUS status;

	status = atcab_init(gCfg);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	rsp_file = fopen(filename, "r");
	TEST_ASSERT_NOT_NULL_MESSAGE(rsp_file, "Failed to  open file");

	do {
		status = read_rsp_int_value(rsp_file, "Len = ", &len_bits);
		if (status != ATCA_SUCCESS)
			continue;

		msg = malloc(len_bits == 0 ? 1 : len_bits / 8);
		TEST_ASSERT_NOT_NULL_MESSAGE(msg, "malloc failed");

		status = read_rsp_hex_value(rsp_file, "Msg = ", msg, len_bits == 0 ? 1 : len_bits / 8);
		TEST_ASSERT_EQUAL(status, ATCA_SUCCESS);

		status = read_rsp_hex_value(rsp_file, "MD = ", md_ref, sizeof(md_ref));
		TEST_ASSERT_EQUAL(status, ATCA_SUCCESS);

		status = atcab_hw_sha2_256(msg, len_bits / 8, md);
		TEST_ASSERT_EQUAL(status, ATCA_SUCCESS);
		TEST_ASSERT_EQUAL_MEMORY(md_ref, md, sizeof(md_ref));

		free(msg);
		msg = NULL;
		count++;
	} while (status == ATCA_SUCCESS);
	TEST_ASSERT_MESSAGE(count > 0, "No long tests found in file.");

	status = atcab_release();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
#endif
}

void test_basic_hw_sha2_256_nist_short(void)
{
	test_basic_hw_sha2_256_nist_simple("cryptoauthlib/test/sha-byte-test-vectors/SHA256ShortMsg.rsp");
}

void test_basic_hw_sha2_256_nist_long(void)
{
	test_basic_hw_sha2_256_nist_simple("cryptoauthlib/test/sha-byte-test-vectors/SHA256LongMsg.rsp");
}

void test_basic_hw_sha2_256_nist_monte(void)
{
#ifndef WIN32
	TEST_IGNORE_MESSAGE("Test only available under windows.");
#else
	FILE* rsp_file = NULL;
	uint8_t seed[ATCA_SHA2_256_DIGEST_SIZE];
	uint8_t md[4][sizeof(seed)];
	int i, j;
	uint8_t m[sizeof(seed) * 3];
	uint8_t md_ref[sizeof(seed)];
	ATCA_STATUS status;

	status = atcab_init(gCfg);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	rsp_file = fopen("cryptoauthlib/test/sha-byte-test-vectors/SHA256Monte.rsp", "r");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, rsp_file, "Failed to  open sha-byte-test-vectors/SHA256Monte.rsp");

	// Find the seed value
	status = read_rsp_hex_value(rsp_file, "Seed = ", seed, sizeof(seed));
	TEST_ASSERT_EQUAL_MESSAGE(ATCA_SUCCESS, status, "Failed to find Seed value in file.");

	for (j = 0; j < 100; j++) {
		memcpy(&md[0], seed, sizeof(seed));
		memcpy(&md[1], seed, sizeof(seed));
		memcpy(&md[2], seed, sizeof(seed));
		for (i = 0; i < 1000; i++) {
			memcpy(m, md, sizeof(m));
			status = atcab_hw_sha2_256(m, sizeof(m), &md[3][0]);
			TEST_ASSERT_EQUAL_MESSAGE(ATCA_SUCCESS, status, "atcac_sw_sha1 failed");
			memmove(&md[0], &md[1], sizeof(seed) * 3);
		}
		status = read_rsp_hex_value(rsp_file, "MD = ", md_ref, sizeof(md_ref));
		TEST_ASSERT_EQUAL_MESSAGE(ATCA_SUCCESS, status, "Failed to find MD value in file.");
		TEST_ASSERT_EQUAL_MEMORY(md_ref, &md[2], sizeof(md_ref));
		memcpy(seed, &md[2], sizeof(seed));
	}

	status = atcab_release();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
#endif
}