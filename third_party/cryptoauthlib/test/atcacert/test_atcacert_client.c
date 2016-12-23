/**
 * \file
 * \brief cert client tests
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


#include "atcacert/atcacert_client.h"
#include "test/unity.h"
#include "test/unity_fixture.h"
#include <string.h>
#include "cryptoauthlib.h"
#include "basic/atca_basic.h"
#include "crypto/atca_crypto_sw_sha2.h"
#include "test_cert_def_0_device.h"
#include "test_cert_def_1_signer.h"

extern ATCAIfaceCfg *gCfg;

uint8_t g_signer_ca_public_key[64];
uint8_t g_signer_public_key[64];
uint8_t g_device_public_key[64];

uint8_t g_signer_cert_ref[512];
size_t g_signer_cert_ref_size = 0;

uint8_t g_device_cert_ref[512];
size_t g_device_cert_ref_size = 0;

static void build_and_save_cert(
    const atcacert_def_t* cert_def,
    uint8_t*              cert,
    size_t*               cert_size,
    const uint8_t ca_public_key[64],
    const uint8_t public_key[64],
    const uint8_t signer_id[2],
    const atcacert_tm_utc_t* issue_date,
    const uint8_t config32[32],
    uint8_t ca_slot)
{
	int ret;
	atcacert_build_state_t build_state;
	uint8_t tbs_digest[32];
	uint8_t signature[64];
	size_t max_cert_size = *cert_size;
	atcacert_tm_utc_t expire_date = {
		.tm_year	= issue_date->tm_year + cert_def->expire_years,
		.tm_mon		= issue_date->tm_mon,
		.tm_mday	= issue_date->tm_mday,
		.tm_hour	= issue_date->tm_hour,
		.tm_min		= 0,
		.tm_sec		= 0
	};
	const atcacert_device_loc_t config32_dev_loc = {
		.zone	= DEVZONE_CONFIG,
		.offset = 0,
		.count	= 32
	};
	atcacert_device_loc_t device_locs[4];
	size_t device_locs_count = 0;
	size_t i;

	if (cert_def->expire_years == 0) {
		ret = atcacert_date_get_max_date(cert_def->expire_date_format, &expire_date);
		TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	}

	ret = atcacert_cert_build_start(&build_state, cert_def, cert, cert_size, ca_public_key);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);

	ret = atcacert_set_subj_public_key(build_state.cert_def, build_state.cert, *build_state.cert_size, public_key);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	ret = atcacert_set_issue_date(build_state.cert_def, build_state.cert, *build_state.cert_size, issue_date);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	ret = atcacert_set_expire_date(build_state.cert_def, build_state.cert, *build_state.cert_size, &expire_date);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	ret = atcacert_set_signer_id(build_state.cert_def, build_state.cert, *build_state.cert_size, signer_id);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	ret = atcacert_cert_build_process(&build_state, &config32_dev_loc, config32);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);

	ret = atcacert_cert_build_finish(&build_state);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);

	ret = atcacert_get_tbs_digest(build_state.cert_def, build_state.cert, *build_state.cert_size, tbs_digest);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);

	ret = atcab_sign(ca_slot, tbs_digest, signature);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);

	ret = atcacert_set_signature(cert_def, cert, cert_size, max_cert_size, signature);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);

	ret = atcacert_get_device_locs(cert_def, device_locs, &device_locs_count, sizeof(device_locs) / sizeof(device_locs[0]), 32);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);

	for (i = 0; i < device_locs_count; i++) {
		size_t end_block;
		size_t start_block;
		uint8_t data[96];
		uint8_t block;

		if (device_locs[i].zone == DEVZONE_CONFIG)
			continue;
		if (device_locs[i].zone == DEVZONE_DATA && device_locs[i].is_genkey)
			continue;

		TEST_ASSERT(sizeof(data) >= device_locs[i].count);

		ret = atcacert_get_device_data(cert_def, cert, *cert_size, &device_locs[i], data);
		TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);

		start_block = device_locs[i].offset / 32;
		end_block = (device_locs[i].offset + device_locs[i].count) / 32;
		for (block = (uint8_t)start_block; block < end_block; block++) {
			ret = atcab_write_zone(device_locs[i].zone, device_locs[i].slot, block, 0, &data[(block - start_block) * 32], 32);
			TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
		}
	}
}

TEST_GROUP(atcacert_client);

TEST_SETUP(atcacert_client)
{
	int ret = 0;
	bool lockstate = 0;

	ret = atcab_init(gCfg);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);

	ret = atcab_is_locked(LOCK_ZONE_CONFIG, &lockstate);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	if (!lockstate)
		TEST_IGNORE_MESSAGE("Config zone must be locked for this test.");

	ret = atcab_is_locked(LOCK_ZONE_DATA, &lockstate);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	if (!lockstate)
		TEST_IGNORE_MESSAGE("Data zone must be locked for this test.");
}

TEST_TEAR_DOWN(atcacert_client)
{
	atcab_release();
}

TEST(atcacert_client, atcacert_client__init)
{
	int ret = 0;

	static const uint8_t signer_ca_private_key_slot = 7;
	static const uint8_t signer_private_key_slot = 2;
	uint8_t signer_id[2] = { 0xC4, 0x8B };
	const atcacert_tm_utc_t signer_issue_date = {
		.tm_year	= 2014 - 1900,
		.tm_mon		= 8 - 1,
		.tm_mday	= 2,
		.tm_hour	= 20,
		.tm_min		= 0,
		.tm_sec		= 0
	};
	static const uint8_t device_private_key_slot = 0;
	const atcacert_tm_utc_t device_issue_date = {
		.tm_year	= 2015 - 1900,
		.tm_mon		= 9 - 1,
		.tm_mday	= 3,
		.tm_hour	= 21,
		.tm_min		= 0,
		.tm_sec		= 0
	};
	uint8_t config32[32];
	char disp_str[1500];
	int disp_size = sizeof(disp_str);


	ret = atcab_read_zone(ATCA_ZONE_CONFIG, 0, 0, 0, config32, 32);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);

	ret = atcab_genkey(signer_ca_private_key_slot, g_signer_ca_public_key);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	disp_size = sizeof(disp_str);
	atcab_bin2hex( g_signer_ca_public_key, ATCA_PUB_KEY_SIZE, disp_str, &disp_size);
	printf("Signer CA Public Key:\r\n%s\r\n", disp_str);

	ret = atcab_genkey(signer_private_key_slot, g_signer_public_key);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	disp_size = sizeof(disp_str);
	atcab_bin2hex( g_signer_public_key, ATCA_PUB_KEY_SIZE, disp_str, &disp_size);
	printf("Signer Public Key:\r\n%s\r\n", disp_str);

	ret = atcab_genkey(device_private_key_slot, g_device_public_key);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	disp_size = sizeof(disp_str);
	atcab_bin2hex( g_device_public_key, ATCA_PUB_KEY_SIZE, disp_str, &disp_size);
	printf("Device Public Key:\r\n%s\r\n", disp_str);

	// Build signer cert
	g_signer_cert_ref_size = sizeof(g_signer_cert_ref);
	build_and_save_cert(
	    &g_test_cert_def_1_signer,
	    g_signer_cert_ref,
	    &g_signer_cert_ref_size,
	    g_signer_ca_public_key,
	    g_signer_public_key,
	    signer_id,
	    &signer_issue_date,
	    config32,
	    signer_ca_private_key_slot);
	disp_size = sizeof(disp_str);
	atcab_bin2hex( g_signer_cert_ref, (int)g_signer_cert_ref_size, disp_str, &disp_size);
	printf("Signer Certificate:\r\n%s\r\n", disp_str);

	g_device_cert_ref_size = sizeof(g_device_cert_ref);
	build_and_save_cert(
	    &g_test_cert_def_0_device,
	    g_device_cert_ref,
	    &g_device_cert_ref_size,
	    g_signer_public_key,
	    g_device_public_key,
	    signer_id,
	    &device_issue_date,
	    config32,
	    signer_private_key_slot);
	disp_size = sizeof(disp_str);
	atcab_bin2hex( g_device_cert_ref, (int)g_device_cert_ref_size, disp_str, &disp_size);
	printf("Device Certificate:\r\n%s\r\n", disp_str);
}

TEST(atcacert_client, atcacert_client__atcacert_read_cert_signer)
{
	int ret = 0;
	uint8_t cert[512];
	size_t cert_size = sizeof(cert);

	ret = atcacert_read_cert(&g_test_cert_def_1_signer, g_signer_ca_public_key, cert, &cert_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(g_signer_cert_ref_size, cert_size);
	TEST_ASSERT_EQUAL_MEMORY(g_signer_cert_ref, cert, cert_size);
}

TEST(atcacert_client, atcacert_client__atcacert_read_cert_device)
{
	int ret = 0;
	uint8_t cert[512];
	size_t cert_size = sizeof(cert);

	ret = atcacert_read_cert(&g_test_cert_def_0_device, g_signer_public_key, cert, &cert_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(g_device_cert_ref_size, cert_size);
	TEST_ASSERT_EQUAL_MEMORY(g_device_cert_ref, cert, cert_size);
}

TEST(atcacert_client, atcacert_client__atcacert_read_cert_small_buf)
{
	int ret = 0;
	uint8_t cert[64];
	size_t cert_size = sizeof(cert);

	ret = atcacert_read_cert(&g_test_cert_def_1_signer, g_signer_ca_public_key, cert, &cert_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BUFFER_TOO_SMALL, ret);
}

TEST(atcacert_client, atcacert_client__atcacert_read_cert_bad_params)
{
	int ret = 0;
	uint8_t cert[128];
	size_t cert_size = sizeof(cert);

	ret = atcacert_read_cert(NULL, g_signer_public_key, cert, &cert_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_read_cert(NULL, NULL, cert, &cert_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_read_cert(&g_test_cert_def_0_device, g_signer_public_key, NULL, &cert_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_read_cert(NULL, g_signer_public_key, NULL, &cert_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_read_cert(&g_test_cert_def_0_device, NULL, NULL, &cert_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_read_cert(NULL, NULL, NULL, &cert_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_read_cert(&g_test_cert_def_0_device, g_signer_public_key, cert, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_read_cert(NULL, g_signer_public_key, cert, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_read_cert(&g_test_cert_def_0_device, NULL, cert, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_read_cert(NULL, NULL, cert, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_read_cert(&g_test_cert_def_0_device, g_signer_public_key, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_read_cert(NULL, g_signer_public_key, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_read_cert(&g_test_cert_def_0_device, NULL, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_read_cert(NULL, NULL, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}

TEST(atcacert_client, atcacert_client__atcacert_get_response)
{
	int ret = 0;
	uint8_t response[64];
	bool is_verified = false;
	const uint8_t challenge[32] = {
		0x0c, 0xa6, 0x34, 0xc8, 0x37, 0x2f, 0x87, 0x99, 0x99, 0x7e, 0x9e, 0xe9, 0xd5, 0xbc, 0x72, 0x71,
		0x84, 0xd1, 0x97, 0x0a, 0xea, 0xfe, 0xac, 0x60, 0x7e, 0xd1, 0x3e, 0x12, 0xb7, 0x32, 0x25, 0xf1
	};
	char disp_str[256];
	int disp_size = sizeof(disp_str);

	ret = atcacert_get_response(0, challenge, response);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);

	ret = atcab_verify_extern(challenge, response, g_device_public_key, &is_verified);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	TEST_ASSERT(is_verified);

	disp_size = sizeof(disp_str);
	atcab_bin2hex( challenge, sizeof(challenge), disp_str, &disp_size);
	printf("Challenge:\r\n%s\r\n", disp_str);
	disp_size = sizeof(disp_str);
	atcab_bin2hex( response, sizeof(response), disp_str, &disp_size);
	printf("Response:\r\n%s\r\n", disp_str);
	disp_size = sizeof(disp_str);
	atcab_bin2hex( g_device_public_key, sizeof(g_device_public_key), disp_str, &disp_size);
	printf("Public Key:\r\n%s\r\n", disp_str);
}

TEST(atcacert_client, atcacert_client__atcacert_get_response_bad_params)
{
	int ret = 0;
	uint8_t response[64];
	const uint8_t challenge[32] = {
		0x0c, 0xa6, 0x34, 0xc8, 0x37, 0x2f, 0x87, 0x99, 0x99, 0x7e, 0x9e, 0xe9, 0xd5, 0xbc, 0x72, 0x71,
		0x84, 0xd1, 0x97, 0x0a, 0xea, 0xfe, 0xac, 0x60, 0x7e, 0xd1, 0x3e, 0x12, 0xb7, 0x32, 0x25, 0xf1
	};

	ret = atcacert_get_response(16, challenge, response);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_get_response(0, NULL, response);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_get_response(16, NULL, response);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_get_response(0, challenge, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_get_response(16, challenge, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_get_response(0, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_get_response(16, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}