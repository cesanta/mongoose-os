/**
 * \file
 * \brief Host side methods using software implementations.  host-side, the one authenticating
 *        a client, of the authentication process. Crypto functions are performed using a software library.
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

#ifndef ATCACERT_HOST_SOFT_H
#define ATCACERT_HOST_SOFT_H

#include <stddef.h>
#include <stdint.h>
#include "atcacert_def.h"

// Inform function naming when compiling in C++
#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup atcacert_ Certificate manipulation methods (atcacert_)
 *
 * \brief
 * These methods provide convenient ways to perform certification I/O with
 * CryptoAuth chips and perform certificate manipulation in memory
 *
   @{ */

/**
 * \brief Verify a certificate against its certificate authority's public key using software crypto
 *        functions.
 *
 * \param[in] cert_def       Certificate definition describing how to extract the TBS and signature
 *                           components from the certificate specified.
 * \param[in] cert           Certificate to verify.
 * \param[in] cert_size      Size of the certificate (cert) in bytes.
 * \param[in] ca_public_key  The ECC P256 public key of the certificate authority that signed this
 *                           certificate. Formatted as the 32 byte X and Y integers concatenated
 *                           together (64 bytes total).
 *
 * \return 0 if the verify succeeds, ATCACERT_VERIFY_FAILED if it fails to verify.
 */
int atcacert_verify_cert_sw( const atcacert_def_t* cert_def,
                             const uint8_t*        cert,
                             size_t cert_size,
                             const uint8_t ca_public_key[64]);

/**
 * \brief Generate a random challenge to be sent to the client using a software PRNG.
 *
 * \param[out] challenge  Random challenge is return here. 32 bytes.
 *
 * \return 0 on success
 */
int atcacert_gen_challenge_sw( uint8_t challenge[32] );

/**
 * \brief Verify a client's response to a challenge using software crypto functions.
 *
 * The challenge-response protocol is an ECDSA Sign and Verify. This performs an ECDSA verify on the
 * response returned by the client, verifying the client has the private key counter-part to the
 * public key returned in its certificate.
 *
 * \param[in] device_public_key  Device public key as read from its certificate. Formatted as the X
 *                               and Y integers concatenated together. 64 bytes.
 * \param[in] challenge          Challenge that was sent to the client. 32 bytes.
 * \param[in] response           Response returned from the client to be verified. 64 bytes.
 *
 * \return 0 if the verify succeeds. ATCACERT_BAD_RESPONSE if the verify fails.
 */
int atcacert_verify_response_sw( const uint8_t device_public_key[64],
                                 const uint8_t challenge[32],
                                 const uint8_t response[64]);

/** @} */
#ifdef __cplusplus
}
#endif

#endif