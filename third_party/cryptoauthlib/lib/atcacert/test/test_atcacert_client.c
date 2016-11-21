/** \brief cert client tests
*
* Copyright (c) 2015 Atmel Corporation. All rights reserved.
*
* \asf_license_start
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
*    Atmel microcontroller product.
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
* \asf_license_stop
 */ 


#include "atcacert/atcacert_client.h"
#include "test/unity.h"
#include "test/unity_fixture.h"
#include <string.h>
#include "cryptoauthlib.h"
#include "basic/atca_basic.h"
#include "crypto/atca_crypto_sw_sha2.h"

static const uint8_t g_signer_cert_def_cert_template[] = {
	0x30, 0x82, 0x01, 0xB1, 0x30, 0x82, 0x01, 0x57, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x03, 0x40,
	0x01, 0x02, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x36,
	0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x07, 0x45, 0x78, 0x61, 0x6D, 0x70,
	0x6C, 0x65, 0x31, 0x22, 0x30, 0x20, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x19, 0x45, 0x78, 0x61,
	0x6D, 0x70, 0x6C, 0x65, 0x20, 0x41, 0x54, 0x45, 0x43, 0x43, 0x35, 0x30, 0x38, 0x41, 0x20, 0x52,
	0x6F, 0x6F, 0x74, 0x20, 0x43, 0x41, 0x30, 0x1E, 0x17, 0x0D, 0x31, 0x35, 0x30, 0x37, 0x33, 0x31,
	0x30, 0x30, 0x31, 0x32, 0x31, 0x35, 0x5A, 0x17, 0x0D, 0x33, 0x35, 0x30, 0x37, 0x33, 0x31, 0x30,
	0x30, 0x31, 0x32, 0x31, 0x35, 0x5A, 0x30, 0x3A, 0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04,
	0x0A, 0x0C, 0x07, 0x45, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x31, 0x26, 0x30, 0x24, 0x06, 0x03,
	0x55, 0x04, 0x03, 0x0C, 0x1D, 0x45, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x20, 0x41, 0x54, 0x45,
	0x43, 0x43, 0x35, 0x30, 0x38, 0x41, 0x20, 0x53, 0x69, 0x67, 0x6E, 0x65, 0x72, 0x20, 0x58, 0x58,
	0x58, 0x58, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06,
	0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0xF8, 0x0D, 0x8B,
	0x65, 0xE8, 0xBC, 0xCE, 0x14, 0x76, 0xE1, 0x8D, 0x05, 0xE2, 0x01, 0x69, 0x3B, 0xA2, 0xA6, 0x59,
	0xCF, 0xB9, 0xFD, 0x95, 0xE7, 0xBA, 0xD0, 0x21, 0x77, 0xF1, 0x38, 0x76, 0x1B, 0x34, 0xF1, 0xB3,
	0x58, 0x95, 0xA1, 0x35, 0x0D, 0x94, 0x82, 0x47, 0xE5, 0x23, 0x6F, 0xB3, 0x92, 0x01, 0x51, 0xD1,
	0x3A, 0x6F, 0x01, 0x23, 0xD6, 0x70, 0xB5, 0xE5, 0x0C, 0xE0, 0xFF, 0x49, 0x31, 0xA3, 0x50, 0x30,
	0x4E, 0x30, 0x0C, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xFF, 0x30,
	0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0x1F, 0xAF, 0x8F, 0x90, 0x86, 0x5F,
	0x7D, 0xD2, 0x26, 0xB0, 0x6F, 0xE3, 0x20, 0x4E, 0x48, 0xA5, 0xD2, 0x94, 0x65, 0xE2, 0x30, 0x1F,
	0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x77, 0x23, 0xA2, 0xC4, 0x32,
	0xA6, 0x94, 0x1D, 0x81, 0x32, 0xCB, 0x76, 0x04, 0xC3, 0x80, 0x1D, 0xD2, 0xBE, 0x95, 0x5D, 0x30,
	0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x03, 0x48, 0x00, 0x30, 0x45,
	0x02, 0x20, 0x43, 0x90, 0xCD, 0x89, 0xE0, 0x75, 0xD0, 0x45, 0x93, 0x7B, 0x37, 0x3F, 0x52, 0x6F,
	0xF6, 0x5C, 0x4B, 0x4C, 0xCA, 0x7C, 0x61, 0x3C, 0x5F, 0x9C, 0xF2, 0xF4, 0xC9, 0xE7, 0xCE, 0xDF,
	0x24, 0xAA, 0x02, 0x21, 0x00, 0x89, 0x52, 0x36, 0xF3, 0xC3, 0x7C, 0xD7, 0x9D, 0x5C, 0x43, 0xF4,
	0xA9, 0x1B, 0xB3, 0xB1, 0xC7, 0x3E, 0xB2, 0x66, 0x74, 0x6C, 0x20, 0x53, 0x0A, 0x3B, 0x90, 0x77,
	0x6C, 0xA9, 0xC7, 0x79, 0x0D
};

static const uint8_t g_device_cert_def_cert_template[] = {
	0x30, 0x82, 0x01, 0x8A, 0x30, 0x82, 0x01, 0x30, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x0A, 0x40,
	0x01, 0x23, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0xEE, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48,
	0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x3A, 0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04, 0x0A,
	0x0C, 0x07, 0x45, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x31, 0x26, 0x30, 0x24, 0x06, 0x03, 0x55,
	0x04, 0x03, 0x0C, 0x1D, 0x45, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x20, 0x41, 0x54, 0x45, 0x43,
	0x43, 0x35, 0x30, 0x38, 0x41, 0x20, 0x53, 0x69, 0x67, 0x6E, 0x65, 0x72, 0x20, 0x58, 0x58, 0x58,
	0x58, 0x30, 0x1E, 0x17, 0x0D, 0x31, 0x35, 0x30, 0x37, 0x33, 0x31, 0x30, 0x30, 0x31, 0x32, 0x31,
	0x36, 0x5A, 0x17, 0x0D, 0x33, 0x35, 0x30, 0x37, 0x33, 0x31, 0x30, 0x30, 0x31, 0x32, 0x31, 0x36,
	0x5A, 0x30, 0x35, 0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x07, 0x45, 0x78,
	0x61, 0x6D, 0x70, 0x6C, 0x65, 0x31, 0x21, 0x30, 0x1F, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x18,
	0x45, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x20, 0x41, 0x54, 0x45, 0x43, 0x43, 0x35, 0x30, 0x38,
	0x41, 0x20, 0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86,
	0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03,
	0x42, 0x00, 0x04, 0xC3, 0xDC, 0x86, 0xE9, 0xCC, 0x59, 0xA1, 0xFA, 0xF8, 0xE6, 0x02, 0xB3, 0x44,
	0x89, 0xD1, 0x70, 0x4A, 0x3B, 0x44, 0x04, 0x52, 0xAA, 0x11, 0x93, 0x35, 0xA9, 0xBE, 0x6F, 0x68,
	0x32, 0xDC, 0x59, 0xCE, 0x5E, 0x74, 0x73, 0xB8, 0x44, 0xBD, 0x08, 0x4D, 0x5D, 0x3D, 0xE5, 0xDE,
	0x21, 0xC3, 0x4F, 0x8D, 0xC1, 0x61, 0x4F, 0x17, 0x27, 0xAF, 0x6D, 0xC4, 0x9C, 0x42, 0x83, 0xEE,
	0x36, 0xE2, 0x31, 0xA3, 0x23, 0x30, 0x21, 0x30, 0x1F, 0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18,
	0x30, 0x16, 0x80, 0x14, 0x1F, 0xAF, 0x8F, 0x90, 0x86, 0x5F, 0x7D, 0xD2, 0x26, 0xB0, 0x6F, 0xE3,
	0x20, 0x4E, 0x48, 0xA5, 0xD2, 0x94, 0x65, 0xE2, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE,
	0x3D, 0x04, 0x03, 0x02, 0x03, 0x48, 0x00, 0x30, 0x45, 0x02, 0x20, 0x5E, 0x13, 0x59, 0x05, 0x08,
	0xDA, 0x03, 0xFD, 0x94, 0x1B, 0xAF, 0xEF, 0x8A, 0x3D, 0xC8, 0x24, 0xE5, 0x49, 0x07, 0xB8, 0xA2,
	0xBD, 0x36, 0x60, 0x26, 0x14, 0x75, 0x27, 0x36, 0x66, 0xE1, 0xAA, 0x02, 0x21, 0x00, 0x96, 0xFF,
	0x2B, 0xDF, 0x34, 0x54, 0x9B, 0x7C, 0x56, 0x8F, 0x54, 0x44, 0x4F, 0xE6, 0xAD, 0x3B, 0xFE, 0x63,
	0xBD, 0xD2, 0x93, 0x65, 0xF2, 0x65, 0x59, 0x22, 0xC6, 0x25, 0x90, 0x7A, 0xEC, 0x19
};

static const atcacert_def_t g_signer_cert_def = {
	.type = CERTTYPE_X509,
	.template_id = 1,
	.chain_id = 0,
	.private_key_slot = 0,
	.sn_source = SNSRC_SIGNER_ID,
	.cert_sn_dev_loc = {
		.zone = DEVZONE_NONE,
		.slot = 0,
		.is_genkey = 0,
		.offset = 0,
		.count = 0
	},
	.issue_date_format = DATEFMT_RFC5280_UTC,
	.expire_date_format = DATEFMT_RFC5280_UTC,
	.tbs_cert_loc = {
		.offset = 4,
		.count = 347
	},
	.expire_years = 20,
	.public_key_dev_loc = {
		.zone = DEVZONE_DATA,
		.slot = 11,
		.is_genkey = 0,
		.offset = 0,
		.count = 72
	},
	.comp_cert_dev_loc = {
		.zone = DEVZONE_DATA,
		.slot = 12,
		.is_genkey = 0,
		.offset = 0,
		.count = 72
	},
	.std_cert_elements = {
		{ // STDCERT_PUBLIC_KEY
			.offset = 205,
			.count = 64
		},
		{ // STDCERT_SIGNATURE
			.offset = 363,
			.count = 73
		},
		{ // STDCERT_ISSUE_DATE
			.offset = 90,
			.count = 13
		},
		{ // STDCERT_EXPIRE_DATE
			.offset = 105,
			.count = 13
		},
		{ // STDCERT_SIGNER_ID
			.offset = 174,
			.count = 4
		},
		{ // STDCERT_CERT_SN
			.offset = 15,
			.count = 3
		},
		{ // STDCERT_AUTH_KEY_ID
			.offset = 331,
			.count = 20
		},
		{ // STDCERT_SUBJ_KEY_ID
			.offset = 298,
			.count = 20
		}
	},
	.cert_elements = NULL,
	.cert_elements_count = 0,
	.cert_template = g_signer_cert_def_cert_template,
	.cert_template_size = sizeof(g_signer_cert_def_cert_template)
};

static const atcacert_def_t g_device_cert_def = {
	.type = CERTTYPE_X509,
	.template_id = 0,
	.chain_id = 0,
	.private_key_slot = 0,
	.sn_source = SNSRC_DEVICE_SN,
	.cert_sn_dev_loc = {
		.zone = DEVZONE_NONE,
		.slot = 0,
		.is_genkey = 0,
		.offset = 0,
		.count = 0
	},
	.issue_date_format = DATEFMT_RFC5280_UTC,
	.expire_date_format = DATEFMT_RFC5280_UTC,
	.tbs_cert_loc = {
		.offset = 4,
		.count = 308
	},
	.expire_years = 20,
	.public_key_dev_loc = {
		.zone = DEVZONE_DATA,
		.slot = 0,
		.is_genkey = 1,
		.offset = 0,
		.count = 64
	},
	.comp_cert_dev_loc = {
		.zone = DEVZONE_DATA,
		.slot = 10,
		.is_genkey = 0,
		.offset = 0,
		.count = 72
	},
	.std_cert_elements = {
		{ // STDCERT_PUBLIC_KEY
			.offset = 211,
			.count = 64
		},
		{ // STDCERT_SIGNATURE
			.offset = 324,
			.count = 73
		},
		{ // STDCERT_ISSUE_DATE
			.offset = 101,
			.count = 13
		},
		{ // STDCERT_EXPIRE_DATE
			.offset = 116,
			.count = 13
		},
		{ // STDCERT_SIGNER_ID
			.offset = 93,
			.count = 4
		},
		{ // STDCERT_CERT_SN
			.offset = 15,
			.count = 10
		},
		{ // STDCERT_AUTH_KEY_ID
			.offset = 292,
			.count = 20
		},
		{ // STDCERT_SUBJ_KEY_ID
			.offset = 0,
			.count = 0
		}
	},
	.cert_elements = NULL,
	.cert_elements_count = 0,
	.cert_template = g_device_cert_def_cert_template,
	.cert_template_size = sizeof(g_device_cert_def_cert_template)
};

atcacert_def_t g_cert_def;

uint8_t g_signer_ca_public_key[64];
uint8_t g_signer_public_key[64];
uint8_t g_device_public_key[64];

uint8_t g_signer_cert_ref[512];
size_t  g_signer_cert_ref_size = 0;

uint8_t g_device_cert_ref[512];
size_t  g_device_cert_ref_size = 0;

static void build_and_save_cert(
    const atcacert_def_t* cert_def,
    uint8_t*              cert,
    size_t*               cert_size,
    const uint8_t         ca_public_key[64],
    const uint8_t         public_key[64],
    const uint8_t         signer_id[2],
    const struct tm*      issue_date,
    const uint8_t         config32[32],
    uint8_t               ca_slot)
{
    int ret;
    atcacert_build_state_t build_state;
    uint8_t tbs_digest[32];
    uint8_t signature[64];
    size_t max_cert_size = *cert_size;
    const struct tm expire_date = {
        .tm_year = issue_date->tm_year + cert_def->expire_years,
        .tm_mon  = issue_date->tm_mon,
        .tm_mday = issue_date->tm_mday,
        .tm_hour = issue_date->tm_hour,
        .tm_min  = 0,
        .tm_sec  = 0
    };
    const atcacert_device_loc_t config32_dev_loc = {
        .zone = DEVZONE_CONFIG,
        .offset = 0,
        .count = 32
    };
    atcacert_device_loc_t device_locs[4];
    size_t device_locs_count = 0;
    size_t i;
    
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
    
    for (i = 0; i < device_locs_count; i++)
    {
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
        for (block = start_block; block < end_block; block++)
        {
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
	#ifdef ATCA_HAL_I2C
    atcab_init( &cfg_ateccx08a_i2c_default );
    #elif ATCA_HAL_SWI
	atcab_init( &cfg_ateccx08a_swi_default );
	#endif
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
    uint8_t signer_id[2] = {0xC4, 0x8B};
    const struct tm signer_issue_date = {
        .tm_year = 2014 - 1900,
        .tm_mon  = 8 - 1,
        .tm_mday = 2,
        .tm_hour = 20,
        .tm_min  = 0,
        .tm_sec  = 0
    };
    static const uint8_t device_private_key_slot = 0;
    const struct tm device_issue_date = {
        .tm_year = 2015 - 1900,
        .tm_mon  = 9 - 1,
        .tm_mday = 3,
        .tm_hour = 21,
        .tm_min  = 0,
        .tm_sec  = 0
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
        &g_signer_cert_def,
        g_signer_cert_ref,
        &g_signer_cert_ref_size,
        g_signer_ca_public_key,
        g_signer_public_key,
        signer_id,
        &signer_issue_date,
        config32,
        signer_ca_private_key_slot);
    disp_size = sizeof(disp_str);
    atcab_bin2hex( g_signer_cert_ref, g_signer_cert_ref_size, disp_str, &disp_size);
    printf("Signer Certificate:\r\n%s\r\n", disp_str);
        
    g_device_cert_ref_size = sizeof(g_device_cert_ref);
    build_and_save_cert(
        &g_device_cert_def,
        g_device_cert_ref,
        &g_device_cert_ref_size,
        g_signer_public_key,
        g_device_public_key,
        signer_id,
        &device_issue_date,
        config32,
        signer_private_key_slot);
    disp_size = sizeof(disp_str);
    atcab_bin2hex( g_device_cert_ref, g_device_cert_ref_size, disp_str, &disp_size);
    printf("Device Certificate:\r\n%s\r\n", disp_str);
}

TEST(atcacert_client, atcacert_client__atcacert_read_cert_signer)
{
	int ret = 0;
	uint8_t cert[1024];
	size_t  cert_size = sizeof(cert);
    
	ret = atcacert_read_cert(&g_signer_cert_def, g_signer_ca_public_key, cert, &cert_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(g_signer_cert_ref_size, cert_size);
	TEST_ASSERT_EQUAL_MEMORY(g_signer_cert_ref, cert, cert_size);
}

TEST(atcacert_client, atcacert_client__atcacert_read_cert_device)
{
    int ret = 0;
    uint8_t cert[1024];
    size_t  cert_size = sizeof(cert);
    
    ret = atcacert_read_cert(&g_device_cert_def, g_signer_public_key, cert, &cert_size);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL(g_device_cert_ref_size, cert_size);
    TEST_ASSERT_EQUAL_MEMORY(g_device_cert_ref, cert, cert_size);
}

TEST(atcacert_client, atcacert_client__atcacert_read_cert_small_buf)
{
    int ret = 0;
    uint8_t cert[64];
    size_t  cert_size = sizeof(cert);
    
    ret = atcacert_read_cert(&g_signer_cert_def, g_signer_ca_public_key, cert, &cert_size);
    TEST_ASSERT_EQUAL(ATCACERT_E_BUFFER_TOO_SMALL, ret);
}

TEST(atcacert_client, atcacert_client__atcacert_read_cert_bad_params)
{
    int ret = 0;
    uint8_t cert[128];
    size_t  cert_size = sizeof(cert);
    
    ret = atcacert_read_cert(NULL, g_signer_public_key, cert, &cert_size);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
    
    ret = atcacert_read_cert(&g_device_cert_def, NULL, cert, &cert_size);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
    
    ret = atcacert_read_cert(NULL, NULL, cert, &cert_size);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
    
    ret = atcacert_read_cert(&g_device_cert_def, g_signer_public_key, NULL, &cert_size);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
    
    ret = atcacert_read_cert(NULL, g_signer_public_key, NULL, &cert_size);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
    
    ret = atcacert_read_cert(&g_device_cert_def, NULL, NULL, &cert_size);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
    
    ret = atcacert_read_cert(NULL, NULL, NULL, &cert_size);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
    
    ret = atcacert_read_cert(&g_device_cert_def, g_signer_public_key, cert, NULL);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
    
    ret = atcacert_read_cert(NULL, g_signer_public_key, cert, NULL);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
    
    ret = atcacert_read_cert(&g_device_cert_def, NULL, cert, NULL);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
    
    ret = atcacert_read_cert(NULL, NULL, cert, NULL);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
    
    ret = atcacert_read_cert(&g_device_cert_def, g_signer_public_key, NULL, NULL);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
    
    ret = atcacert_read_cert(NULL, g_signer_public_key, NULL, NULL);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
    
    ret = atcacert_read_cert(&g_device_cert_def, NULL, NULL, NULL);
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
        0x84, 0xd1, 0x97, 0x0a, 0xea, 0xfe, 0xac, 0x60, 0x7e, 0xd1, 0x3e, 0x12, 0xb7, 0x32, 0x25, 0xf1 };
    char disp_str[256];
    int  disp_size = sizeof(disp_str);
    
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
        0x84, 0xd1, 0x97, 0x0a, 0xea, 0xfe, 0xac, 0x60, 0x7e, 0xd1, 0x3e, 0x12, 0xb7, 0x32, 0x25, 0xf1 };
    
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