/**
 * \file
 * \brief Host side methods to support CryptoAuth computations
 *
 * \copyright Copyright (c) 2015 Atmel Corporation. All rights reserved.
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

#include "atca_host.h"
#include "crypto/atca_crypto_sw_sha2.h"


/** \brief This function copies otp and sn data into a command buffer.
 *
 * \param[in, out] param pointer to parameter structure
 * \return pointer to command buffer byte that was copied last
 */
uint8_t *atcah_include_data(struct atca_include_data_in_out *param)
{
	if (param->mode & MAC_MODE_INCLUDE_OTP_88) {
		memcpy(param->p_temp, param->otp, ATCA_OTP_SIZE_8 + ATCA_OTP_SIZE_3);            // use OTP[0:10], Mode:5 is overridden
		param->p_temp += ATCA_OTP_SIZE_8 + ATCA_OTP_SIZE_3;
	}else {
		if (param->mode & MAC_MODE_INCLUDE_OTP_64)
			memcpy(param->p_temp, param->otp, ATCA_OTP_SIZE_8);         // use 8 bytes OTP[0:7] for (6)
		else
			memset(param->p_temp, 0, ATCA_OTP_SIZE_8);                  // use 8 zeros for (6)
		param->p_temp += ATCA_OTP_SIZE_8;

		memset(param->p_temp, 0, ATCA_OTP_SIZE_3);                     // use 3 zeros for (7)
		param->p_temp += ATCA_OTP_SIZE_3;
	}

	// (8) 1 byte SN[8] = 0xEE
	*param->p_temp++ = ATCA_SN_8;

	// (9) 4 bytes SN[4:7] or zeros
	if (param->mode & MAC_MODE_INCLUDE_SN)
		memcpy(param->p_temp, &param->sn[4], ATCA_SN_SIZE_4);           //use SN[4:7] for (9)
	else
		memset(param->p_temp, 0, ATCA_SN_SIZE_4);                       //use zeros for (9)
	param->p_temp += ATCA_SN_SIZE_4;

	// (10) 2 bytes SN[0:1] = 0x0123
	*param->p_temp++ = ATCA_SN_0;
	*param->p_temp++ = ATCA_SN_1;

	// (11) 2 bytes SN[2:3] or zeros
	if (param->mode & MAC_MODE_INCLUDE_SN)
		memcpy(param->p_temp, &param->sn[2], ATCA_SN_SIZE_2);           //use SN[2:3] for (11)
	else
		memset(param->p_temp, 0, ATCA_SN_SIZE_2);                       //use zeros for (9)
	param->p_temp += ATCA_SN_SIZE_2;

	return param->p_temp;
}

/** \brief This function calculates a 32-byte nonce based on a 20-byte input value (param->num_in) and 32-byte random number (param->rand_out).

   This nonce will match with the nonce generated in the device when executing a Nonce command.
   To use this function, an application first sends a Nonce command with a chosen param->num_in to the device.
   Nonce Mode parameter must be set to use random nonce (mode 0 or 1).\n
   The device generates a nonce, stores it in its TempKey, and outputs the random number param->rand_out it used in the hash calculation to the host.
   The values of param->rand_out and param->num_in are passed to this nonce calculation function. The function calculates the nonce and returns it.
   This function can also be used to fill in the nonce directly to TempKey (pass-through mode). The flags will automatically be set according to the mode used.
    \param[in, out] param pointer to parameter structure
    \return status of the operation
 */
ATCA_STATUS atcah_nonce(struct atca_nonce_in_out *param)
{
	uint8_t temporary[ATCA_MSG_SIZE_NONCE];
	uint8_t *p_temp;

	// Check parameters
	if (!param->temp_key || !param->num_in || (param->mode > NONCE_MODE_PASSTHROUGH) || (param->mode == NONCE_MODE_INVALID)
	    || (((param->mode == NONCE_MODE_SEED_UPDATE || (param->mode == NONCE_MODE_NO_SEED_UPDATE)) && !param->rand_out)))
		return ATCA_BAD_PARAM;

	// Calculate or pass-through the nonce to TempKey->Value
	if ((param->mode == NONCE_MODE_SEED_UPDATE) || (param->mode == NONCE_MODE_NO_SEED_UPDATE)) {
		// Calculate nonce using SHA-256 (refer to data sheet)
		p_temp = temporary;

		memcpy(p_temp, param->rand_out, NONCE_RSP_SIZE_LONG - ATCA_PACKET_OVERHEAD);
		p_temp += NONCE_RSP_SIZE_LONG - ATCA_PACKET_OVERHEAD;

		memcpy(p_temp, param->num_in, NONCE_NUMIN_SIZE);
		p_temp += NONCE_NUMIN_SIZE;

		*p_temp++ = ATCA_NONCE;
		*p_temp++ = param->mode;
		*p_temp++ = 0x00;

		// Calculate SHA256 to get the nonce
		atcah_sha256(ATCA_MSG_SIZE_NONCE, temporary, param->temp_key->value);

		// Update TempKey->SourceFlag to 0 (random)
		param->temp_key->source_flag = 0;
	}else if (param->mode == NONCE_MODE_PASSTHROUGH) {
		// Pass-through mode
		memcpy(param->temp_key->value, param->num_in, NONCE_NUMIN_SIZE_PASSTHROUGH);

		// Update TempKey->SourceFlag to 1 (not random)
		param->temp_key->source_flag = 1;
	}

	// Update TempKey fields
	param->temp_key->key_id = 0;
	param->temp_key->gen_data = 0;
	param->temp_key->check_flag = 0;
	param->temp_key->valid = 1;

	return ATCA_SUCCESS;
}


/** \brief This function generates an SHA-256 digest (MAC) of a key, challenge, and other information.

   The resulting digest will match with the one generated by the device when executing a MAC command.
   The TempKey (if used) should be valid (temp_key.valid = 1) before executing this function.

 * \param[in, out] param pointer to parameter structure
 * \return status of the operation
 */
ATCA_STATUS atcah_mac(struct atca_mac_in_out *param)
{
	uint8_t temporary[ATCA_MSG_SIZE_MAC];
	uint8_t *p_temp;
	struct atca_include_data_in_out include_data;

	// Initialize struct
	include_data.otp = param->otp;
	include_data.sn = param->sn;
	include_data.mode = param->mode;

	// Check parameters
	if (!param->response
	    || (param->mode & ~MAC_MODE_MASK)
	    || (!(param->mode & MAC_MODE_BLOCK1_TEMPKEY) && !param->key)
	    || (!(param->mode & MAC_MODE_BLOCK2_TEMPKEY) && !param->challenge)
	    || ((param->mode & MAC_MODE_USE_TEMPKEY_MASK) && !param->temp_key)
	    || (((param->mode & MAC_MODE_INCLUDE_OTP_64) || (param->mode & MAC_MODE_INCLUDE_OTP_88)) && !param->otp)
	    || ((param->mode & MAC_MODE_INCLUDE_SN) && !param->sn)
	    )
		return ATCA_BAD_PARAM;

	// Check TempKey fields validity if TempKey is used
	if (((param->mode & MAC_MODE_USE_TEMPKEY_MASK) != 0)
	    // TempKey.CheckFlag must be 0 and TempKey.Valid must be 1
	    && (param->temp_key->check_flag || (param->temp_key->valid != 1)
	    // If either mode parameter bit 0 or bit 1 are set, mode parameter bit 2 must match temp_key.source_flag.
	    // Logical not (!) is used to evaluate the expression to TRUE / FALSE first before comparison (!=).
	        || (!(param->mode & MAC_MODE_SOURCE_FLAG_MATCH) != !(param->temp_key->source_flag)))
	    ) {
		// Invalidate TempKey, then return
		param->temp_key->valid = 0;
		return ATCA_EXECUTION_ERROR;
	}

	// Start calculation
	p_temp = temporary;

	// (1) first 32 bytes
	memcpy(p_temp, param->mode & MAC_MODE_BLOCK1_TEMPKEY ? param->temp_key->value : param->key, ATCA_KEY_SIZE);                // use Key[KeyID]
	p_temp += ATCA_KEY_SIZE;

	// (2) second 32 bytes
	memcpy(p_temp, param->mode & MAC_MODE_BLOCK2_TEMPKEY ? param->temp_key->value : param->challenge, ATCA_KEY_SIZE);          // use Key[KeyID]
	p_temp += ATCA_KEY_SIZE;

	// (3) 1 byte opcode
	*p_temp++ = ATCA_MAC;

	// (4) 1 byte mode parameter
	*p_temp++ = param->mode;

	// (5) 2 bytes keyID
	*p_temp++ = param->key_id & 0xFF;
	*p_temp++ = (param->key_id >> 8) & 0xFF;

	include_data.p_temp = p_temp;
	atcah_include_data(&include_data);

	// Calculate SHA256 to get the MAC digest
	atcah_sha256(ATCA_MSG_SIZE_MAC, temporary, param->response);

	// Update TempKey fields
	if (param->temp_key)
		param->temp_key->valid = 0;

	return ATCA_SUCCESS;
}


/** \brief This function calculates a SHA-256 digest (MAC) of a password and other information, to be verified using the CheckMac device command.

   This password checking operation is described in Atmel ATSHA204 [DATASHEET].
   Before performing password checking operation, TempKey should contain a randomly generated nonce. The TempKey in the device has to match the one in the application.
   A user enters the password to be verified by an application.
   The application passes this password to the CheckMac calculation function, along with 13 bytes of OtherData, a 32-byte target key, and optionally 11 bytes of OTP.
   The function calculates a 32-byte ClientResp, returns it to Application. The function also replaces the current TempKey value with the target key.
   The application passes the calculated ClientResp along with OtherData inside a CheckMac command to the device.
   The device validates ClientResp, and copies the target slot to its TempKey.

   If the password is stored in an odd numbered slot, the target slot is the password slot itself, so the target_key parameter should point to the password being checked.
   If the password is stored in an even numbered slot, the target slot is the next odd numbered slot (KeyID + 1), so the target_key parameter should point to a key that
   is equal to the target slot in the device.

   Note that the function does not check the result of the password checking operation.
   Regardless of whether the CheckMac command returns success or not, the TempKey variable of the application will hold the value of the target key.
   Therefore the application has to make sure that password checking operation succeeds before using the TempKey for subsequent operations.

 * \param[in, out] param pointer to parameter structure
 * \return status of the operation
 */
ATCA_STATUS atcah_check_mac(struct atca_check_mac_in_out *param)
{
	uint8_t temporary[ATCA_MSG_SIZE_MAC];
	uint8_t *p_temp;

	// Check parameters
	if (((param->mode & MAC_MODE_USE_TEMPKEY_MASK) != MAC_MODE_BLOCK2_TEMPKEY)
	    || !param->password || !param->other_data
	    || !param->target_key || !param->client_resp || !param->temp_key
	    || ((param->mode & MAC_MODE_INCLUDE_OTP_64) && !param->otp)
	    )
		return ATCA_BAD_PARAM;

	// Check TempKey fields validity (TempKey is always used)
	// TempKey.CheckFlag must be 0, TempKey.Valid must be 1, TempKey.SourceFlag must be 0
	if (param->temp_key->check_flag || (param->temp_key->valid != 1) || param->temp_key->source_flag) {
		// Invalidate TempKey, then return
		param->temp_key->valid = 0;
		return ATCA_EXECUTION_ERROR;
	}

	// Start calculation
	p_temp = temporary;

	// (1) first 32 bytes
	memcpy(p_temp, param->password, ATCA_KEY_SIZE);           // password to be checked
	p_temp += ATCA_KEY_SIZE;

	// (2) second 32 bytes
	memcpy(p_temp, param->temp_key->value, ATCA_KEY_SIZE);    // use TempKey.Value
	p_temp += ATCA_KEY_SIZE;

	// (3, 4, 5) 4 byte OtherData[0:3]
	memcpy(p_temp, &param->other_data[0], ATCA_OTHER_DATA_SIZE_4);
	p_temp += ATCA_OTHER_DATA_SIZE_4;

	// (6) 8 bytes OTP[0:7] or 0x00's
	if (param->mode & MAC_MODE_INCLUDE_OTP_64)
		memcpy(p_temp, param->otp, ATCA_OTP_SIZE_8);    // use 8 bytes OTP[0:7] for (6)
	else
		memset(p_temp, 0, ATCA_OTP_SIZE_8);             // use 8 zeros for (6)
	p_temp += ATCA_OTP_SIZE_8;

	// (7) 3 byte OtherData[4:6]
	memcpy(p_temp, &param->other_data[ATCA_OTHER_DATA_SIZE_4], ATCA_OTHER_DATA_SIZE_3);  // use OtherData[4:6] for (7)
	p_temp += ATCA_OTHER_DATA_SIZE_3;

	// (8) 1 byte SN[8] = 0xEE
	*p_temp++ = ATCA_SN_8;

	// (9) 4 byte OtherData[7:10]
	memcpy(p_temp, &param->other_data[ATCA_OTHER_DATA_SIZE_4 + ATCA_OTHER_DATA_SIZE_3], ATCA_OTHER_DATA_SIZE_4);  // use OtherData[7:10] for (9)
	p_temp += ATCA_OTHER_DATA_SIZE_4;

	// (10) 2 bytes SN[0:1] = 0x0123
	*p_temp++ = ATCA_SN_0;
	*p_temp++ = ATCA_SN_1;

	// (11) 2 byte OtherData[11:12]
	memcpy(p_temp, &param->other_data[ATCA_OTHER_DATA_SIZE_4 + ATCA_OTHER_DATA_SIZE_3 + ATCA_OTHER_DATA_SIZE_2],
	       ATCA_OTHER_DATA_SIZE_2);  // use OtherData[11:12] for (11)
	p_temp += ATCA_OTHER_DATA_SIZE_2;

	// Calculate SHA256 to get the MAC digest
	atcah_sha256(ATCA_MSG_SIZE_MAC, temporary, param->client_resp);

	// Update TempKey fields
	memcpy(param->temp_key->value, param->target_key, ATCA_KEY_SIZE);
	param->temp_key->gen_data = 0;
	param->temp_key->source_flag = 1;
	param->temp_key->valid = 1;

	return ATCA_SUCCESS;
}


/** \brief This function generates an HMAC / SHA-256 hash of a key and other information.

   The resulting hash will match with the one generated in the device by an HMAC command.
   The TempKey has to be valid (temp_key.valid = 1) before executing this function.

 * \param[in, out] param pointer to parameter structure
 * \return status of the operation
 */
ATCA_STATUS atcah_hmac(struct atca_hmac_in_out *param)
{
	// Local Variables
	struct atca_include_data_in_out include_data;
	uint8_t temporary[ATCA_MSG_SIZE_HMAC_INNER];
	uint8_t i;
	uint8_t *p_temp;

	// Initialize struct
	include_data.otp = param->otp;
	include_data.sn = param->sn;
	include_data.mode = param->mode;

	// Check parameters
	if (!param->response || !param->key || !param->temp_key
	    || (param->mode & ~HMAC_MODE_MASK)
	    || (((param->mode & MAC_MODE_INCLUDE_OTP_64) || (param->mode & MAC_MODE_INCLUDE_OTP_88)) && !param->otp)
	    || ((param->mode & MAC_MODE_INCLUDE_SN) && !param->sn)
	    )
		return ATCA_BAD_PARAM;

	// Check TempKey fields validity (TempKey is always used)
	if (    // TempKey.CheckFlag must be 0 and TempKey.Valid must be 1
	    param->temp_key->check_flag || (param->temp_key->valid != 1)
	        // The mode parameter bit 2 must match temp_key.source_flag.
	        // Logical not (!) is used to evaluate the expression to TRUE / FALSE first before comparison (!=).
	    || (!(param->mode & MAC_MODE_SOURCE_FLAG_MATCH) != !(param->temp_key->source_flag))
	    ) {
		// Invalidate TempKey, then return
		param->temp_key->valid = 0;
		return ATCA_EXECUTION_ERROR;
	}

	// Start first calculation (inner)
	p_temp = temporary;

	// Refer to fips-198a.pdf, length Key = 32 bytes, Block size = 512 bits = 64 bytes.
	// So the Key must be padded with zeros.
	// XOR K0 with ipad, then append
	for (i = 0; i < ATCA_KEY_SIZE; i++)
		*p_temp++ = param->key[i] ^ 0x36;

	// XOR the remaining zeros and append
	for (i = 0; i < HMAC_BLOCK_SIZE - ATCA_KEY_SIZE; i++)
		*p_temp++ = 0 ^ 0x36;

	// Next append the stream of data 'text'
	// (1) first 32 bytes: zeros
	memset(p_temp, 0, HMAC_BLOCK_SIZE - ATCA_KEY_SIZE);
	p_temp += HMAC_BLOCK_SIZE - ATCA_KEY_SIZE;

	// (2) second 32 bytes: TempKey
	memcpy(p_temp, param->temp_key->value, ATCA_KEY_SIZE);
	p_temp += ATCA_KEY_SIZE;

	// (3) 1 byte opcode
	*p_temp++ = ATCA_HMAC;

	// (4) 1 byte mode parameter
	*p_temp++ = param->mode;

	// (5) 2 bytes keyID
	*p_temp++ = param->key_id & 0xFF;
	*p_temp++ = (param->key_id >> 8) & 0xFF;

	// Calculate SHA256
	// H((K0^ipad):text), use param.response for temporary storage
	atcah_sha256(ATCA_MSG_SIZE_HMAC_INNER, temporary, param->response);

	// Start second calculation (outer)
	p_temp = temporary;

	// XOR K0 with opad, then append
	for (i = 0; i < ATCA_KEY_SIZE; i++)
		*p_temp++ = param->key[i] ^ 0x5C;

	// XOR the remaining zeros and append
	for (i = 0; i < HMAC_BLOCK_SIZE - ATCA_KEY_SIZE; i++)
		*p_temp++ = 0 ^ 0x5C;

	// Append result from last calculation H((K0 ^ ipad): text)
	memcpy(p_temp, param->response, ATCA_KEY_SIZE);
	p_temp += ATCA_KEY_SIZE;

	include_data.p_temp = p_temp;
	atcah_include_data(&include_data);

	// Calculate SHA256 to get the resulting HMAC
	atcah_sha256(ATCA_MSG_SIZE_HMAC, temporary, param->response);

	// Update TempKey fields
	param->temp_key->valid = 0;

	return ATCA_SUCCESS;
}


/** \brief This function combines the current TempKey with a stored value.

   The stored value can be a data slot, OTP page, configuration zone, or hardware transport key.
   The TempKey generated by this function will match with the TempKey in the device generated
   when executing a GenDig command.
   The TempKey should be valid (temp_key.valid = 1) before executing this function.
   To use this function, an application first sends a GenDig command with a chosen stored value to the device.
   This stored value must be known by the application and is passed to this GenDig calculation function.
   The function calculates a new TempKey and returns it.

 * \param[in, out] param pointer to parameter structure
 * \return status of the operation
 */
ATCA_STATUS atcah_gen_dig(struct atca_gen_dig_in_out *param)
{
	uint8_t temporary[ATCA_MSG_SIZE_GEN_DIG];
	uint8_t *p_temp;

	// Check parameters
	if (!param->stored_value || !param->temp_key
	    || ((param->zone != GENDIG_ZONE_OTP)
	        && (param->zone != GENDIG_ZONE_DATA)
	        && (param->zone != GENDIG_ZONE_CONFIG))
	    )
		return ATCA_BAD_PARAM;

	// Check TempKey fields validity (TempKey is always used)
	if ( // TempKey.CheckFlag must be 0 and TempKey.Valid must be 1
	    param->temp_key->check_flag || (param->temp_key->valid != 1)
	    ) {
		// Invalidate TempKey, then return
		param->temp_key->valid = 0;
		return ATCA_EXECUTION_ERROR;
	}

	// Start calculation
	p_temp = temporary;

	// (1) 32 bytes inputKey
	//     (Config[KeyID] or OTP[KeyID] or Data.slot[KeyID] or TransportKey[KeyID])
	memcpy(p_temp, param->stored_value, ATCA_KEY_SIZE);
	p_temp += ATCA_KEY_SIZE;

	// (2) 1 byte Opcode
	*p_temp++ = ATCA_GENDIG;

	// (3) 1 byte Param1 (zone)
	*p_temp++ = param->zone;

	// (4) 2 bytes Param2 (keyID)
	*p_temp++ = param->key_id & 0xFF;
	*p_temp++ = (param->key_id >> 8) & 0xFF;

	// (5) 1 byte SN[8] = 0xEE
	*p_temp++ = ATCA_SN_8;

	// (6) 2 bytes SN[0:1] = 0x0123
	*p_temp++ = ATCA_SN_0;
	*p_temp++ = ATCA_SN_1;

	// (7) 25 zeros
	memset(p_temp, 0, ATCA_GENDIG_ZEROS_SIZE);
	p_temp += ATCA_GENDIG_ZEROS_SIZE;

	// (8) 32 bytes TempKey
	memcpy(p_temp, param->temp_key->value, ATCA_KEY_SIZE);

	// Calculate SHA256 to get the new TempKey
	atcah_sha256(ATCA_MSG_SIZE_GEN_DIG, temporary, param->temp_key->value);

	// Update TempKey fields
	param->temp_key->valid = 1;

	if ((param->zone == GENDIG_ZONE_DATA) && (param->key_id <= 15)) {
		param->temp_key->gen_data = 1;
		param->temp_key->key_id = (param->key_id & 0xF);    // mask lower 4-bit only
	}else {
		param->temp_key->gen_data = 0;
		param->temp_key->key_id = 0;
	}

	return ATCA_SUCCESS;
}

/** \brief This function combines the session key with a plain text.
 * \param[in, out] param pointer to parameter structure
 * \return status of the operation
 */
ATCA_STATUS atcah_gen_mac(struct atca_gen_dig_in_out *param)
{
	uint8_t temporary[ATCA_MSG_SIZE_GEN_DIG];
	uint8_t *p_temp;

	// Check parameters
	if (!param->stored_value || !param->temp_key)
		return ATCA_BAD_PARAM;

	// Check TempKey fields validity (TempKey is always used)
	if ( // TempKey.CheckFlag must be 0 and TempKey.Valid must be 1
	    param->temp_key->check_flag || (param->temp_key->valid != 1)
	    ) {
		// Invalidate TempKey, then return
		param->temp_key->valid = 0;
		return ATCA_EXECUTION_ERROR;
	}

	// Start calculation
	p_temp = temporary;

	// (1) 32 bytes SessionKey
	//     (Config[KeyID] or OTP[KeyID] or Data.slot[KeyID] or TransportKey[KeyID])
	memcpy(p_temp, param->temp_key->value, ATCA_KEY_SIZE);
	p_temp += ATCA_KEY_SIZE;

	// (2) 1 byte Opcode
	*p_temp++ = ATCA_WRITE;

	// (3) 1 byte Param1 (zone)
	*p_temp++ = param->zone;

	// (4) 2 bytes Param2 (keyID)
	*p_temp++ = param->key_id & 0xFF;
	*p_temp++ = (param->key_id >> 8) & 0xFF;

	// (5) 1 byte SN[8] = 0xEE
	*p_temp++ = ATCA_SN_8;

	// (6) 2 bytes SN[0:1] = 0x0123
	*p_temp++ = ATCA_SN_0;
	*p_temp++ = ATCA_SN_1;

	// (7) 25 zeros
	memset(p_temp, 0, ATCA_GENDIG_ZEROS_SIZE);
	p_temp += ATCA_GENDIG_ZEROS_SIZE;

	// (8) 32 bytes PlainText
	memcpy(p_temp, param->stored_value, ATCA_KEY_SIZE);

	// Calculate SHA256 to get the new TempKey
	atcah_sha256(ATCA_MSG_SIZE_GEN_DIG, temporary, param->temp_key->value);

	// Update TempKey fields
	param->temp_key->valid = 1;

	if ((param->zone == GENDIG_ZONE_DATA) && (param->key_id <= 15)) {
		param->temp_key->gen_data = 1;
		param->temp_key->key_id = (param->key_id & 0xF);    // mask lower 4-bit only
	}else {
		param->temp_key->gen_data = 0;
		param->temp_key->key_id = 0;
	}

	return ATCA_SUCCESS;
}

/** \brief This function calculates the input MAC for the PrivWrite command.

   The PrivWrite command will need an input MAC if SlotConfig.WriteConfig.Encrypt is set.

 * \param[in, out] param pointer to parameter structure
 * \return status of the operation
 */
ATCA_STATUS atcah_write_auth_mac(struct atca_write_mac_in_out *param)
{
	uint8_t temporary[ATCA_MSG_SIZE_PRIVWRITE_MAC];
	uint8_t i;
	uint8_t *p_temp;
	uint8_t session_key2[32];

	// Check parameters
	if (!param->input_data || !param->temp_key)
		return ATCA_BAD_PARAM;

	// Check TempKey fields validity (TempKey is always used)
	if ( // TempKey.CheckFlag must be 0 and TempKey.Valid must be 1
	    param->temp_key->check_flag || (param->temp_key->valid != 1)
	    ) {
		// Invalidate TempKey, then return
		param->temp_key->valid = 0;
		return ATCA_EXECUTION_ERROR;
	}
	// Encrypt by XOR-ing Data with the TempKey
	for (i = 0; i < 32; i++)
		param->encrypted_data[i] = param->encryption_key[i] ^ param->temp_key->value[i];

	// Calculate the new tempkey
	atcah_sha256(32, param->temp_key->value, session_key2);

	// If the pointer *mac is provided by the caller then calculate input MAC
	if (param->auth_mac) {
		// Start calculation
		p_temp = temporary;

		// (1) 32 bytes TempKey
		memcpy(p_temp, param->temp_key->value, ATCA_KEY_SIZE);
		p_temp += ATCA_KEY_SIZE;

		// (2) 1 byte Opcode
		*p_temp++ = ATCA_PRIVWRITE;

		// (3) 1 byte Param1 (zone)
		*p_temp++ = param->zone;

		// (4) 2 bytes Param2 (keyID)
		*p_temp++ = param->key_id & 0xFF;
		*p_temp++ = (param->key_id >> 8) & 0xFF;

		// (5) 1 byte SN[8] = 0xEE
		*p_temp++ = ATCA_SN_8;

		// (6) 2 bytes SN[0:1] = 0x0123
		*p_temp++ = ATCA_SN_0;
		*p_temp++ = ATCA_SN_1;

		// (7) 21 zeros
		memset(p_temp, 0, ATCA_PRIVWRITE_MAC_ZEROS_SIZE);
		p_temp += ATCA_PRIVWRITE_MAC_ZEROS_SIZE;

		// (8) 36 bytes PlainText : 0 0 0 0 (4bytes) | private_key (32bytes)
		memcpy(p_temp, param->input_data, ATCA_PLAIN_TEXT_SIZE);

		// Calculate SHA256 to get the new TempKey
		atcah_sha256(ATCA_MSG_SIZE_PRIVWRITE_MAC, temporary, param->auth_mac);
	}
	// Update TempKey fields
	param->temp_key->valid = 1;

	return ATCA_SUCCESS;
}

/** \brief This function calculates the input MAC for the PrivWrite command.

   The PrivWrite command will need an input MAC if SlotConfig.WriteConfig.Encrypt is set.

 * \param[in, out] param pointer to parameter structure
 * \return status of the operation
 */
ATCA_STATUS atcah_privwrite_auth_mac(struct atca_write_mac_in_out *param)
{
	uint8_t temporary[ATCA_MSG_SIZE_PRIVWRITE_MAC];
	uint8_t i;
	uint8_t *p_temp;
	uint8_t session_key2[32];

	// Check parameters
	if (!param->input_data || !param->temp_key)
		return ATCA_BAD_PARAM;

	// Check TempKey fields validity (TempKey is always used)
	if ( // TempKey.CheckFlag must be 0 and TempKey.Valid must be 1
	    param->temp_key->check_flag || (param->temp_key->valid != 1)
	    ) {
		// Invalidate TempKey, then return
		param->temp_key->valid = 0;
		return ATCA_EXECUTION_ERROR;
	}


	/* Encrypt by XOR-ing Data with the TempKey
	 */

	// Encrypt the first 4 bytes of the cipher text, which should be 0s
	for (i = 0; i < 4; i++)
		param->encrypted_data[i] = 0 ^ param->temp_key->value[i];

	// Encrypt the next 28 bytes of the cipher text, which is the first part of the private key.
	for (i = 4; i < 32; i++)
		param->encrypted_data[i] = param->encryption_key[i - 4] ^ param->temp_key->value[i];

	// Calculate the new key for the last 4 bytes of the cipher text
	atcah_sha256(32, param->temp_key->value, session_key2);

	// Encrypt the last 4 bytes of the cipher text, which is the remaining part of the private key
	for (i = 32; i < 36; i++)
		param->encrypted_data[i] = param->encryption_key[i - 4] ^ session_key2[i - 32];

	// If the pointer *mac is provided by the caller then calculate input MAC
	if (param->auth_mac) {
		// Start calculation
		p_temp = temporary;

		// (1) 32 bytes TempKey
		memcpy(p_temp, param->temp_key->value, ATCA_KEY_SIZE);
		p_temp += ATCA_KEY_SIZE;

		// (2) 1 byte Opcode
		*p_temp++ = ATCA_PRIVWRITE;

		// (3) 1 byte Param1 (zone)
		*p_temp++ = param->zone;

		// (4) 2 bytes Param2 (keyID)
		*p_temp++ = param->key_id & 0xFF;
		*p_temp++ = (param->key_id >> 8) & 0xFF;

		// (5) 1 byte SN[8] = 0xEE
		*p_temp++ = ATCA_SN_8;

		// (6) 2 bytes SN[0:1] = 0x0123
		*p_temp++ = ATCA_SN_0;
		*p_temp++ = ATCA_SN_1;

		// (7) 21 zeros
		memset(p_temp, 0, ATCA_PRIVWRITE_MAC_ZEROS_SIZE);
		p_temp += ATCA_PRIVWRITE_MAC_ZEROS_SIZE;

		// (8) 36 bytes PlainText : 0 0 0 0 (4bytes) | private_key (32bytes)
		memcpy(p_temp, param->input_data, ATCA_PLAIN_TEXT_SIZE);

		// Calculate SHA256 to get the new TempKey
		atcah_sha256(ATCA_MSG_SIZE_PRIVWRITE_MAC, temporary, param->auth_mac);
	}

	// Update TempKey fields
	param->temp_key->valid = 1;

	return ATCA_SUCCESS;
}

/** \brief This function combines a key with the TempKey.

   Used in conjunction with DeriveKey command, the key derived by this function will match the key in the device.
   Two kinds of operation are supported:
   <ul>
   <li>Roll Key operation: target_key and parent_key parameters should be set to point to the same location (TargetKey).</li>
   <li>Create Key operation: target_key should be set to point to TargetKey, parent_key should be set to point to ParentKey.</li>
   </ul>
   After executing this function, the initial value of target_key will be overwritten with the derived key.
   The TempKey should be valid (temp_key.valid = 1) before executing this function.

 * \param[in, out] param pointer to parameter structure
 * \return status of the operation
 */
ATCA_STATUS atcah_derive_key(struct atca_derive_key_in_out *param)
{
	uint8_t temporary[ATCA_MSG_SIZE_DERIVE_KEY];
	uint8_t *p_temp;

	// Check parameters
	if (!param->parent_key || !param->target_key || !param->temp_key
	    || (param->random & ~DERIVE_KEY_RANDOM_FLAG) || (param->target_key_id > ATCA_KEY_ID_MAX))
		return ATCA_BAD_PARAM;


	// Check TempKey fields validity (TempKey is always used)
	if (    // TempKey.CheckFlag must be 0 and TempKey.Valid must be 1
	    param->temp_key->check_flag || (param->temp_key->valid != 1)
	        // The random parameter bit 2 must match temp_key.source_flag
	        // Logical not (!) is used to evaluate the expression to TRUE / FALSE first before comparison (!=).
	    || (!(param->random & DERIVE_KEY_RANDOM_FLAG) != !(param->temp_key->source_flag))
	    ) {
		// Invalidate TempKey, then return
		param->temp_key->valid = 0;
		return ATCA_EXECUTION_ERROR;
	}

	// Start calculation
	p_temp = temporary;

	// (1) 32 bytes parent key
	memcpy(p_temp, param->parent_key, ATCA_KEY_SIZE);
	p_temp += ATCA_KEY_SIZE;

	// (2) 1 byte Opcode
	*p_temp++ = ATCA_DERIVE_KEY;

	// (3) 1 byte Param1 (random)
	*p_temp++ = param->random;

	// (4) 2 bytes Param2 (keyID)
	*p_temp++ = param->target_key_id & 0xFF;
	*p_temp++ = (param->target_key_id >> 8) & 0xFF;

	// (5) 1 byte SN[8] = 0xEE
	*p_temp++ = ATCA_SN_8;

	// (6) 2 bytes SN[0:1] = 0x0123
	*p_temp++ = ATCA_SN_0;
	*p_temp++ = ATCA_SN_1;

	// (7) 25 zeros
	memset(p_temp, 0, ATCA_DERIVE_KEY_ZEROS_SIZE);
	p_temp += ATCA_DERIVE_KEY_ZEROS_SIZE;

	// (8) 32 bytes TempKey
	memcpy(p_temp, param->temp_key->value, ATCA_KEY_SIZE);
	p_temp += ATCA_KEY_SIZE;

	// Calculate SHA256 to get the derived key.
	atcah_sha256(ATCA_MSG_SIZE_DERIVE_KEY, temporary, param->target_key);

	// Update TempKey fields
	param->temp_key->valid = 0;

	return ATCA_SUCCESS;
}


/** \brief This function calculates the input MAC for a DeriveKey command.

   The DeriveKey command will need an input MAC if SlotConfig[TargetKey].Bit15 is set.

 * \param[in, out] param pointer to parameter structure
 * \return status of the operation
 */
ATCA_STATUS atcah_derive_key_mac(struct atca_derive_key_mac_in_out *param)
{
	uint8_t temporary[ATCA_MSG_SIZE_DERIVE_KEY_MAC];
	uint8_t *p_temp;

	// Check parameters
	if (!param->parent_key || !param->mac || (param->random & ~DERIVE_KEY_RANDOM_FLAG)
	    || (param->target_key_id > ATCA_KEY_ID_MAX))
		return ATCA_BAD_PARAM;

	// Start calculation
	p_temp = temporary;

	// (1) 32 bytes parent key
	memcpy(p_temp, param->parent_key, ATCA_KEY_SIZE);
	p_temp += ATCA_KEY_SIZE;

	// (2) 1 byte Opcode
	*p_temp++ = ATCA_DERIVE_KEY;

	// (3) 1 byte Param1 (random)
	*p_temp++ = param->random;

	// (4) 2 bytes Param2 (keyID)
	*p_temp++ = param->target_key_id & 0xFF;
	*p_temp++ = (param->target_key_id >> 8) & 0xFF;

	// (5) 1 byte SN[8] = 0xEE
	*p_temp++ = ATCA_SN_8;

	// (6) 2 bytes SN[0:1] = 0x0123
	*p_temp++ = ATCA_SN_0;
	*p_temp++ = ATCA_SN_1;

	// Calculate SHA256 to get the input MAC for DeriveKey command
	atcah_sha256(ATCA_MSG_SIZE_DERIVE_KEY_MAC, temporary, param->mac);

	return ATCA_SUCCESS;
}


/** \brief This function encrypts 32-byte plain text data to be written using Write opcode, and optionally calculates input MAC.

   To use this function, first the nonce must be valid and synchronized between device and application.
   The application sends a GenDig command to the device, using a parent key. If the Data zone has been locked, this is
   specified by SlotConfig.WriteKey. The device updates its TempKey when executing the command.
   The application then updates its own TempKey using the GenDig calculation function, using the same key.
   The application passes the plain text data to the encryption function.\n
   If input MAC is needed the application must pass a valid pointer to buffer in the "mac" command parameter.
   If input MAC is not needed the application can pass a NULL pointer in the "mac" command parameter.
   The function encrypts the data and optionally calculates the input MAC, and returns it to the application.
   Using these encrypted data and the input MAC, the application sends a Write command to the Device. The device
   validates the MAC, then decrypts and writes the data.\n
   The encryption function does not check whether the TempKey has been generated by the correct ParentKey for the
   corresponding zone. Therefore, to get a correct result after the Data and OTP zones have been locked, the application
   has to make sure that prior GenDig calculation was done using the correct ParentKey.

 * \param[in, out] param pointer to parameter structure
 * \return status of the operation
 */
ATCA_STATUS atcah_encrypt(struct atca_encrypt_in_out *param)
{
	uint8_t temporary[ATCA_MSG_SIZE_ENCRYPT_MAC];
	uint8_t i;
	uint8_t *p_temp;

	// Check parameters
	if (!param->crypto_data || !param->temp_key || (param->zone & ~WRITE_ZONE_MASK))
		return ATCA_BAD_PARAM;

	// Check TempKey fields validity, and illegal address
	// Note that if temp_key.key_id is not checked,
	//   we cannot make sure if the key used in previous GenDig IS equal to
	//   the key pointed by SlotConfig.WriteKey in the device.
	if (    // TempKey.CheckFlag must be 0
	    param->temp_key->check_flag
	        // TempKey.Valid must be 1
	    || (param->temp_key->valid != 1)
	        // TempKey.GenData must be 1
	    || (param->temp_key->gen_data != 1)
	        // TempKey.SourceFlag must be 0 (random)
	    || param->temp_key->source_flag
	        // Illegal address
	    || (param->address & ~ATCA_ADDRESS_MASK)
	    ) {
		// Invalidate TempKey, then return
		param->temp_key->valid = 0;
		return ATCA_EXECUTION_ERROR;
	}

	// If the pointer *mac is provided by the caller then calculate input MAC
	if (param->mac) {
		// Start calculation
		p_temp = temporary;

		// (1) 32 bytes parent key
		memcpy(p_temp, param->temp_key->value, ATCA_KEY_SIZE);
		p_temp += ATCA_KEY_SIZE;

		// (2) 1 byte Opcode
		*p_temp++ = ATCA_WRITE;

		// (3) 1 byte Param1 (zone)
		*p_temp++ = param->zone;

		// (4) 2 bytes Param2 (address)
		*p_temp++ = param->address & 0xFF;
		*p_temp++ = (param->address >> 8) & 0xFF;

		// (5) 1 byte SN[8] = 0xEE
		*p_temp++ = ATCA_SN_8;

		// (6) 2 bytes SN[0:1] = 0x0123
		*p_temp++ = ATCA_SN_0;
		*p_temp++ = ATCA_SN_1;

		// (7) 25 zeros
		memset(p_temp, 0, ATCA_GENDIG_ZEROS_SIZE);
		p_temp += ATCA_GENDIG_ZEROS_SIZE;

		// (8) 32 bytes data
		memcpy(p_temp, param->crypto_data, ATCA_KEY_SIZE);

		// Calculate SHA256 to get the input MAC
		atcah_sha256(ATCA_MSG_SIZE_ENCRYPT_MAC, temporary, param->mac);
	}

	// Encrypt by XOR-ing Data with the TempKey
	for (i = 0; i < ATCA_KEY_SIZE; i++)
		param->crypto_data[i] ^= param->temp_key->value[i];

	// Update TempKey fields
	param->temp_key->valid = 0;

	return ATCA_SUCCESS;
}


/** \brief This function decrypts 32-byte encrypted data received with the Read command.

   To use this function, first the nonce must be valid and synchronized between device and application.
   The application sends a GenDig command to the Device, using a key specified by SlotConfig.ReadKey.
   The device updates its TempKey.
   The application then updates its own TempKey using the GenDig calculation function, using the same key.
   The application sends a Read command to the device for a user zone configured with EncryptRead.
   The device encrypts 32-byte zone content, and outputs it to the host.
   The application passes these encrypted data to this decryption function. The function decrypts the data and returns them.
   TempKey must be updated by GenDig using a ParentKey as specified by SlotConfig.ReadKey before executing this function.
   The decryption function does not check whether the TempKey has been generated by a correct ParentKey for the corresponding zone.
   Therefore to get a correct result, the application has to make sure that prior GenDig calculation was done using correct ParentKey.

 * \param[in, out] param pointer to parameter structure
 * \return status of the operation
 */
ATCA_STATUS atcah_decrypt(struct atca_decrypt_in_out *param)
{
	uint8_t i;

	// Check parameters
	if (!param->crypto_data || !param->temp_key)
		return ATCA_BAD_PARAM;

	// Check TempKey fields validity
	// Note that if temp_key.key_id is not checked,
	// we cannot make sure if the key used in previous GenDig IS equal to
	// the key pointed by SlotConfig.ReadKey in the device.
	if (    // TempKey.CheckFlag must be 0
	    param->temp_key->check_flag
	        // TempKey.Valid must be 1
	    || (param->temp_key->valid != 1)
	        // TempKey.GenData must be 1
	    || (param->temp_key->gen_data != 1)
	        // TempKey.SourceFlag must be 0 (random)
	    || param->temp_key->source_flag
	    ) {
		// Invalidate TempKey, then return
		param->temp_key->valid = 0;
		return ATCA_EXECUTION_ERROR;
	}

	// Decrypt by XOR-ing Data with the TempKey
	for (i = 0; i < ATCA_KEY_SIZE; i++)
		param->crypto_data[i] ^= param->temp_key->value[i];

	// Update TempKey fields
	param->temp_key->valid = 0;

	return ATCA_SUCCESS;
}

/** \brief This function creates a SHA256 digest on a little-endian system.
 *
 * Limitations: This function was implemented with the ATSHA204 CryptoAuth device
 * in mind. It will therefore only work for length values of len % 64 < 62.
 *
 * \param[in] len byte length of message
 * \param[in] message pointer to message
 * \param[out] digest SHA256 of message
 */
ATCA_STATUS atcah_sha256(int32_t len, const uint8_t *message, uint8_t *digest)
{
	return atcac_sw_sha2_256(message, len, digest);
}

