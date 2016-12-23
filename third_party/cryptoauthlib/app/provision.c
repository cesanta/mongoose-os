/** \file provision.c
* \brief provisioning phase of example
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


#include "provision.h"
#include "cert_def_1_signer.h"
#include "cert_def_2_device.h"
#include "basic/atca_basic.h"
#include <stdio.h>

/** \defgroup auth Node authentication stages for node-auth-basic example
 *
@{ */

// modified W25 ECC508 configuration (slot 7 has external sign turned on for cert testing)
static const uint8_t g_ecc_configdata[128] = {
    0x01, 0x23, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00,  0x04, 0x05, 0x06, 0x07, 0xEE, 0x00, 0x01, 0x00,
    D_I2C, 0x00, 0x55, 0x00, 0x8F, 0x20, 0xC4, 0x44,  0x87, 0x20, 0xC4, 0x44, 0x8F, 0x0F, 0x8F, 0x8F,
    0x9F, 0x8F, 0x83, 0x64, 0xC4, 0x44, 0xC4, 0x44,  0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF,  0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,  0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x33, 0x00, 0x1C, 0x00, 0x13, 0x00, 0x1C, 0x00,  0x3C, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0x33, 0x00,
    0x1C, 0x00, 0x1C, 0x00, 0x3C, 0x00, 0x3C, 0x00,  0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00 };

static const uint8_t g_signer_ca_private_key[36] = {
	0x00, 0x00, 0x00, 0x00,
    0x49, 0x0c, 0x1e, 0x09, 0xe2, 0x40, 0xaf, 0x00, 0xe9, 0x6b, 0x43, 0x32, 0x92, 0x0e, 0x15, 0x8f,
    0x69, 0x58, 0x8e, 0xd4, 0x25, 0xc7, 0xf6, 0x8b, 0x0c, 0x6a, 0x52, 0x5d, 0x0d, 0x21, 0x4f, 0xee };
	
uint8_t g_signer_ca_public_key[64];

/** \brief example of building a save a cert in the ATECC508A.  Normally this would be done during
 * production and provisioning at a factory and therefore things like the root CA public key and
 * other attributes would be coming from the server and HSM.   This is a functional piece of code
 * which emulates that process for illustration, example, and demonstration purposes.
*/
static int build_and_save_cert(
    const atcacert_def_t*    cert_def,
    uint8_t*                 cert,
    size_t*                  cert_size,
    const uint8_t            ca_public_key[64],
    const uint8_t            public_key[64],
    const uint8_t            signer_id[2],
    const atcacert_tm_utc_t* issue_date,
    const uint8_t            config32[32],
    uint8_t                  ca_slot)
{
    int ret;
    atcacert_build_state_t build_state;
    uint8_t tbs_digest[32];
    uint8_t signature[64];
    size_t max_cert_size = *cert_size;
    atcacert_tm_utc_t expire_date = {
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
    
    if (cert_def->expire_years == 0)
    {
        ret = atcacert_date_get_max_date(cert_def->expire_date_format, &expire_date);
        if (ret != ATCACERT_E_SUCCESS) return ret;
    }
    
    ret = atcacert_cert_build_start(&build_state, cert_def, cert, cert_size, ca_public_key);
    if (ret != ATCACERT_E_SUCCESS) return ret;
    
    ret = atcacert_set_subj_public_key(build_state.cert_def, build_state.cert, *build_state.cert_size, public_key);
    if (ret != ATCACERT_E_SUCCESS) return ret;
    ret = atcacert_set_issue_date(build_state.cert_def, build_state.cert, *build_state.cert_size, issue_date);
    if (ret != ATCACERT_E_SUCCESS) return ret;
    ret = atcacert_set_expire_date(build_state.cert_def, build_state.cert, *build_state.cert_size, &expire_date);
    if (ret != ATCACERT_E_SUCCESS) return ret;
    ret = atcacert_set_signer_id(build_state.cert_def, build_state.cert, *build_state.cert_size, signer_id);
    if (ret != ATCACERT_E_SUCCESS) return ret;
    ret = atcacert_cert_build_process(&build_state, &config32_dev_loc, config32);
    if (ret != ATCACERT_E_SUCCESS) return ret;
    
    ret = atcacert_cert_build_finish(&build_state);
    if (ret != ATCACERT_E_SUCCESS) return ret;
    
    ret = atcacert_get_tbs_digest(build_state.cert_def, build_state.cert, *build_state.cert_size, tbs_digest);
    if (ret != ATCACERT_E_SUCCESS) return ret;
    
    ret = atcab_sign(ca_slot, tbs_digest, signature);
    if (ret != ATCA_SUCCESS) return ret;
    
    ret = atcacert_set_signature(cert_def, cert, cert_size, max_cert_size, signature);
    if (ret != ATCACERT_E_SUCCESS) return ret;
    
    ret = atcacert_get_device_locs(cert_def, device_locs, &device_locs_count, sizeof(device_locs) / sizeof(device_locs[0]), 32);
    if (ret != ATCACERT_E_SUCCESS) return ret;
    
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
        
        ret = atcacert_get_device_data(cert_def, cert, *cert_size, &device_locs[i], data);
        if (ret != ATCACERT_E_SUCCESS) return ret;
        
        start_block = device_locs[i].offset / 32;
        end_block = (device_locs[i].offset + device_locs[i].count) / 32;
        for (block = start_block; block < end_block; block++)
        {
            ret = atcab_write_zone(device_locs[i].zone, device_locs[i].slot, block, 0, &data[(block - start_block) * 32], 32);
            if (ret != ATCA_SUCCESS) return ret;
        }
    }
    
    return 0;
}

/** \brief entry point called from the console interaction to provision the client side certs and device key.  This is
 * a self-contained example of the provisioning process that would normally happen at the factory using high security
 * modules (HSM) and production facilities.  This is functional code which can be used to self-provisioin a device for
 * illustration, example, or demo purposes.
 */

int client_provision(void)
{
    int ret = 0;
    bool lockstate = 0;
    uint8_t signer_ca_private_key_slot = 7;
    uint8_t signer_private_key_slot = 2;
    uint8_t signer_id[2] = {0xC4, 0x8B};
    const atcacert_tm_utc_t signer_issue_date = {
        .tm_year = 2014 - 1900,
        .tm_mon  = 8 - 1,
        .tm_mday = 2,
        .tm_hour = 20,
        .tm_min  = 0,
        .tm_sec  = 0
    };
    uint8_t device_private_key_slot = 0;
    const atcacert_tm_utc_t device_issue_date = {
        .tm_year = 2015 - 1900,
        .tm_mon  = 9 - 1,
        .tm_mday = 3,
        .tm_hour = 21,
        .tm_min  = 0,
        .tm_sec  = 0
    };
	static const uint8_t access_key_slot = 4;
	const uint8_t access_key[] = {
		0x32, 0x12, 0xd0, 0x66, 0xf5, 0xed, 0x52, 0xc7, 0x79, 0x98, 0xff, 0xaa, 0xac, 0x43, 0x22, 0x60,
		0xdd, 0xff, 0x9c, 0x10, 0x99, 0x6f, 0x41, 0x66, 0x3a, 0x60, 0x23, 0xfa, 0xf6, 0xaa, 0x3e, 0xc5
	};
    uint8_t config64[64];
	bool is_signer_ca_slot_ext_sig    = false;
	bool is_signer_ca_slot_priv_write = false;
    uint8_t lock_response;
    uint8_t signer_public_key[64];
    uint8_t device_public_key[64];
    uint8_t signer_cert_ref[512];
    size_t  signer_cert_ref_size = 0;
    uint8_t device_cert_ref[512];
    size_t  device_cert_ref_size = 0;
    
    atcab_sleep();
    
    ret = atcab_is_locked(LOCK_ZONE_CONFIG, &lockstate);
    if (ret != ATCA_SUCCESS) return ret;
    if (!lockstate)
    {
        ret = atcab_write_ecc_config_zone(g_ecc_configdata);
        if (ret != ATCA_SUCCESS) return ret;
        
        ret = atcab_lock_config_zone(&lock_response);
        if (ret != ATCA_SUCCESS) return ret;
    }

	// Read the first 64 bytes of the config zone to get the slot config at least
	ret = atcab_read_zone(ATCA_ZONE_CONFIG, 0, 0, 0, &config64[0], 32);
    if (ret != ATCA_SUCCESS) return ret;
	ret = atcab_read_zone(ATCA_ZONE_CONFIG, 0, 1, 0, &config64[32], 32);
    if (ret != ATCA_SUCCESS) return ret;
	
	is_signer_ca_slot_ext_sig    = (config64[20 + signer_ca_private_key_slot*2] & 0x01);
	is_signer_ca_slot_priv_write = (config64[20 + signer_ca_private_key_slot*2 + 1] & 0x40);
	
    ret = atcab_is_locked(LOCK_ZONE_DATA, &lockstate);
    if (ret != ATCA_SUCCESS) return ret;
    if (!lockstate)
    {
        ret = atcab_priv_write(signer_ca_private_key_slot, g_signer_ca_private_key, 0, NULL);
        if (ret != ATCA_SUCCESS) return ret;
		
		ret = atcab_write_zone(DEVZONE_DATA, access_key_slot, 0, 0, access_key, 32);
        if (ret != ATCA_SUCCESS) return ret;
        
        ret = atcab_lock_data_zone(&lock_response);
        if (ret != ATCA_SUCCESS) return ret;
    }
	else if (!is_signer_ca_slot_ext_sig)
	{
		// The signer CA slot can't perform external signs.
		// Use the signer slot for both. A little weird, but it lets the example run.
		signer_ca_private_key_slot = signer_private_key_slot;
	}
	else if (is_signer_ca_slot_priv_write)
	{
		//ret = atcab_priv_write(signer_ca_private_key_slot, g_signer_ca_private_key, access_key_slot, access_key);
		//if (ret != ATCA_SUCCESS) return printf("Warning: PrivWrite to slot %d failed. Example may still work though.\n", signer_ca_private_key_slot);
	}
    
	if (signer_ca_private_key_slot != signer_private_key_slot)
	{
		ret = atcab_get_pubkey(signer_ca_private_key_slot, g_signer_ca_public_key);
		if (ret == ATCA_EXECUTION_ERROR)
			ret = atcab_genkey(signer_ca_private_key_slot, g_signer_ca_public_key);
		if (ret != ATCA_SUCCESS) return ret;
	}
    
    ret = atcab_genkey(signer_private_key_slot, signer_public_key);
    if (ret != ATCA_SUCCESS) return ret;
	if (signer_ca_private_key_slot == signer_private_key_slot)
	{
		memcpy(g_signer_ca_public_key, signer_public_key, sizeof(g_signer_ca_public_key));
	}
        
    ret = atcab_genkey(device_private_key_slot, device_public_key);
    if (ret != ATCA_SUCCESS) return ret;
    
    // Build signer cert
    signer_cert_ref_size = sizeof(signer_cert_ref);
    ret = build_and_save_cert(
        &g_cert_def_1_signer,
        signer_cert_ref,
        &signer_cert_ref_size,
        g_signer_ca_public_key,
        signer_public_key,
        signer_id,
        &signer_issue_date,
        config64,
        signer_ca_private_key_slot);
    if (ret != ATCA_SUCCESS) return ret;
        
    device_cert_ref_size = sizeof(device_cert_ref);
    ret = build_and_save_cert(
        &g_cert_def_2_device,
        device_cert_ref,
        &device_cert_ref_size,
        signer_public_key,
        device_public_key,
        signer_id,
        &device_issue_date,
        config64,
        signer_private_key_slot);
    if (ret != ATCA_SUCCESS) return ret;
    
    return 0;
}

/** @} */
