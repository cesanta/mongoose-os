/**
 * \file
 * \brief Wrapper API for software SHA 256 routines
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

#include "atca_crypto_sw_sha2.h"
#include "hashes/sha2_routines.h"

/** \brief initializes the SHA256 software
 * \param[in] ctx  ptr to context data structure
 * \return ATCA_STATUS value
 */

int atcac_sw_sha2_256_init(atcac_sha2_256_ctx* ctx)
{
	if (sizeof(sw_sha256_ctx) > sizeof(atcac_sha2_256_ctx))
		return ATCA_ASSERT_FAILURE; // atcac_sha1_ctx isn't large enough for this implementation
	sw_sha256_init((sw_sha256_ctx*)ctx);

	return ATCA_SUCCESS;
}

/** \brief updates the running hash with the next block of data, called iteratively for the entire
    stream of data to be hashed
    \param[in] ctx        ptr to SHA context data structure
    \param[in] data       ptr to next block of data to hash
    \param[in] data_size  size amount of data to hash in the given block, in bytes
    \return ATCA_STATUS
 */

int atcac_sw_sha2_256_update(atcac_sha2_256_ctx* ctx, const uint8_t* data, size_t data_size)
{
	sw_sha256_update((sw_sha256_ctx*)ctx, data, (uint32_t)data_size);

	return ATCA_SUCCESS;
}

/** \brief completes the final SHA calculation and returns the final digest/hash
 * \param[in]  ctx     ptr to context data structure
 * \param[out] digest  receives the computed digest of the SHA 256 has
 * \return ATCA_STATUS
 */

int atcac_sw_sha2_256_finish(atcac_sha2_256_ctx* ctx, uint8_t digest[ATCA_SHA2_256_DIGEST_SIZE])
{
	sw_sha256_final((sw_sha256_ctx*)ctx, digest);

	return ATCA_SUCCESS;
}


/** \brief single call convenience function to comput SHA256 of given data
 * \param[in]  data       pointer to stream of data to hash
 * \param[in]  data_size  size of data stream to hash
 * \param[out] digest     result
 * \return ATCA_STATUS
 */

int atcac_sw_sha2_256(const uint8_t* data, size_t data_size, uint8_t digest[ATCA_SHA2_256_DIGEST_SIZE])
{
	int ret;
	atcac_sha2_256_ctx ctx;

	ret = atcac_sw_sha2_256_init(&ctx);
	if (ret != ATCA_SUCCESS)
		return ret;

	ret = atcac_sw_sha2_256_update(&ctx, data, data_size);
	if (ret != ATCA_SUCCESS)
		return ret;

	ret = atcac_sw_sha2_256_finish(&ctx, digest);
	if (ret != ATCA_SUCCESS)
		return ret;

	return ATCA_SUCCESS;
}