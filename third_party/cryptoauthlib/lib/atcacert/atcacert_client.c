/**
 * \file
 * \brief Client side cert i/o methods. These declarations deal with the client-side, the node being authenticated,
 *        of the authentication process. It is assumed the client has an ECC CryptoAuthentication device
 *        (e.g. ATECC508A) and the certificates are stored on that device.
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


#include "atcacert_client.h"
#include "cryptoauthlib.h"
#include "basic/atca_basic.h"

int atcacert_read_cert( const atcacert_def_t* cert_def,
                        const uint8_t ca_public_key[64],
                        uint8_t*              cert,
                        size_t*               cert_size)
{
	int ret = 0;
	atcacert_device_loc_t device_locs[16];
	size_t device_locs_count = 0;
	size_t i = 0;
	atcacert_build_state_t build_state;

	if (cert_def == NULL || ca_public_key == NULL || cert == NULL || cert_size == NULL)
		return ATCACERT_E_BAD_PARAMS;

	ret = atcacert_get_device_locs(
	    cert_def,
	    device_locs,
	    &device_locs_count,
	    sizeof(device_locs) / sizeof(device_locs[0]),
	    32);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_cert_build_start(&build_state, cert_def, cert, cert_size, ca_public_key);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	for (i = 0; i < device_locs_count; i++) {
		uint8_t data[416];
		if (device_locs[i].zone == DEVZONE_DATA && device_locs[i].is_genkey) {
			ret = atcab_get_pubkey(device_locs[i].slot, data);
			if (ret != ATCA_SUCCESS)
				return ret;
		}else {
			size_t start_block = device_locs[i].offset / 32;
			uint8_t block;
			size_t end_block = (device_locs[i].offset + device_locs[i].count) / 32;
			for (block = (uint8_t)start_block; block < end_block; block++) {
				ret = atcab_read_zone(device_locs[i].zone, device_locs[i].slot, block, 0, &data[block * 32 - device_locs[i].offset], 32);
				if (ret != ATCA_SUCCESS)
					return ret;
			}
		}

		ret = atcacert_cert_build_process(&build_state, &device_locs[i], data);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
	}

	ret = atcacert_cert_build_finish(&build_state);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	return ATCACERT_E_SUCCESS;
}

int atcacert_get_response( uint8_t device_private_key_slot,
                           const uint8_t challenge[32],
                           uint8_t response[64])
{
	if (device_private_key_slot > 15 || challenge == NULL || response == NULL)
		return ATCACERT_E_BAD_PARAMS;

	return atcab_sign(device_private_key_slot, challenge, response);
}