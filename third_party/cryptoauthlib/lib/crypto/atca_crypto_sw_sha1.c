/**
 * \file
 * \brief Wrapper API for SHA 1 routines
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


#include "atca_crypto_sw_sha1.h"
#include "hashes/sha1_routines.h"

int atcac_sw_sha1_init(atcac_sha1_ctx* ctx)
{
	if (sizeof(CL_HashContext) > sizeof(atcac_sha1_ctx))
		return ATCA_ASSERT_FAILURE; // atcac_sha1_ctx isn't large enough for this implementation
	CL_hashInit((CL_HashContext*)ctx);

	return ATCA_SUCCESS;
}

int atcac_sw_sha1_update(atcac_sha1_ctx* ctx, const uint8_t* data, size_t data_size)
{
	CL_hashUpdate((CL_HashContext*)ctx, data, (int)data_size);

	return ATCA_SUCCESS;
}

int atcac_sw_sha1_finish(atcac_sha1_ctx* ctx, uint8_t digest[ATCA_SHA1_DIGEST_SIZE])
{
	CL_hashFinal((CL_HashContext*)ctx, digest);

	return ATCA_SUCCESS;
}

int atcac_sw_sha1(const uint8_t* data, size_t data_size, uint8_t digest[ATCA_SHA1_DIGEST_SIZE])
{
	int ret;
	atcac_sha1_ctx ctx;

	ret = atcac_sw_sha1_init(&ctx);
	if (ret != ATCA_SUCCESS)
		return ret;

	ret = atcac_sw_sha1_update(&ctx, data, data_size);
	if (ret != ATCA_SUCCESS)
		return ret;

	ret = atcac_sw_sha1_finish(&ctx, digest);
	if (ret != ATCA_SUCCESS)
		return ret;

	return ATCA_SUCCESS;
}