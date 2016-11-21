/**
 * \file
 * \brief host side methods using CryptoAuth hardware
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

#include "atcacert_host_hw.h"
#include "basic/atca_basic.h"
#include "crypto/atca_crypto_sw_sha2.h"

int atcacert_verify_cert_hw( const atcacert_def_t* cert_def,
                             const uint8_t*        cert,
                             size_t cert_size,
                             const uint8_t ca_public_key[64])
{
	int ret = 0;
	uint8_t tbs_digest[32];
	uint8_t signature[64];
	bool is_verified = false;

	if (cert_def == NULL || ca_public_key == NULL || cert == NULL)
		return ATCACERT_E_BAD_PARAMS;

	ret = atcacert_get_tbs_digest(cert_def, cert, cert_size, tbs_digest);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_get_signature(cert_def, cert, cert_size, signature);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcab_verify_extern(tbs_digest, signature, ca_public_key, &is_verified);
	if (ret != ATCA_SUCCESS)
		return ret;

	return is_verified ? ATCACERT_E_SUCCESS : ATCACERT_E_VERIFY_FAILED;
}

int atcacert_gen_challenge_hw( uint8_t challenge[32] )
{
	if (challenge == NULL)
		return ATCACERT_E_BAD_PARAMS;

	return atcab_random(challenge);
}

int atcacert_verify_response_hw( const uint8_t device_public_key[64],
                                 const uint8_t challenge[32],
                                 const uint8_t response[64])
{
	int ret = 0;
	bool is_verified = false;

	if (device_public_key == NULL || challenge == NULL || response == NULL)
		return ATCACERT_E_BAD_PARAMS;

	ret = atcab_verify_extern(challenge, response, device_public_key, &is_verified);
	if (ret != ATCA_SUCCESS)
		return ret;

	return is_verified ? ATCACERT_E_SUCCESS : ATCACERT_E_VERIFY_FAILED;
}