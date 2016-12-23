/**
 * \file
 *
 * \brief  Collection of functions for hardware abstraction of TLS implementations (e.g. OpenSSL)
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

#ifndef ATCATLS_H
#define ATCATLS_H

#include "cryptoauthlib.h"
#include "atcacert/atcacert_def.h"

/** \defgroup atcatls TLS integration with ATECC (atcatls_)
 *
 * \brief
    Instructions for integrating the ECC508 into a platform:
    1.	Add compiler switch for ECC_HAL
    2.	Define the configuration properties of the secure element in the atcatls_cfg.h file
    3.	Add call to HAL_init() before main loop.
            - This will use the configuration information and replace the private key resource in /oic/sec/cred with a handle to the secure element keys.
    4.	Re-compile the OIC implementation for the target platform
   @{ */

// The number of bytes in a standard ECC508 memory block
//#define MEM_BLOCK_SIZE      ATCA_BLOCK_SIZE
//#define TLS_RANDOM_SIZE     MEM_BLOCK_SIZE
// The number of bytes in ECC keys & signatures
//#define PUB_KEY_SIZE        ATCA_PUB_KEY_SIZE
//#define PRIV_KEY_SIZE       ATCA_PRIV_KEY_SIZE
//#define SIGNATURE_SIZE      ATCA_SIG_SIZE

// Configures the device for tls operations
ATCA_STATUS atcatls_config_default(void);

// TLS API Init/finish
ATCA_STATUS atcatls_init(ATCAIfaceCfg *pCfg);
ATCA_STATUS atcatls_finish(void);

// Core TLS definitions
ATCA_STATUS atcatls_sign(uint8_t slotid, const uint8_t *message, uint8_t *signature);
ATCA_STATUS atcatls_verify(const uint8_t *message, const uint8_t *signature, const uint8_t *pubkey, bool *verified);
ATCA_STATUS atcatls_ecdh(uint8_t slotid, const uint8_t* pubkey, uint8_t* pmk);
ATCA_STATUS atcatls_ecdh_enc(uint8_t slotid, uint8_t enckeyId, const uint8_t* pubkey, uint8_t* pmk);
ATCA_STATUS atcatls_ecdhe(uint8_t slotid, const uint8_t* pubkey, uint8_t* pubkeyret, uint8_t* pmk);
ATCA_STATUS atcatls_create_key(uint8_t slotid, uint8_t *pubkey);
ATCA_STATUS atcatls_calc_pubkey(uint8_t slotid, uint8_t *pubkey);
ATCA_STATUS atcatls_write_pubkey(uint8_t slotid, uint8_t pubkey[ATCA_PUB_KEY_SIZE], bool lock);
ATCA_STATUS atcatls_read_pubkey(uint8_t slotid, uint8_t *pubkey);
ATCA_STATUS atcatls_random(uint8_t* randout);
ATCA_STATUS atcatls_get_sn(uint8_t sn_out[ATCA_SERIAL_NUM_SIZE]);

// Certificate Handling
ATCA_STATUS atcatls_get_cert(const atcacert_def_t* cert_def, const uint8_t *ca_public_key, uint8_t *certout, size_t* certsize);
ATCA_STATUS atcatls_get_ca_cert(uint8_t *certout, size_t* certsize);
ATCA_STATUS atcatls_verify_cert(const atcacert_def_t* cert_def, const uint8_t* cert, size_t cert_size, const uint8_t* ca_public_key);
ATCA_STATUS atcatls_read_ca_pubkey(uint8_t slotid, uint8_t caPubkey[ATCA_PUB_KEY_SIZE]);

// CSR Handling
ATCA_STATUS atcatls_create_csr(const atcacert_def_t* csr_def, char *csrout, size_t* csrsize);

// Test Certificates 
ATCA_STATUS atcatls_get_device_cert(uint8_t *certout, size_t* certsize);
ATCA_STATUS atcatls_get_signer_cert(uint8_t *certout, size_t* certsize);

// Encrypted Read/Write
ATCA_STATUS atcatls_init_enckey(uint8_t* enckeyout, uint8_t enckeyId, bool lock);
ATCA_STATUS atcatls_set_enckey(uint8_t* enckeyin, uint8_t enckeyId, bool lock);
ATCA_STATUS atcatls_get_enckey(uint8_t* enckeyout);
ATCA_STATUS atcatls_enc_read(uint8_t slotid, uint8_t block, uint8_t enckeyId, uint8_t* data, int16_t* bufsize);
ATCA_STATUS atcatls_enc_write(uint8_t slotid, uint8_t block, uint8_t enckeyId, uint8_t* data, int16_t bufsize);
ATCA_STATUS atcatls_enc_rsakey_read(uint8_t enckeyId, uint8_t* rsakey, int16_t* keysize);
ATCA_STATUS atcatls_enc_rsakey_write(uint8_t enckeyId, uint8_t* rsakey, int16_t keysize);

// Interface to get the encryption key from the platform
typedef ATCA_STATUS (atcatlsfn_get_enckey)(uint8_t* enckey, int16_t keysize);
ATCA_STATUS atcatlsfn_set_get_enckey(atcatlsfn_get_enckey* fn_get_enckey);

/** @} */

#endif // ATCATLS_H
