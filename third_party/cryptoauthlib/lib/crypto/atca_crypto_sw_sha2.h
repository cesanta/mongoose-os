/**
 * \file
 * \brief  Wrapper API for software SHA 256 routines
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

#ifndef ATCA_CRYPTO_SW_SHA2_H
#define ATCA_CRYPTO_SW_SHA2_H

#include "atca_crypto_sw.h"
#include <stddef.h>
#include <stdint.h>

/** \defgroup atcac_ Software crypto methods (atcac_)
 *
 * \brief
 * These methods provide a software implementation of various crypto
 * algorithms
 *
   @{ */

#define ATCA_SHA2_256_DIGEST_SIZE (32)

typedef struct {
	uint32_t pad[48]; //!< Filler value to make sure the actual implementation has enough room to store its context. uint32_t is used to remove some alignment warnings.
} atcac_sha2_256_ctx;

#ifdef __cplusplus
extern "C" {
#endif

int atcac_sw_sha2_256_init(atcac_sha2_256_ctx* ctx);
int atcac_sw_sha2_256_update(atcac_sha2_256_ctx* ctx, const uint8_t* data, size_t data_size);
int atcac_sw_sha2_256_finish(atcac_sha2_256_ctx * ctx, uint8_t digest[ATCA_SHA2_256_DIGEST_SIZE]);
int atcac_sw_sha2_256(const uint8_t * data, size_t data_size, uint8_t digest[ATCA_SHA2_256_DIGEST_SIZE]);

#ifdef __cplusplus
}
#endif

/** @} */
#endif