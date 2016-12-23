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
		memcpy(param->p_temp, param->otp, 11);            // use OTP[0:10], Mode:5 is overridden
		param->p_temp += 11;
	}else {
		if (param->mode & MAC_MODE_INCLUDE_OTP_64)
			memcpy(param->p_temp, param->otp, 8);         // use 8 bytes OTP[0:7] for (6)
		else
			memset(param->p_temp, 0, 8);                  // use 8 zeros for (6)
		param->p_temp += 8;

		memset(param->p_temp, 0, 3);                     // use 3 zeros for (7)
		param->p_temp += 3;
	}

	// (8) 1 byte SN[8] = 0xEE
	*param->p_temp++ = param->sn[8];

	// (9) 4 bytes SN[4:7] or zeros
	if (param->mode & MAC_MODE_INCLUDE_SN)
		memcpy(param->p_temp, &param->sn[4], 4);           //use SN[4:7] for (9)
	else
		memset(param->p_temp, 0, 4);                       //use zeros for (9)
	param->p_temp += 4;

	// (10) 2 bytes
	*param->p_temp++ = param->sn[0];
	*param->p_temp++ = param->sn[1];

	// (11) 2 bytes SN[2:3] or zeros
	if (param->mode & MAC_MODE_INCLUDE_SN)
		memcpy(param->p_temp, &param->sn[2], 2);           //use SN[2:3] for (11)
	else
		memset(param->p_temp, 0, 2);                       //use zeros for (9)
	param->p_temp += 2;

	return param->p_temp;
}

/** \brief 
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
		atcac_sw_sha2_256(temporary, ATCA_MSG_SIZE_NONCE, param->temp_key->value);

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
	param->temp_key->gen_dig_data = 0;
	param->temp_key->no_mac_flag = 0;
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
	    && (param->temp_key->no_mac_flag || (param->temp_key->valid != 1)
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
	atcac_sw_sha2_256(temporary, ATCA_MSG_SIZE_MAC, param->response);

	// Update TempKey fields
	if (param->temp_key)
		param->temp_key->valid = 0;

	return ATCA_SUCCESS;
}


/** \brief 
 * \param[inout] param  Input and output parameters
 * \return status of the operation
 */
ATCA_STATUS atcah_check_mac(struct atca_check_mac_in_out *param)
{
	uint8_t msg[ATCA_MSG_SIZE_MAC];
    bool is_temp_key_req = false;

    // Check parameters
    if (param == NULL || param->other_data == NULL || param->sn == NULL || param->client_resp == NULL)
        return ATCA_BAD_PARAM;

    if ((param->mode & CHECKMAC_MODE_BLOCK1_TEMPKEY) || (param->mode & CHECKMAC_MODE_BLOCK2_TEMPKEY))
        is_temp_key_req = true; // Message uses TempKey
    else if ((param->mode == 0x01 || param->mode == 0x05) && param->target_key != NULL)
        is_temp_key_req = true; // CheckMac copy will be performed

    if (is_temp_key_req && param->temp_key == NULL)
        return ATCA_BAD_PARAM;
    if (!(param->mode & CHECKMAC_MODE_BLOCK1_TEMPKEY) && param->slot_key == NULL)
        return ATCA_BAD_PARAM;
    if (!(param->mode & CHECKMAC_MODE_BLOCK2_TEMPKEY) && param->client_chal == NULL)
        return ATCA_BAD_PARAM;
    if ((param->mode & CHECKMAC_MODE_INCLUDE_OTP_64) && param->otp == NULL)
        return ATCA_BAD_PARAM;

    if ((param->mode & CHECKMAC_MODE_BLOCK1_TEMPKEY) || (param->mode & CHECKMAC_MODE_BLOCK2_TEMPKEY))
    {
        // This will use TempKey in message, check validity
        if (!param->temp_key->valid)
            return ATCA_EXECUTION_ERROR; // TempKey is not valid
        if (((param->mode >> 2) & 0x01) != param->temp_key->source_flag)
            return ATCA_EXECUTION_ERROR; // TempKey SourceFlag doesn't match bit 2 of the mode
    }

    // Build the message
    memset(msg, 0, sizeof(msg));
    if (param->mode & CHECKMAC_MODE_BLOCK1_TEMPKEY)
        memcpy(&msg[0], param->temp_key->value, 32);
    else
        memcpy(&msg[0], param->slot_key, 32);
    if (param->mode & CHECKMAC_MODE_BLOCK2_TEMPKEY)
        memcpy(&msg[32], param->temp_key->value, 32);
    else
        memcpy(&msg[32], param->client_chal, 32);
    memcpy(&msg[64], &param->other_data[0], 4);
    if (param->mode & CHECKMAC_MODE_INCLUDE_OTP_64)
        memcpy(&msg[68], param->otp, 8);
    memcpy(&msg[76], &param->other_data[4], 3);
    msg[79] = param->sn[8];
    memcpy(&msg[80], &param->other_data[7], 4);
    memcpy(&msg[84], &param->sn[0], 2);
    memcpy(&msg[86], &param->other_data[11], 2);

	// Calculate the client response
	atcac_sw_sha2_256(msg, sizeof(msg), param->client_resp);

	// Update TempKey fields
    if ((param->mode == 0x01 || param->mode == 0x05) && param->target_key != NULL)
    {
        // CheckMac Copy will be performed
        memcpy(param->temp_key->value, param->target_key, ATCA_KEY_SIZE);
        param->temp_key->gen_dig_data = 0;
        param->temp_key->source_flag = 1;
        param->temp_key->valid = 1;
    }

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
	uint8_t temporary[HMAC_BLOCK_SIZE + ATCA_MSG_SIZE_HMAC];
	uint8_t i = 0;
	uint8_t *p_temp = NULL;
    
	// Check parameters
	if (!param->response || !param->key || !param->temp_key
	    || (param->mode & ~HMAC_MODE_MASK)
	    || (((param->mode & MAC_MODE_INCLUDE_OTP_64) || (param->mode & MAC_MODE_INCLUDE_OTP_88)) && !param->otp)
	    || (!param->sn)
	    )
		return ATCA_BAD_PARAM;

	// Check TempKey fields validity (TempKey is always used)
	if (    // TempKey.CheckFlag must be 0 and TempKey.Valid must be 1
	    param->temp_key->no_mac_flag || (param->temp_key->valid != 1)
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
    
	// XOR key with ipad
	for (i = 0; i < ATCA_KEY_SIZE; i++)
		*p_temp++ = param->key[i] ^ 0x36;
        
    // zero pad key out to block size
    // Refer to fips-198 , length Key = 32 bytes, Block size = 512 bits = 64 bytes.
    // So the Key must be padded with zeros.
    memset(p_temp, 0x36, HMAC_BLOCK_SIZE - ATCA_KEY_SIZE);
    p_temp += HMAC_BLOCK_SIZE - ATCA_KEY_SIZE;
    
	// Next append the stream of data 'text'
    memset(p_temp, 0, ATCA_KEY_SIZE);
    p_temp += ATCA_KEY_SIZE;
    
    memcpy(p_temp, param->temp_key->value, ATCA_KEY_SIZE);
    p_temp += ATCA_KEY_SIZE;
    
	*p_temp++ = ATCA_HMAC;
    *p_temp++ = param->mode;
    *p_temp++ = (uint8_t)(param->key_id >> 0);
    *p_temp++ = (uint8_t)(param->key_id >> 8);
    
    include_data.otp = param->otp;
    include_data.sn = param->sn;
    include_data.mode = param->mode;
    include_data.p_temp = p_temp;
    atcah_include_data(&include_data);

	// Calculate SHA256
	// H((K0^ipad):text), use param.response for temporary storage
	atcac_sw_sha2_256(temporary, HMAC_BLOCK_SIZE + ATCA_MSG_SIZE_HMAC, param->response);


	// Start second calculation (outer)
    p_temp = temporary;
    
	// XOR K0 with opad
	for (i = 0; i < ATCA_KEY_SIZE; i++)
		*p_temp++ = param->key[i] ^ 0x5C;

	// zero pad key out to block size
	// Refer to fips-198 , length Key = 32 bytes, Block size = 512 bits = 64 bytes.
	// So the Key must be padded with zeros.
    memset(p_temp, 0x5C, HMAC_BLOCK_SIZE - ATCA_KEY_SIZE);
	p_temp += HMAC_BLOCK_SIZE - ATCA_KEY_SIZE;

	// Append result from last calculation H((K0 ^ ipad) || text)
	memcpy(p_temp, param->response, ATCA_SHA_DIGEST_SIZE);
	p_temp += ATCA_SHA_DIGEST_SIZE;
    
	// Calculate SHA256 to get the resulting HMAC
	atcac_sw_sha2_256(temporary, HMAC_BLOCK_SIZE + ATCA_SHA_DIGEST_SIZE, param->response);

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
	    param->temp_key->no_mac_flag || (param->temp_key->valid != 1)
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

	// (5) 1 byte SN[8]
	*p_temp++ = param->sn[8];

	// (6) 2 bytes SN[0:1]
	*p_temp++ = param->sn[0];
	*p_temp++ = param->sn[1];

	// (7) 25 zeros
	memset(p_temp, 0, ATCA_GENDIG_ZEROS_SIZE);
	p_temp += ATCA_GENDIG_ZEROS_SIZE;

	// (8) 32 bytes TempKey
	memcpy(p_temp, param->temp_key->value, ATCA_KEY_SIZE);

	// Calculate SHA256 to get the new TempKey
	atcac_sw_sha2_256(temporary, ATCA_MSG_SIZE_GEN_DIG, param->temp_key->value);

	// Update TempKey fields
	param->temp_key->valid = 1;

	if ((param->zone == GENDIG_ZONE_DATA) && (param->key_id <= 15)) {
		param->temp_key->gen_dig_data = 1;
		param->temp_key->key_id = (param->key_id & 0xF);    // mask lower 4-bit only
	}else {
		param->temp_key->gen_dig_data = 0;
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
	    param->temp_key->no_mac_flag || (param->temp_key->valid != 1)
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

	// (5) 1 byte SN[8]
	*p_temp++ = param->sn[8];

	// (6) 2 bytes SN[0:1]
	*p_temp++ = param->sn[0];
	*p_temp++ = param->sn[1];

	// (7) 25 zeros
	memset(p_temp, 0, ATCA_GENDIG_ZEROS_SIZE);
	p_temp += ATCA_GENDIG_ZEROS_SIZE;

	// (8) 32 bytes PlainText
	memcpy(p_temp, param->stored_value, ATCA_KEY_SIZE);

	// Calculate SHA256 to get the new TempKey
	atcac_sw_sha2_256(temporary, ATCA_MSG_SIZE_GEN_DIG, param->temp_key->value);

	// Update TempKey fields
	param->temp_key->valid = 1;

	if ((param->zone == GENDIG_ZONE_DATA) && (param->key_id <= 15)) {
		param->temp_key->gen_dig_data = 1;
		param->temp_key->key_id = (param->key_id & 0xF);    // mask lower 4-bit only
	}else {
		param->temp_key->gen_dig_data = 0;
		param->temp_key->key_id = 0;
	}

	return ATCA_SUCCESS;
}

/** \brief This function calculates the input MAC for the Write command.

   The Write command will need an input MAC if SlotConfig.WriteConfig.Encrypt is set.

 * \param[in, out] param pointer to parameter structure
 * \return status of the operation
 */
ATCA_STATUS atcah_write_auth_mac(struct atca_write_mac_in_out *param)
{
	uint8_t mac_input[ATCA_MSG_SIZE_ENCRYPT_MAC];
	uint8_t i;
	uint8_t *p_temp;

	// Check parameters
	if (!param->input_data || !param->temp_key)
		return ATCA_BAD_PARAM;

	// Check TempKey fields validity (TempKey is always used)
	if ( // TempKey.CheckFlag must be 0 and TempKey.Valid must be 1
	    param->temp_key->no_mac_flag || (param->temp_key->valid != 1)
	    ) {
		// Invalidate TempKey, then return
		param->temp_key->valid = 0;
		return ATCA_EXECUTION_ERROR;
	}
	// Encrypt by XOR-ing Data with the TempKey
	for (i = 0; i < 32; i++)
		param->encrypted_data[i] = param->input_data[i] ^ param->temp_key->value[i];
    
	// If the pointer *mac is provided by the caller then calculate input MAC
	if (param->auth_mac) {
		// Start calculation
		p_temp = mac_input;

		// (1) 32 bytes TempKey
		memcpy(p_temp, param->temp_key->value, ATCA_KEY_SIZE);
		p_temp += ATCA_KEY_SIZE;

		// (2) 1 byte Opcode
		*p_temp++ = ATCA_WRITE;

		// (3) 1 byte Param1 (zone)
		*p_temp++ = param->zone;

		// (4) 2 bytes Param2 (keyID)
		*p_temp++ = param->key_id & 0xFF;
		*p_temp++ = (param->key_id >> 8) & 0xFF;

		// (5) 1 byte SN[8]
		*p_temp++ = param->sn[8];

		// (6) 2 bytes SN[0:1]
		*p_temp++ = param->sn[0];
		*p_temp++ = param->sn[1];

		// (7) 25 zeros
		memset(p_temp, 0, ATCA_WRITE_MAC_ZEROS_SIZE);
		p_temp += ATCA_WRITE_MAC_ZEROS_SIZE;

		// (8) 32 bytes PlainText
		memcpy(p_temp, param->input_data, ATCA_KEY_SIZE);

		// Calculate SHA256 to get MAC
		atcac_sw_sha2_256(mac_input, sizeof(mac_input), param->auth_mac);
	}
    
	return ATCA_SUCCESS;
}

/** \brief This function calculates the input MAC for the PrivWrite command.

   The PrivWrite command will need an input MAC if SlotConfig.WriteConfig.Encrypt is set.

 * \param[in, out] param pointer to parameter structure
 * \return status of the operation
 */
ATCA_STATUS atcah_privwrite_auth_mac(struct atca_write_mac_in_out *param)
{
	uint8_t mac_input[ATCA_MSG_SIZE_PRIVWRITE_MAC];
	uint8_t i = 0;
	uint8_t *p_temp = NULL;
	uint8_t session_key2[32];

	// Check parameters
	if (!param->input_data || !param->temp_key)
		return ATCA_BAD_PARAM;

	// Check TempKey fields validity (TempKey is always used)
	if ( // TempKey.CheckFlag must be 0 and TempKey.Valid must be 1
	    param->temp_key->no_mac_flag || (param->temp_key->valid != 1)
	    ) {
		// Invalidate TempKey, then return
		param->temp_key->valid = 0;
		return ATCA_EXECUTION_ERROR;
	}


	/* Encrypt by XOR-ing Data with the TempKey
	 */

	// Encrypt the next 28 bytes of the cipher text, which is the first part of the private key.
	for (i = 0; i < 32; i++)
		param->encrypted_data[i] = param->input_data[i] ^ param->temp_key->value[i];

	// Calculate the new key for the last 4 bytes of the cipher text
	atcac_sw_sha2_256(param->temp_key->value, 32, session_key2);

	// Encrypt the last 4 bytes of the cipher text, which is the remaining part of the private key
	for (i = 32; i < 36; i++)
		param->encrypted_data[i] = param->input_data[i] ^ session_key2[i - 32];

	// If the pointer *mac is provided by the caller then calculate input MAC
	if (param->auth_mac) {
		// Start calculation
		p_temp = mac_input;

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

		// (5) 1 byte SN[8]
		*p_temp++ = param->sn[8];

		// (6) 2 bytes SN[0:1]
		*p_temp++ = param->sn[0];
		*p_temp++ = param->sn[1];

		// (7) 21 zeros
		memset(p_temp, 0, ATCA_PRIVWRITE_MAC_ZEROS_SIZE);
		p_temp += ATCA_PRIVWRITE_MAC_ZEROS_SIZE;

		// (8) 36 bytes PlainText (Private Key)
		memcpy(p_temp, param->input_data, ATCA_PRIVWRITE_PLAIN_TEXT_SIZE);

		// Calculate SHA256 to get the new TempKey
		atcac_sw_sha2_256(mac_input, sizeof(mac_input), param->auth_mac);
	}
    
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
	    || (param->mode & ~DERIVE_KEY_RANDOM_FLAG) || (param->target_key_id > ATCA_KEY_ID_MAX))
		return ATCA_BAD_PARAM;


	// Check TempKey fields validity (TempKey is always used)
	if (    // TempKey.CheckFlag must be 0 and TempKey.Valid must be 1
	    param->temp_key->no_mac_flag || (param->temp_key->valid != 1)
	        // The random parameter bit 2 must match temp_key.source_flag
	        // Logical not (!) is used to evaluate the expression to TRUE / FALSE first before comparison (!=).
	    || (!(param->mode & DERIVE_KEY_RANDOM_FLAG) != !(param->temp_key->source_flag))
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
	*p_temp++ = param->mode;

	// (4) 2 bytes Param2 (keyID)
	*p_temp++ = param->target_key_id & 0xFF;
	*p_temp++ = (param->target_key_id >> 8) & 0xFF;

	// (5) 1 byte SN[8]
	*p_temp++ = param->sn[8];

	// (6) 2 bytes SN[0:1]
	*p_temp++ = param->sn[0];
	*p_temp++ = param->sn[1];

	// (7) 25 zeros
	memset(p_temp, 0, ATCA_DERIVE_KEY_ZEROS_SIZE);
	p_temp += ATCA_DERIVE_KEY_ZEROS_SIZE;

	// (8) 32 bytes TempKey
	memcpy(p_temp, param->temp_key->value, ATCA_KEY_SIZE);
	p_temp += ATCA_KEY_SIZE;

	// Calculate SHA256 to get the derived key.
	atcac_sw_sha2_256(temporary, ATCA_MSG_SIZE_DERIVE_KEY, param->target_key);

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
	if (!param->parent_key || !param->mac || (param->mode & ~DERIVE_KEY_RANDOM_FLAG)
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
	*p_temp++ = param->mode;

	// (4) 2 bytes Param2 (keyID)
	*p_temp++ = param->target_key_id & 0xFF;
	*p_temp++ = (param->target_key_id >> 8) & 0xFF;

	// (5) 1 byte SN[8]
	*p_temp++ = param->sn[8];

	// (6) 2 bytes SN[0:1]
	*p_temp++ = param->sn[0];
	*p_temp++ = param->sn[1];

	// Calculate SHA256 to get the input MAC for DeriveKey command
	atcac_sw_sha2_256(temporary, ATCA_MSG_SIZE_DERIVE_KEY_MAC, param->mac);

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
/*
// Not sure this one is being used
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
	    param->temp_key->no_mac_flag
	        // TempKey.Valid must be 1
	    || (param->temp_key->valid != 1)
	        // TempKey.GenData must be 1
	    || (param->temp_key->gen_dig_data != 1)
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
*/


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
	    param->temp_key->no_mac_flag
	        // TempKey.Valid must be 1
	    || (param->temp_key->valid != 1)
	        // TempKey.GenData must be 1
	    || (param->temp_key->gen_dig_data != 1)
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
 * \param[in] len byte length of message
 * \param[in] message pointer to message
 * \param[out] digest SHA256 of message
 */
ATCA_STATUS atcah_sha256(int32_t len, const uint8_t *message, uint8_t *digest)
{
	return atcac_sw_sha2_256(message, len, digest);
}

/** \brief Calculate the PubKey digest created by GenKey and saved to TempKey.
 *
 * \param[inout] param  GenKey parameters required to calculate the PubKey
 *                        digest. Digest is return in the temp_key parameter.
 *
 * \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcah_gen_key_msg(struct atca_gen_key_in_out *param)
{
	uint8_t msg[128];

	if (param == NULL || param->public_key == NULL || param->sn == NULL || param->temp_key == NULL)
		return ATCA_BAD_PARAM;
	if (param->public_key_size == 0 || param->public_key_size > 88)
		return ATCA_BAD_PARAM;

	memset(msg, 0, sizeof(msg));
	memcpy(&msg[0], param->temp_key->value, 32);
	msg[32] = ATCA_GENKEY;

	if (param->mode & GENKEY_MODE_PUBKEY_DIGEST)
	{
		// Calculate PubKey digest of stored public key, takes priority over other bits
		if (param->other_data == NULL)
			return ATCA_BAD_PARAM;
		memcpy(&msg[33], param->other_data, 3); // OtherData replaces mode and key_id in message
	}
	else if (param->mode & GENKEY_MODE_PUBLIC)
	{
		msg[33] = param->mode;
		msg[34] = (uint8_t)(param->key_id >> 0);
		msg[35] = (uint8_t)(param->key_id >> 8);
	}
	else
	{
		// Mode indicates no PubKey digest was requested.
		// No change to TempKey.
		return ATCA_SUCCESS;
	}

	msg[36] = param->sn[8];
	memcpy(&msg[37], &param->sn[0], 2);

	// Copy public key into end of message
	memcpy(&msg[sizeof(msg) - param->public_key_size], param->public_key, param->public_key_size);

	atcac_sw_sha2_256(msg, sizeof(msg), param->temp_key->value);
	param->temp_key->gen_dig_data = 0;
	param->temp_key->gen_key_data = 1;
	param->temp_key->key_id = param->key_id;

	return ATCA_SUCCESS;
}

/** \brief Populate the slot_config, key_config, and is_slot_locked fields in
 *         the atca_sign_internal_in_out structure from the provided config
 *         zone.
 *
 * The atca_sign_internal_in_out structure has a number of fields
 * (slot_config, key_config, is_slot_locked) that can be determined
 * automatically from the current state of TempKey and the full config
 * zone.
 *
 * \param[inout] param   Sign(Internal) parameters to be filled out. Only
 *                       slot_config, key_config, and is_slot_locked will be
 *                       set.
 * \param[in]    config  Full 128 byte config zone for the device.
 */
ATCA_STATUS atcah_config_to_sign_internal(ATCADeviceType device_type, struct atca_sign_internal_in_out *param, const uint8_t* config)
{
	const uint8_t* value = NULL;
	uint16_t slot_locked = 0;
	if (param == NULL || config == NULL || param->temp_key == NULL)
		return ATCA_BAD_PARAM;

	// SlotConfig[TempKeyFlags.keyId]
	value = &config[20 + param->temp_key->key_id * 2];
	param->slot_config = (uint16_t)value[0] | ((uint16_t)value[1] << 8);

	// KeyConfig[TempKeyFlags.keyId]
	value = &config[96 + param->temp_key->key_id * 2];
	param->key_config = (uint16_t)value[0] | ((uint16_t)value[1] << 8);

	if (device_type == ATECC108A && param->temp_key->key_id < 8)
	{
		value = &config[52 + param->temp_key->key_id*2];
		param->use_flag = value[0];
		param->update_count = value[0];
	}
	else
	{
		param->use_flag = 0x00;
		param->update_count = 0x00;
	}

	//SlotLocked:TempKeyFlags.keyId
	slot_locked = (uint16_t)config[88] | ((uint16_t)config[89] << 8);
	param->is_slot_locked = (slot_locked & (1 << param->temp_key->key_id)) ? false : true;

	return ATCA_SUCCESS;
}

/** \brief Builds the full message that would be signed by the Sign(Internal)
 *         command.
 *
 * Additionally, the function will optionally output the OtherData data
 * required by the Verify(In/Validate) command as well as the SHA256 digest of
 * the full message.
 *
 * \param[inout] param  Input data and output buffers required by the command.
 *
 * \return ATCA_SUCCESS on success
 */
ATCA_STATUS atcah_sign_internal_msg(ATCADeviceType device_type, struct atca_sign_internal_in_out *param)
{
	uint8_t msg[55];

	if (param == NULL || param->temp_key == NULL || param->sn == NULL)
		return ATCA_BAD_PARAM;

	memset(msg, 0, sizeof(msg));
	memcpy(&msg[0], param->temp_key->value, 32);
	msg[32] = ATCA_SIGN; // Sign OpCode
	msg[33] = param->mode; // Sign Mode
	msg[34] = (uint8_t)(param->key_id >> 0); // Sign KeyID
	msg[35] = (uint8_t)(param->key_id >> 8);
	msg[36] = (uint8_t)(param->slot_config >> 0); // SlotConfig[TempKeyFlags.keyId]
	msg[37] = (uint8_t)(param->slot_config >> 8);
	msg[38] = (uint8_t)(param->key_config >> 0); // KeyConfig[TempKeyFlags.keyId]
	msg[39] = (uint8_t)(param->key_config >> 8);

	//TempKeyFlags (b0-3: keyId, b4: sourceFlag, b5: GenDigData, b6: GenKeyData, b7: NoMacFlag)
	msg[40] |= ((param->temp_key->key_id & 0x0F) << 0);
	msg[40] |= ((param->temp_key->source_flag & 0x01) << 4);
	msg[40] |= ((param->temp_key->gen_dig_data & 0x01) << 5);
	msg[40] |= ((param->temp_key->gen_key_data & 0x01) << 6);
	msg[40] |= ((param->temp_key->no_mac_flag & 0x01) << 7);

	if (device_type == ATECC108A && param->temp_key->key_id < 8)
	{
		msg[41] = param->use_flag;     // UseFlag[TempKeyFlags.keyId]
		msg[42] = param->update_count; // UpdateCount[TempKeyFlags.keyId]
	}
	else
	{
		msg[41] = 0x00;
		msg[42] = 0x00;
	}

	// Serial Number
	msg[43] = param->sn[8];
	memcpy(&msg[48], &param->sn[0], 2);
	if (param->mode & SIGN_MODE_INCLUDE_SN)
	{
		memcpy(&msg[44], &param->sn[4], 4);
		memcpy(&msg[50], &param->sn[2], 2);
	}

	// The bit within the SlotLocked field corresponding to the last key used in the TempKey computation is in the LSB
	msg[52] = param->is_slot_locked ? 0x00 : 0x01;

	// If the slot contains a public key corresponding to a supported curve, and if PubInfo indicates this key must be
	// validated before being used by Verify, and if the validity bits have a value of 0x05, then the PubKey Valid byte
	// will be 0x01.In all other cases, it will be 0.
	msg[53] = param->for_invalidate ? 0x01 : 0x00;

	msg[54] = 0x00;

	if (param->message)
		memcpy(param->message, msg, sizeof(msg));
	if (param->verify_other_data)
	{
		memcpy(&param->verify_other_data[0],  &msg[33], 10);
		memcpy(&param->verify_other_data[10], &msg[44], 4);
		memcpy(&param->verify_other_data[14], &msg[50], 5);
	}
	if (param->digest)
		return atcac_sw_sha2_256(msg, sizeof(msg), param->digest);
	else
		return ATCA_SUCCESS;
}
