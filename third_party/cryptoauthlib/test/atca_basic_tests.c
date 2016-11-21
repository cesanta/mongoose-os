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

// modified W25 ECC508 configuration (slot 7 has external sign turned on for cert testing, slot 7 has privwrite turned on)
uint8_t w25_test_ecc_configdata[ATCA_CONFIG_SIZE] = {
	0x01, 0x23, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x04, 0x05, 0x06, 0x07, 0xEE,  0x00, 0x01, 0x00,
	0xC0, 0x00, 0x55, 0x00, 0x8F, 0x2F, 0xC4, 0x44, 0x87, 0x20, 0xC4, 0x44, 0x8F,  0x0F, 0x8F, 0x8F,
	0x9F, 0x8F, 0x83, 0x64, 0xC4, 0x44, 0xC4, 0x44, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,  0x0F, 0x0F, 0x0F,
	0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF,  0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00,
	0x33, 0x00, 0x1C, 0x00, 0x13, 0x00, 0x1C, 0x00, 0x3C, 0x00, 0x1C, 0x00, 0x1C,  0x00, 0x33, 0x00,
	0x1C, 0x00, 0x1C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C,  0x00, 0x3C, 0x00
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

static const uint8_t g_slot4_key[32] = {
	0x37, 0x80, 0xe6, 0x3d, 0x49, 0x68, 0xad, 0xe5, 0xd8, 0x22, 0xc0, 0x13, 0xfc, 0xc3, 0x23, 0x84,
	0x5d, 0x1b, 0x56, 0x9f, 0xe7, 0x05, 0xb6, 0x00, 0x06, 0xfe, 0xec, 0x14, 0x5a, 0x0d, 0xb1, 0xe3
};

/** \brief tests of the Basic Crypto API functionality
 */
void atca_basic_tests(ATCADeviceType deviceType)
{
	UnityBegin("atca_basic_tests.c");

	// do this set of tests regardless of which device type is requested

	switch ( deviceType ) {
	case ATSHA204A:
		#ifdef ATCA_HAL_I2C
		gCfg = &cfg_sha204a_i2c_default;
		#elif defined(ATCA_HAL_SWI)
		gCfg = &cfg_sha204a_swi_default;
		#endif
		atca_sha204a_basic_tests(deviceType);
		break;
	case ATECC108A:
		#ifdef ATCA_HAL_I2C
		gCfg = &cfg_ateccx08a_i2c_default;
		#elif defined(ATCA_HAL_SWI)
		gCfg = &cfg_ateccx08a_swi_default;
		#endif
		gCfg->devtype = ATECC108A;
		atca_ecc108a_basic_tests(deviceType);
		break;
	case ATECC508A:
		#ifdef ATCA_HAL_I2C
		gCfg = &cfg_ateccx08a_i2c_default;
		#elif defined(ATCA_HAL_SWI)
		gCfg = &cfg_ateccx08a_swi_default;
		#endif
		gCfg->devtype = ATECC508A;
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
	RUN_TEST(test_basic_write_config_zone);
	RUN_TEST(test_basic_read_config_zone);
	RUN_TEST(test_basic_write_sha_config_zone);
	RUN_TEST(test_basic_read_sha_config_zone);
	RUN_TEST(test_basic_lock_config_zone);
	RUN_TEST(test_basic_write_otp_zone);
	RUN_TEST(test_basic_lock_data_zone);
	RUN_TEST(test_basic_read_otp_zone);
	RUN_TEST(test_basic_write_data_zone);
	RUN_TEST(test_basic_read_data_zone);
	RUN_TEST(test_basic_gendig);
	RUN_TEST(test_basic_mac);
	RUN_TEST(test_basic_checkmac);

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
	RUN_TEST(test_basic_lock_data_zone);
	RUN_TEST(test_write_bytes_slot);
	RUN_TEST(test_write_bytes_zone_slot8);
	RUN_TEST(test_basic_genkey);
	RUN_TEST(test_basic_sign);
	RUN_TEST(test_read_sig);
	RUN_TEST(test_basic_lock_data_slot);
	RUN_TEST(test_create_key);
	RUN_TEST(test_get_pubkey);
	RUN_TEST(test_basic_verify_external);
	RUN_TEST(test_basic_gendig);
	RUN_TEST(test_basic_mac);
	RUN_TEST(test_basic_checkmac);
	RUN_TEST(test_basic_sha);
	RUN_TEST(test_basic_sha_long);
	RUN_TEST(test_basic_sha_short);
	RUN_TEST(test_basic_priv_write_encrypted);
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

	switch ( locked ) {
	case true:
		status = atcab_read_zone(LOCK_ZONE_DATA, slot, block, offset, data, sizeof(data));
		TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
		break;
	case false:
		status = atcab_read_zone(LOCK_ZONE_DATA, slot, block, offset, data, sizeof(data));
		TEST_ASSERT_EQUAL( ATCA_EXECUTION_ERROR, status );
		break;
	}

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

		status = atcab_write_ecc_config_zone(w25_test_ecc_configdata);
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
	bool isLocked = false;
	uint8_t config_data[ATCA_CONFIG_SIZE];

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_read_ecc_config_zone(config_data);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	if ((isLocked == false) && (status == ATCA_SUCCESS)) {
		TEST_ASSERT_EQUAL_UINT8_ARRAY(&config_data[16], &w25_test_ecc_configdata[16], 70);
		// Ignore lock values, bytes 86 through 89
		TEST_ASSERT_EQUAL_UINT8_ARRAY(&config_data[16 + 74], &w25_test_ecc_configdata[16 + 74], 18);
	}

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

	status = atcab_write_config_zone(ATSHA204A, sha204_default_config);
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

	status = atcab_read_config_zone(ATSHA204A, config_data);
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

void test_write_bytes_slot(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = 0;
	uint8_t write_data[72];

	status = atcab_init(gCfg);

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed." );

	//reset lock
	isLocked = false;
	// check the lock status of the slot
	status = atcab_is_slot_locked(12, &isLocked);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	if ( isLocked != 0 )
		TEST_IGNORE_MESSAGE("Slot 12 locked already." );

	memset(write_data, 0xA5, sizeof(write_data));
	status = atcab_write_bytes_slot(12, 8, write_data, 32);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	//reset lock
	isLocked = false;
	// check the lock status of the slot
	status = atcab_is_slot_locked(12, &isLocked);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	if ( isLocked != 0 )
		TEST_IGNORE_MESSAGE("Slot 12 locked already." );

	status = atcab_write_bytes_slot(12, 60, write_data, 4);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	status = atcab_write_bytes_slot(4, 0, g_slot4_key, sizeof(g_slot4_key));
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

	status = atcab_write_bytes_zone(ATECC508A, ATCA_ZONE_CONFIG, 96, &w25_test_ecc_configdata[96], sizeof(w25_test_ecc_configdata) - 96);
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

	status = atcab_write_bytes_zone(ATSHA204A, ATCA_ZONE_OTP, 0x00, write_data, sizeof(write_data));
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

	status = atcab_read_bytes_zone(ATSHA204A, ATCA_ZONE_OTP, 0x00, sizeof(read_data), read_data);
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_write_data_zone(void)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = 0;
	uint8_t write_data[ATCA_BLOCK_SIZE];

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed." );

	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed." );

	memset(write_data, 0xB6, sizeof(write_data));

	status = atcab_write_bytes_zone(ATSHA204A, ATCA_ZONE_DATA, 0x100, write_data, sizeof(write_data));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

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

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed." );

	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed." );

	status = atcab_read_bytes_zone(ATSHA204A, ATCA_ZONE_DATA, 0x100, sizeof(read_data), read_data);
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

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed." );

	status = atcab_is_locked( LOCK_ZONE_DATA, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test to succeed." );

	memset(write_data, 0xA5, sizeof(write_data));

	status = atcab_write_bytes_zone(ATECC508A, ATCA_ZONE_DATA, 292, write_data, sizeof(write_data));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

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
	uint8_t signature[ATCA_SIG_SIZE];
	bool isLocked = false;

	status = atcab_init( gCfg );

	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status);

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	if ( !isLocked )
		TEST_IGNORE_MESSAGE("Configuration zone must be locked for this test to succeed.");

	memset( msg, 0x55, ATCA_SHA_DIGEST_SIZE );

	status = atcab_sign(0, msg, signature);
	TEST_ASSERT_EQUAL_MESSAGE( ATCA_SUCCESS, status, "signature failed" );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
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

void test_basic_verify_external(void)
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
	memset( read_key, 0xFF, sizeof(read_key) );
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

	status = atcab_gendig_host(GENDIG_ZONE_DATA, read_key_id, cipher_text, sizeof(cipher_text));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_read_zone(ATCA_ZONE_DATA, key_id_alice + 1, 0, 0, cipher_text, sizeof(cipher_text));
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	gendig_param.zone = GENDIG_ZONE_DATA;
	gendig_param.key_id = read_key_id;
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

	status = atcab_gendig_host(GENDIG_ZONE_DATA, key_id, dummy, sizeof(dummy));
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

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
}

void test_basic_checkmac(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t mode = MAC_MODE_CHALLENGE;
	uint16_t key_id = 0x0001;
	uint8_t challenge[RANDOM_NUM_SIZE];
	uint8_t mac_response[MAC_SIZE];
	uint8_t other_data[CHECKMAC_OTHER_DATA_SIZE];
	bool isLocked;

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

	status = atcab_mac( mode, key_id, challenge, mac_response );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	memset(other_data, 0, sizeof(other_data));
	other_data[0] = ATCA_MAC;
	other_data[2] = (uint8_t)key_id;

	status = atcab_checkmac( mode, key_id, challenge, mac_response, other_data );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_release();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );
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
	TEST_ASSERT_EQUAL_INT8_ARRAY(rightAnswer, digest, ATCA_SHA_DIGEST_SIZE);

	len = sizeof( displaystr );
	atcab_bin2hex(digest, ATCA_SHA_DIGEST_SIZE, displaystr, &len );
	printf("digest: %s\r\n", displaystr);

	memset( message, 0x5A, sizeof(message) );
	status = atcab_sha_start();
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_sha_update( sizeof(message), message );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_sha_update( sizeof(message), message );
	TEST_ASSERT_EQUAL( ATCA_SUCCESS, status );

	status = atcab_sha_update( sizeof(message), message );
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
