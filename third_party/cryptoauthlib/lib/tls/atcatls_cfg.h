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

#ifndef ATCATLS_CFG_H
#define ATCATLS_CFG_H

#include "cryptoauthlib.h"
/** \defgroup atcatls TLS integration with ATECC (atcatls_)
   @{ */

// Slot definitions for ECC508 used by the default TLS configuration.
#define TLS_SLOT_AUTH_PRIV      ((uint8_t)0x0)  //!< Primary authentication private key
#define TLS_SLOT_AUTH_PMK       ((uint8_t)0x1)  //!< Premaster key for ECDH cipher suites
#define TLS_SLOT_ECDH_PRIV      ((uint8_t)0x2)  //!< ECDH private key
#define TLS_SLOT_ECDHE_PRIV     ((uint8_t)0x2)  //!< ECDHE private key
#define TLS_SLOT_ECDH_PMK       ((uint8_t)0x3)  //!< ECDH/ECDHE pmk slot.  This slot is encrypted with encParentSlot
#define TLS_SLOT_ENC_PARENT     ((uint8_t)0x4)  //!< The parent encryption key.  This is a random key set on a per-platform basis.
#define TLS_SLOT_SHAKEY         ((uint8_t)0x5)  //!< SHA key slot.  Used for SHA use cases
#define TLS_SLOT_HOST_SHAKEY    ((uint8_t)0x6)  //!< Host SHA key slot.  Used for host SHA use cases
#define TLS_SLOT_FEATURE_PRIV   ((uint8_t)0x7)  //!< Feature private key. Used for feature use cases
#define TLS_SLOT8_ENC_STORE     ((uint8_t)0x8)  //!< Encrypted storage for 416 bytes
#define TLS_SLOT9_ENC_STORE     ((uint8_t)0x9)  //!< Encrypted storage for 72 bytes
#define TLS_SLOT_AUTH_CERT      ((uint8_t)0xA)  //!< Compressed certificate information for the authPrivSlot
#define TLS_SLOT_SIGNER_PUBKEY  ((uint8_t)0xB)  //!< Public key of the signer of authCertSlot.
#define TLS_SLOT_SIGNER_CERT    ((uint8_t)0xC)  //!< Compressed certificate information for the signerPubkey
#define TLS_SLOT_FEATURE_CERT   ((uint8_t)0xD)  //!< Compressed certificate information for the featurePrivSlot
#define TLS_SLOT_PKICA_PUBKEY   ((uint8_t)0xE)  //!< Public key for the PKI certificate authority
#define TLS_SLOT_MFRCA_PUBKEY   ((uint8_t)0xF)  //!< Public key for the MFR certificate authority

typedef struct {
	uint8_t authPrivSlot;       //!< Primary authentication private key
	uint8_t authPmkSlot;        //!< Premaster key for ECDH cipher suites
	uint8_t ecdhPrivSlot;       //!< ECDH private key
	uint8_t ecdhePrivSlot;      //!< ECDHE private key
	uint8_t ecdhPmkSlot;        //!< ECDH/ECDHE pmk slot.  This slot is encrypted with encParentSlot
	uint8_t encParentSlot;      //!< The parent encryption key.  This is a random key set on a per-platform basis.
	uint8_t shaKeySlot;         //!< SHA key slot.  Used for SHA use cases
	uint8_t hostShaKeySlot;     //!< Host SHA key slot.  Used for host SHA use cases
	uint8_t featurePrivSlot;    //!< Feature private key. Used for feature use cases
	uint8_t encStoreSlot8;      //!< Encrypted storage for 416 bytes
	uint8_t encStoreSlot9;      //!< Encrypted storage for 72 bytes
	uint8_t authCertSlot;       //!< Compressed certificate information for the authPrivSlot
	uint8_t signerPubkeySlot;   //!< Public key of the signer of authCertSlot.
	uint8_t signerCertSlot;     //!< Compressed certificate information for the signerPubkey
	uint8_t featureCertSlot;    //!< Compressed certificate information for the featurePrivSlot
	uint8_t pkiCaPubkeySlot;    //!< Public key for the PKI certificate authority
	uint8_t mfrCaPubkeySlot;    //!< Public key for the MFR certificate authority
} TlsSlotDef;

/** @} */

#endif // ATCATLS_CFG_H
