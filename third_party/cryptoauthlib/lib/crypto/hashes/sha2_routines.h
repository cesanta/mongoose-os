/**
 * \file
 * \brief Software implementation of the SHA256 algorithm.
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

#ifndef SHA2_ROUTINES_H
#define SHA2_ROUTINES_H

#include <stdint.h>

#define SHA256_DIGEST_SIZE (32)
#define SHA256_BLOCK_SIZE  (64)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t total_msg_size;                //!< Total number of message bytes processed
	uint32_t block_size;                    //!< Number of bytes in current block
	uint8_t block[SHA256_BLOCK_SIZE * 2];   //!< Unprocessed message storage
	uint32_t hash[8];                       //!< Hash state
} sw_sha256_ctx;

void sw_sha256_init(sw_sha256_ctx* ctx);

void sw_sha256_update(sw_sha256_ctx* ctx, const uint8_t* message, uint32_t len);

void sw_sha256_final(sw_sha256_ctx * ctx, uint8_t digest[SHA256_DIGEST_SIZE]);

void sw_sha256(const uint8_t * message, unsigned int len, uint8_t digest[SHA256_DIGEST_SIZE]);

#ifdef __cplusplus
}
#endif

#endif // SHA2_ROUTINES_H

