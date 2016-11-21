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


#ifndef ATCACERT_CLIENT_H
#define ATCACERT_CLIENT_H

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
 * \brief Reads the certificate specified by the certificate definition from the
 *        ATECC508A device.
 *
 * This process involves reading the dynamic cert data from the device and combining it
 * with the template found in the certificate definition.
 *
 * \param[in]    cert_def       Certificate definition describing where to find the dynamic
 *                              certificate information on the device and how to incorporate it
 *                              into the template.
 * \param[in]    ca_public_key  The ECC P256 public key of the certificate authority that signed
 *                              this certificate. Formatted as the 32 byte X and Y integers
 *                              concatenated together (64 bytes total).
 * \param[out]   cert           Buffer to received the certificate.
 * \param[inout] cert_size      As input, the size of the cert buffer in bytes.
 *                              As output, the size of the certificate returned in cert in bytes.
 *
 * \return 0 on success
 */
int atcacert_read_cert( const atcacert_def_t* cert_def,
                        const uint8_t ca_public_key[64],
                        uint8_t*              cert,
                        size_t*               cert_size);

/**
 * \brief Calculates the response to a challenge sent from the host.
 *
 * The challenge-response protocol is an ECDSA Sign and Verify. This performs the ECDSA Sign on the
 * challenge and returns the signature as the response.
 *
 * \param[in]  device_private_key_slot  Slot number for the device's private key. This must be the
 *                                      same slot used to generate the public key included in the
 *                                      device's certificate.
 * \param[in]  challenge                Challenge to generate the response for. Must be 32 bytes.
 * \param[out] response                 Response will be returned in this buffer. 64 bytes.
 *
 * \return 0 on success
 */
int atcacert_get_response( uint8_t device_private_key_slot,
                           const uint8_t challenge[32],
                           uint8_t response[64]);

/** @} */
#ifdef __cplusplus
}
#endif

#endif