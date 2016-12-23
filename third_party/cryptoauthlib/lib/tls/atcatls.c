/**
 * \file
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

#include <stdlib.h>
#include <stdio.h>
#include "atcatls.h"
#include "atcatls_cfg.h"
#include "basic/atca_basic.h"
#include "atcacert/atcacert_client.h"
#include "atcacert/atcacert_host_hw.h"

// File scope defines
// The RSA key will be written to the upper blocks of slot 8
#define RSA_KEY_SLOT            8
#define RSA_KEY_START_BLOCK     5

// File scope global varibles
uint8_t _enckey[ATCA_KEY_SIZE] = { 0 };
atcatlsfn_get_enckey* _fn_get_enckey = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////

//#define LOCKABLE_SHA_KEYS

//// Data to be written to each Address
//uint8_t config_data_default[] = {
//	// block 0
//	// Not Written: First 16 bytes are not written
//	0x01, 0x23, 0x00, 0x00,
//	0x00, 0x00, 0x50, 0x00,
//	0x04, 0x05, 0x06, 0x07,
//	0xEE, 0x00, 0x01, 0x00,
//	// I2C, reserved, OtpMode, ChipMode
//	0xC0, 0x00, 0xAA, 0x00,
//	// SlotConfig
//	0x8F, 0x20, 0xC4, 0x44,
//	0x87, 0x20, 0xC4, 0x44,
//#ifdef LOCKABLE_SHA_KEYS
//	0x8F, 0x0F, 0x8F, 0x0F,
//	// block 1
//	0x9F, 0x0F, 0x82, 0x20,
//#else
//	0x8F, 0x0F, 0x8F, 0x8F,
//	// block 1
//	0x9F, 0x8F, 0x82, 0x20,
//#endif
//	0xC4, 0x44, 0xC4, 0x44,
//	0x0F, 0x0F, 0x0F, 0x0F,
//	0x0F, 0x0F, 0x0F, 0x0F,
//	0x0F, 0x0F, 0x0F, 0x0F,
//	// Counters
//	0xFF, 0xFF, 0xFF, 0xFF,
//	0x00, 0x00, 0x00, 0x00,
//	0xFF, 0xFF, 0xFF, 0xFF,
//	// block 2
//	0x00, 0x00, 0x00, 0x00,
//	// Last Key Use
//	0xFF, 0xFF, 0xFF, 0xFF,
//	0xFF, 0xFF, 0xFF, 0xFF,
//	0xFF, 0xFF, 0xFF, 0xFF,
//	0xFF, 0xFF, 0xFF, 0xFF,
//	// Not Written: UserExtra, Selector, LockValue, LockConfig (word offset = 5)
//	0x00, 0x00, 0x00, 0x00,
//	// SlotLock[2], RFU[2]
//	0xFF, 0xFF, 0x00, 0x00,
//	// X.509 Format
//	0x00, 0x00, 0x00, 0x00,
//	// block 3
//	// KeyConfig
//	0x33, 0x00, 0x5C, 0x00,
//	0x13, 0x00, 0x5C, 0x00,
//#ifdef LOCKABLE_SHA_KEYS
//	0x3C, 0x00, 0x3C, 0x00,
//	0x3C, 0x00, 0x33, 0x00,
//#else
//	0x3C, 0x00, 0x1C, 0x00,
//	0x1C, 0x00, 0x33, 0x00,
//#endif
//	0x1C, 0x00, 0x1C, 0x00,
//	0x3C, 0x00, 0x3C, 0x00,
//	0x3C, 0x00, 0x3C, 0x00,
//	0x1C, 0x00, 0x3C, 0x00,
//};

/** \brief Configure the ECC508 for use with TLS API funcitons.
 *		The configuration zone is written and locked.
 *		All GenKey and slot initialization is done and then the data zone is locked.
 *		This configuration needs to be performed before the TLS API functions are called
 *		On a locked ECC508 device, this function will check the configuraiton against the default and fail if it does not match.
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_config_default()
{
	return device_init_default();
	//ATCA_STATUS status = ATCA_SUCCESS;
	//bool isLocked = false;
	//bool sameConfig = false;
	//uint8_t lockRsp[LOCK_RSP_SIZE] = { 0 };
	//uint8_t pubkey[ATCA_PUB_KEY_SIZE] = { 0 };

	//do {

	//	// Get the config lock setting
	//	if ((status = atcab_is_locked(LOCK_ZONE_CONFIG, &isLocked)) != ATCA_SUCCESS) BREAK(status, "Read of lock byte failed");

	//	if (isLocked == false) {
	//		// Configuration zone must be unlocked for the write to succeed
	//		if ((status = atcab_write_ecc_config_zone(config_data_default)) != ATCA_SUCCESS) BREAK(status, "Write config zone failed");

	//		// Lock the config zone
	//		if ((status = atcab_lock_config_zone(lockRsp) != ATCA_SUCCESS)) BREAK(status, "Lock config zone failed");

	//		// At this point we have a properly configured and locked config zone
	//		// GenKey all public-private key pairs
	//		if ((status = atcab_genkey(TLS_SLOT_AUTH_PRIV, pubkey)) != ATCA_SUCCESS) BREAK(status, "Genkey failed:AUTH_PRIV_SLOT");
	//		if ((status = atcab_genkey(TLS_SLOT_ECDH_PRIV, pubkey)) != ATCA_SUCCESS) BREAK(status, "Genkey failed:ECDH_PRIV_SLOT");
	//		if ((status = atcab_genkey(TLS_SLOT_FEATURE_PRIV, pubkey)) != ATCA_SUCCESS) BREAK(status, "Genkey failed:FEATURE_PRIV_SLOT");
	//	}else {
	//		// If the config zone is locked, compare the bytes to this configuration
	//		if ((status = atcab_cmp_config_zone(config_data_default, &sameConfig)) != ATCA_SUCCESS) BREAK(status, "Config compare failed");
	//		if (sameConfig == false) {
	//			// The device is locked with the wrong configuration, return an error
	//			status = ATCA_GEN_FAIL;
	//			BREAK(status, "The device is locked with the wrong configuration");
	//		}
	//	}
	//	// Lock the Data zone
	//	// Don't get status since it is ok if it's already locked
	//	atcab_lock_data_zone(lockRsp);

	//} while (0);

	//return status;
}

/** \brief Initialize the ECC508 for use with the TLS API.  Like a constructor
 *  \param[in] pCfg The ATCAIfaceCfg configuration that defines the HAL layer interface
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_init(ATCAIfaceCfg* pCfg)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	// Insert any other constructor code for TLS here.
	status = atcab_init(pCfg);
	return status;
}

/** \brief Finalize the ECC508 when finished.  Like a destructor
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_finish()
{
	ATCA_STATUS status = ATCA_SUCCESS;

	// Insert any other destructor code for TLS here.
	status = atcab_release();
	return status;
}

/** \brief Get the serial number of this device
 *  \param[out] sn_out Pointer to the buffer that will hold the 9 byte serial number read from this device
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_get_sn(uint8_t sn_out[ATCA_SERIAL_NUM_SIZE])
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Call the basic API to get the serial number
		if ((status = atcab_read_serial_number(sn_out)) != ATCA_SUCCESS) BREAK(status, "Get serial number failed");

	} while (0);

	return status;
}

/** \brief Sign the message with the specified slot and return the signature
 *  \param[in] slotid The private P256 key slot to use for signing
 *  \param[in] message A pointer to the 32 byte message to be signed
 *  \param[out] signature A pointer that will hold the 64 byte P256 signature
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_sign(uint8_t slotid, const uint8_t *message, uint8_t *signature)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Check the inputs
		if (message == NULL || signature == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Bad input parameters");
		}
		// Sign the message
		if ((status = atcab_sign(slotid, message, signature)) != ATCA_SUCCESS) BREAK(status, "Sign Failed");

	} while (0);

	return status;
}

/** \brief Verify the signature of the specified message using the specified public key
 *  \param[in] message A pointer to the 32 byte message to be verified
 *  \param[in] signature A pointer to the 64 byte P256 signature to be verified
 *  \param[in] pubkey A pointer to the 64 byte P256 public key used for verificaion
 *  \param[out] verified A pointer to the boolean result of this verify operation
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_verify(const uint8_t *message, const uint8_t *signature, const uint8_t *pubkey, bool *verified)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Check the inputs
		if (message == NULL || signature == NULL || pubkey == NULL || verified == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Bad input parameters");
		}
		// Verify the signature of the message
		if ((status = atcab_verify_extern(message, signature, pubkey, verified)) != ATCA_SUCCESS) BREAK(status, "Verify Failed");

	} while (0);

	return status;
}

/**
 * \brief Verify a certificate against its certificate authority's public key using the host's ATECC device for crypto functions.
 * \param[in] cert_def       Certificate definition describing how to extract the TBS and signature components from the certificate specified.
 * \param[in] cert           Certificate to verify.
 * \param[in] cert_size      Size of the certificate (cert) in bytes.
 * \param[in] ca_public_key  The ECC P256 public key of the certificate authority that signed this
 *                           certificate. Formatted as the 32 byte X and Y integers concatenated
 *                           together (64 bytes total).
 * \return ATCA_SUCCESS if the verify succeeds, ATCACERT_VERIFY_FAILED or ATCA_EXECUTION_ERROR if it fails to verify.
 */
ATCA_STATUS atcatls_verify_cert(const atcacert_def_t* cert_def, const uint8_t* cert, size_t cert_size, const uint8_t* ca_public_key)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Check the inputs
		if (cert_def == NULL || cert == NULL || ca_public_key == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Bad input parameters");
		}
		// Verify the certificate
		status = atcacert_verify_cert_hw(cert_def, cert, cert_size, ca_public_key);
		if (status != ATCA_SUCCESS) BREAK(status, "Verify Failed");

	} while (0);

	return status;
}

/** \brief Generate a pre-master key (pmk) given a private key slot and a public key that will be shared with
 *  \param[in] slotid slot of key for ECDH computation
 *  \param[in] pubkey public to shared with
 *  \param[out] pmk - computed ECDH key - A buffer with size of ATCA_KEY_SIZE
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_ecdh(uint8_t slotid, const uint8_t* pubkey, uint8_t* pmk)
{
	return atcatls_ecdh_enc(slotid, TLS_SLOT_ENC_PARENT, pubkey, pmk);
}

/** \brief Generate a pre-master key (pmk) given a private key slot and a public key that will be shared with.
 *         This version performs an encrypted read from (slotid + 1)
 *  \param[in] slotid Slot of key for ECDH computation
 *  \param[in] enckeyid Slot of key for the encryption parent
 *  \param[in] pubkey Public to shared with
 *  \param[out] pmk - Computed ECDH key - A buffer with size of ATCA_KEY_SIZE
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_ecdh_enc(uint8_t slotid, uint8_t enckeyid, const uint8_t* pubkey, uint8_t* pmk)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t encKey[ECDH_KEY_SIZE] = { 0 };

	do {
		// Check the inputs
		if (pubkey == NULL || pmk == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Bad input parameters");
		}
		// Get the encryption key for this platform
		if ((status = atcatls_get_enckey(encKey)) != ATCA_SUCCESS) BREAK(status, "Get enckey Failed");

		// Send the encrypted version of the ECDH command with the public key provided
		if ((status = atcab_ecdh_enc(slotid, pubkey, pmk, encKey, enckeyid)) != ATCA_SUCCESS) BREAK(status, "ECDH Failed");

	} while (0);

	return status;
}

/** \brief Generate a pre-master key (pmk) given a private key slot to create and a public key that will be shared with
 *  \param[in] slotid Slot of key for ECDHE computation
 *  \param[in] pubkey Public to share with
 *  \param[out] pubkeyret Public that was created as part of the ECDHE operation
 *  \param[out] pmk - Computed ECDH key - A buffer with size of ATCA_KEY_SIZE
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_ecdhe(uint8_t slotid, const uint8_t* pubkey, uint8_t* pubkeyret, uint8_t* pmk)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Check the inputs
		if ((pubkey == NULL) || (pubkeyret == NULL) || (pmk == NULL)) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Bad input parameters");
		}
		// Create a new key in the ECDH slot
		if ((status = atcab_genkey(slotid, pubkeyret)) != ATCA_SUCCESS) BREAK(status, "Create key failed");

		// Send the ECDH command with the public key provided
		if ((status = atcab_ecdh(slotid, pubkey, pmk)) != ATCA_SUCCESS) BREAK(status, "ECDH failed");


	} while (0);

	return status;
}

/** \brief Create a unique public-private key pair in the specified slot
 *  \param[in] slotid The slot id to create the ECC private key
 *  \param[out] pubkey Pointer the public key bytes that coorespond to the private key that was created
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_create_key(uint8_t slotid, uint8_t* pubkey)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (pubkey == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Call the Genkey command on the specified slot
		if ((status = atcab_genkey(slotid, pubkey)) != ATCA_SUCCESS) BREAK(status, "Create key failed");

	} while (0);

	return status;
}

/** \brief Get the public key from the specified private key slot
 *  \param[in] slotid The slot id containing the private key used to calculate the public key
 *  \param[out] pubkey Pointer the public key bytes that coorespond to the private key
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_calc_pubkey(uint8_t slotid, uint8_t *pubkey)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (pubkey == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Call the GenKey command to return the public key
		if ((status = atcab_get_pubkey(slotid, pubkey)) != ATCA_SUCCESS) BREAK(status, "Gen public key failed");

	} while (0);

	return status;
}

/** \brief reads a pub key from a readable data slot versus atcab_get_pubkey which generates a pubkey from a private key slot
 *  \param[in] slotid Slot number to read, expected value is 0x8 through 0xF
 *  \param[out] pubkey Pointer the public key bytes that were read from the slot
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_read_pubkey(uint8_t slotid, uint8_t *pubkey)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (pubkey == NULL || slotid < 8 || slotid > 0xF) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Bad atcatls_read_pubkey() input parameters");
		}
		// Call the GenKey command to return the public key
		if ((status = atcab_read_pubkey(slotid, pubkey)) != ATCA_SUCCESS) BREAK(status, "Read public key failed");

	} while (0);

	return status;
}

/** \brief Get a random number
 *  \param[out] randout Pointer the 32 random bytes that were returned by the Random Command
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_random(uint8_t* randout)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (randout == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Call the random command
		if ((status = atcab_random(randout)) != ATCA_SUCCESS) BREAK(status, "Random command failed");

	} while (0);

	return status;
}

/** \brief Set the function used to retrieve the unique encryption key for this platform.
 *  \param[in] fn_get_enckey Pointer to a function that will return the platform encryption key
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatlsfn_set_get_enckey(atcatlsfn_get_enckey* fn_get_enckey)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (fn_get_enckey == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Set the get_enckey callback function
		_fn_get_enckey = fn_get_enckey;

	} while (0);

	return status;
}

/** \brief Initialize the unique encryption key for this platform.
 *		Write a random number to the parent encryption key slot
 *		Return the random number for storage on platform
 *  \param[out] enckeyout Pointer to a random 32 byte encryption key that will be stored on the platform and in the device
 *  \param[in] enckeyid Slot id on the ECC508 to store the encryption key
 *  \param[in] lock If this is set to true, the slot that stores the encryption key will be locked
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_init_enckey(uint8_t* enckeyout, uint8_t enckeyid, bool lock)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (enckeyout == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Get a random number
		if ((status = atcatls_random(enckeyout)) != ATCA_SUCCESS) BREAK(status, "Random command failed");

		// Write the random number as the encryption key
		atcatls_set_enckey(enckeyout, enckeyid, lock);

	} while (0);

	return status;
}

/** \brief Initialize the unique encryption key for this platform
 *		Write the provided encryption key to the parent encryption key slot
 *		Function optionally lock the parent encryption key slot after it is written
 *  \param[in] enckeyin Pointer to a 32 byte encryption key that will be stored on the platform and in the device
 *  \param[in] enckeyid Slot id on the ECC508 to store the encryption key
 *  \param[in] lock If this is set to true, the slot that stores the encryption key will be locked
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_set_enckey(uint8_t* enckeyin, uint8_t enckeyid, bool lock)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t block = 0;
	uint8_t offset = 0;
	uint8_t lockSuccess = 0;

	do {
		// Verify input parameters
		if (enckeyin == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Write the random number to specified slot
		if ((status = atcab_write_zone(ATCA_ZONE_DATA, enckeyid, block, offset, enckeyin, ATCA_BLOCK_SIZE)) != ATCA_SUCCESS)
			BREAK(status, "Write parent encryption key failed");

		// Optionally lock the key
		if (lock)
			// Send the slot lock command for this slot, ignore the return status
			atcab_lock_data_slot(enckeyid, &lockSuccess);

	} while (0);

	return status;
}

/** \brief Return the random number for storage on platform.
 *		This function reads from platform storage, not the ECC508 device
 *		Therefore, the implementation is platform specific and must be provided at integration
 *  \param[out] enckeyout Pointer to a 32 byte encryption key that is stored on the platform and in the device
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_get_enckey(uint8_t* enckeyout)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (enckeyout == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Memset the output to 0x00
		memset(enckeyout, 0x00, ATCA_KEY_SIZE);

		// Call the function provided by the platform.  The encryption key must be stored in the platform
		if (_fn_get_enckey != NULL)
			_fn_get_enckey(enckeyout, ATCA_KEY_SIZE);
		else
			// Get encryption key funciton is not defined.  Return failure.
			status = ATCA_FUNC_FAIL;
	} while (0);

	return status;
}

/** \brief Read encrypted bytes from the specified slot
 *  \param[in]  slotid    The slot id for the encrypted read
 *  \param[in]  block     The block id in the specified slot
 *  \param[in]  enckeyid  The keyid of the parent encryption key
 *  \param[out] data      The 32 bytes of clear text data that was read encrypted from the slot, then decrypted
 *  \param[inout] bufsize In:Size of data buffer.  Out:Number of bytes read
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_enc_read(uint8_t slotid, uint8_t block, uint8_t enckeyid, uint8_t* data, int16_t* bufsize)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t enckey[ATCA_KEY_SIZE] = { 0 };

	do {
		// Verify input parameters
		if (data == NULL || bufsize == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Get the encryption key from the platform
		if ((status = atcatls_get_enckey(enckey)) != ATCA_SUCCESS) BREAK(status, "Get encryption key failed");

		// Memset the input data buffer
		memset(data, 0x00, *bufsize);

		// todo: implement to account for the correct block on the ECC508
		if ((status = atcab_read_enc(slotid, block, data, enckey, enckeyid)) != ATCA_SUCCESS) BREAK(status, "Read encrypted failed");

	} while (0);

	return status;
}

/** \brief Write encrypted bytes to the specified slot
 *  \param[in]  slotid    The slot id for the encrypted write
 *  \param[in]  block     The block id in the specified slot
 *  \param[in]  enckeyid  The keyid of the parent encryption key
 *  \param[out] data      The 32 bytes of clear text data that will be encrypted to write to the slot.
 *  \param[in] bufsize    Size of data buffer.
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_enc_write(uint8_t slotid, uint8_t block, uint8_t enckeyid, uint8_t* data, int16_t bufsize)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t enckey[ATCA_KEY_SIZE] = { 0 };

	do {
		// Verify input parameters
		if (data == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Get the encryption key from the platform
		if ((status = atcatls_get_enckey(enckey)) != ATCA_SUCCESS) BREAK(status, "Get encryption key failed");

		// todo: implement to account for the correct block on the ECC508
		if ((status = atcab_write_enc(slotid, block, data, enckey, enckeyid)) != ATCA_SUCCESS) BREAK(status, "Write encrypted failed");

	} while (0);

	return status;
}

/** \brief Read a private RSA key from the device.  The read will be encrypted
 *  \param[in]  enckeyid  The keyid of the parent encryption key
 *  \param[out] rsakey    The RSA key bytes
 *  \param[inout] keysize Size of RSA key.
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_enc_rsakey_read(uint8_t enckeyid, uint8_t* rsakey, int16_t* keysize)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t enckey[ATCA_KEY_SIZE] = { 0 };
	uint8_t slotid = RSA_KEY_SLOT;
	uint8_t startBlock = RSA_KEY_START_BLOCK;
	uint8_t memBlock = 0;
	uint8_t numKeyBlocks = RSA2048_KEY_SIZE / ATCA_BLOCK_SIZE;
	uint8_t block = 0;
	uint8_t memLoc = 0;

	do {
		// Verify input parameters
		if (rsakey == NULL || keysize == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		if (*keysize < RSA2048_KEY_SIZE) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "RSA key buffer too small");
		}

		// Get the encryption key from the platform
		if ((status = atcatls_get_enckey(enckey)) != ATCA_SUCCESS) BREAK(status, "Get encryption key failed");

		// Read the RSA key by blocks
		for (memBlock = 0; memBlock < numKeyBlocks; memBlock++) {
			block = startBlock + memBlock;
			memLoc = ATCA_BLOCK_SIZE * memBlock;
			if ((status = atcab_read_enc(slotid, block, &rsakey[memLoc], enckey, enckeyid)) != ATCA_SUCCESS) BREAK(status, "Read RSA failed");
		}
		*keysize = RSA2048_KEY_SIZE;

	} while (0);

	return status;
}

/** \brief Write a private RSA key from the device.  The write will be encrypted
 *  \param[in] enckeyid  The keyid of the parent encryption key
 *  \param[in] rsakey    The RSA key bytes
 *  \param[in] keysize    Size of RSA key.
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_enc_rsakey_write(uint8_t enckeyid, uint8_t* rsakey, int16_t keysize)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t enckey[ATCA_KEY_SIZE] = { 0 };
	uint8_t slotid = RSA_KEY_SLOT;
	uint8_t startBlock = RSA_KEY_START_BLOCK;
	uint8_t memBlock = 0;
	uint8_t numKeyBlocks = RSA2048_KEY_SIZE / ATCA_BLOCK_SIZE;
	uint8_t block = 0;
	uint8_t memLoc = 0;

	do {
		// Verify input parameters
		if (rsakey == NULL ) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		if (keysize < RSA2048_KEY_SIZE) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "RSA key buffer too small");
		}

		// Get the encryption key from the platform
		if ((status = atcatls_get_enckey(enckey)) != ATCA_SUCCESS) BREAK(status, "Get encryption key failed");

		// Read the RSA key by blocks
		for (memBlock = 0; memBlock < numKeyBlocks; memBlock++) {
			block = startBlock + memBlock;
			memLoc = ATCA_BLOCK_SIZE * memBlock;
			if ((status = atcab_write_enc(slotid, block, &rsakey[memLoc], enckey, enckeyid)) != ATCA_SUCCESS) BREAK(status, "Read RSA failed");
		}

	} while (0);

	return status;
}

/** \brief Write a public key from the device.
 *  \param[in] slotid The slot ID to write to
 *  \param[in] pubkey The key bytes
 *  \param[in] lock   If true, lock the slot after writing these bytes.
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_write_pubkey(uint8_t slotid, uint8_t pubkey[ATCA_PUB_KEY_SIZE], bool lock)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t lock_response = 0x00;

	do {
		// Write the buffer as a public key into the specified slot
		if ((status = atcab_write_pubkey(slotid, pubkey)) != ATCA_SUCCESS) BREAK(status, "Write of public key slot failed");


		// Lock the slot if indicated
		if (lock == true)
			if ((status = atcab_lock_data_slot(slotid, &lock_response)) != ATCA_SUCCESS) BREAK(status, "Lock public key slot failed");

	} while (0);

	return status;

}

/** \brief Write a public key from the device.
 *  \param[in] slotid   The slot ID to write to
 *  \param[in] caPubkey The key bytes
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_read_ca_pubkey(uint8_t slotid, uint8_t caPubkey[ATCA_PUB_KEY_SIZE])
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	do {
		// Read public key from the specified slot and return it in the buffer provided
		if ((status = atcab_read_pubkey(slotid, caPubkey)) != ATCA_SUCCESS) BREAK(status, "Read of public key slot failed");


	} while (0);

	return status;
}

/**
 * \brief Reads the certificate specified by the certificate definition from the ATECC508A device.
 *        This process involves reading the dynamic cert data from the device and combining it
 *        with the template found in the certificate definition. Return the certificate int der format
 * \param[in] cert_def Certificate definition describing where to find the dynamic certificate information
 *                     on the device and how to incorporate it into the template.
 * \param[in] ca_public_key The ECC P256 public key of the certificate authority that signed this certificate.
 *                          Formatted as the 32 byte X and Y integers concatenated together (64 bytes total).
 * \param[out] certout Buffer to received the certificate.
 * \param[inout] certsize As input, the size of the cert buffer in bytes.
 *                         As output, the size of the certificate returned in cert in bytes.
 * \return ATCA_STATUS
 */
ATCA_STATUS atcatls_get_cert(const atcacert_def_t* cert_def, const uint8_t *ca_public_key, uint8_t *certout, size_t* certsize)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (certout == NULL || certsize == NULL || cert_def == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}

		// Build a certificate with signature and public key
		status = atcacert_read_cert(cert_def, ca_public_key, certout, certsize);
		if (status != ATCACERT_E_SUCCESS)
			BREAK(status, "Failed to read certificate");

	} while (0);

	return status;
}

/**
 * \brief Creates a CSR specified by the CSR definition from the ATECC508A device.
 *        This process involves reading the dynamic CSR data from the device and combining it
 *        with the template found in the CSR definition, then signing it. Return the CSR int der format
 * \param[in] csr_def CSR definition describing where to find the dynamic CSR information
 *                     on the device and how to incorporate it into the template.
 * \param[out] csrout Buffer to received the CSR.
 * \param[inout] csrsize As input, the size of the CSR buffer in bytes.
 *                         As output, the size of the CSR returned in cert in bytes.
 * \return ATCA_STATUS
 */
ATCA_STATUS atcatls_create_csr(const atcacert_def_t* csr_def, char *csrout, size_t* csrsize)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do
	{
		// Verify input parameters
		if (csrout == NULL || csrsize == NULL || csr_def == NULL)
		{
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}

		// Build a certificate with signature and public key
		status = atcacert_create_csr_pem(csr_def, csrout, csrsize);
		if (status != ATCACERT_E_SUCCESS) BREAK(status, "Failed to create CSR");

	} while (0);

	return status;

}

