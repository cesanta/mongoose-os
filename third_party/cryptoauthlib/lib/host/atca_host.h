/**
 * \file
 * \brief  Definitions and Prototypes for ATCA Utility Functions
 * \author Atmel Crypto Products
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


#ifndef ATCA_HOST_H
#   define ATCA_HOST_H

#include "cryptoauthlib.h"  // contains definitions used by chip and these routines

/** \defgroup atcah Host side crypto methods (atcah_)
 *
 * \brief
 * Use these functions if your system does not use an ATCADevice as a host but
 * implements the host in firmware. The functions provide host-side cryptographic functionality
 * for an ATECC client device. They are intended to accompany the CryptoAuthLib functions.
 * They can be called directly from an application, or integrated into an API.
 *
 * Modern compilers can garbage-collect unused functions. If your compiler does not support this feature,
 * you can just discard this module from your project if you do use an ATECC as a host. Or, if you don't,
 * delete the functions you do not use.
   @{ */

/** \name Definitions for ATECC Message Sizes to Calculate a SHA256 Hash

 *  \brief "||" is the concatenation operator.
 *         The number in braces is the length of the hash input value in bytes.
   @{ */

//! RandOut{32} || NumIn{20} || OpCode{1} || Mode{1} || LSB of Param2{1}
#define ATCA_MSG_SIZE_NONCE            (55)


/** \brief (Key or TempKey){32} || (Challenge or TempKey){32} || OpCode{1} || Mode{1} || Param2{2}
|| (OTP0_7 or 0){8} || (OTP8_10 or 0){3} || SN8{1} || (SN4_7 or 0){4} || SN0_1{2} || (SN2_3 or 0){2}
*/
#define ATCA_MSG_SIZE_MAC              (88)
#define ATCA_MSG_SIZE_HMAC             (88)

//! KeyId{32} || OpCode{1} || Param1{1} || Param2{2} || SN8{1} || SN0_1{2} || 0{25} || TempKey{32}
#define ATCA_MSG_SIZE_GEN_DIG          (96)


//! KeyId{32} || OpCode{1} || Param1{1} || Param2{2} || SN8{1} || SN0_1{2} || 0{25} || TempKey{32}
#define ATCA_MSG_SIZE_DERIVE_KEY       (96)


//! KeyId{32} || OpCode{1} || Param1{1} || Param2{2} || SN8{1} || SN0_1{2}
#define ATCA_MSG_SIZE_DERIVE_KEY_MAC   (39)

//! KeyId{32} || OpCode{1} || Param1{1} || Param2{2}|| SN8{1} || SN0_1{2} || 0{25} || TempKey{32}
#define ATCA_MSG_SIZE_ENCRYPT_MAC      (96)

//! KeyId{32} || OpCode{1} || Param1{1} || Param2{2}|| SN8{1} || SN0_1{2} || 0{21} || PlainText{36}
#define ATCA_MSG_SIZE_PRIVWRITE_MAC    (96)

#define ATCA_COMMAND_HEADER_SIZE       ( 4)
#define ATCA_GENDIG_ZEROS_SIZE         (25)
#define ATCA_WRITE_MAC_ZEROS_SIZE      (25)
#define ATCA_PRIVWRITE_MAC_ZEROS_SIZE  (21)
#define ATCA_PRIVWRITE_PLAIN_TEXT_SIZE (36)
#define ATCA_DERIVE_KEY_ZEROS_SIZE     (25)
#define HMAC_BLOCK_SIZE                (64)
/** @} */

/** \name Default Fixed Byte Values of Serial Number (SN[0:1] and SN[8])
   @{ */
#define ATCA_SN_0_DEF                (0x01)
#define ATCA_SN_1_DEF                (0x23)
#define ATCA_SN_8_DEF                (0xEE)
/** @} */


/** \name Definition for TempKey Mode
   @{ */
//! mode mask for MAC command when using TempKey
#define MAC_MODE_USE_TEMPKEY_MASK      ((uint8_t)0x03)
/** @} */

/** \brief Structure to hold TempKey fields
 */
typedef struct atca_temp_key {
	uint8_t value[ATCA_KEY_SIZE]; //!< Value of TempKey
	uint32_t key_id       : 4;    //!< If TempKey was derived from a slot or transport key (GenDig or GenKey), that key ID is saved here.
	uint32_t source_flag  : 1;    //!< Indicates id TempKey started from a random nonce (0) or not (1).
	uint32_t gen_dig_data : 1;    //!< TempKey was derived from the GenDig command.
	uint32_t gen_key_data : 1;    //!< TempKey was derived from the GenKey command (ATECC devices only).
	uint32_t no_mac_flag  : 1;    //!< TempKey was derived from a key that has the NoMac bit set preventing the use of the MAC command. Known as CheckFlag in ATSHA devices).
	uint32_t valid        : 1;    //!< TempKey is valid.
} atca_temp_key_t;


/** \struct atca_include_data_in_out
 *  \brief Input / output parameters for function atca_include_data().
 *  \var atca_include_data_in_out::p_temp
 *       \brief [out] pointer to output buffer
 *  \var atca_include_data_in_out::otp
 *       \brief [in] pointer to one-time-programming data
 *  \var atca_include_data_in_out::sn
 *       \brief [in] pointer to serial number data
 */
struct atca_include_data_in_out {
	uint8_t *p_temp;
	const uint8_t *otp;
	const uint8_t *sn;
	uint8_t mode;
};


/** \struct atca_nonce_in_out
 *  \brief Input/output parameters for function atca_nonce().
 *  \var atca_nonce_in_out::mode
 *       \brief [in] Mode parameter used in Nonce command (Param1).
 *  \var atca_nonce_in_out::num_in
 *       \brief [in] Pointer to 20-byte NumIn data used in Nonce command.
 *  \var atca_nonce_in_out::rand_out
 *       \brief [in] Pointer to 32-byte RandOut data from Nonce command.
 *  \var atca_nonce_in_out::temp_key
 *       \brief [in,out] Pointer to TempKey structure.
 */
typedef struct atca_nonce_in_out {
	uint8_t mode;
	const uint8_t *num_in;
	const uint8_t *rand_out;
	struct atca_temp_key *temp_key;
} atca_nonce_in_out_t;


/** \struct atca_mac_in_out
 *  \brief Input/output parameters for function atca_mac().
 *  \var atca_mac_in_out::mode
 *       \brief [in] Mode parameter used in MAC command (Param1).
 *  \var atca_mac_in_out::key_id
 *       \brief [in] KeyID parameter used in MAC command (Param2).
 *  \var atca_mac_in_out::challenge
 *       \brief [in] Pointer to 32-byte Challenge data used in MAC command, depending on mode.
 *  \var atca_mac_in_out::key
 *       \brief [in] Pointer to 32-byte key used to generate MAC digest.
 *  \var atca_mac_in_out::otp
 *       \brief [in] Pointer to 11-byte OTP, optionally included in MAC digest, depending on mode.
 *  \var atca_mac_in_out::sn
 *       \brief [in] Pointer to 9-byte SN, optionally included in MAC digest, depending on mode.
 *  \var atca_mac_in_out::response
 *       \brief [out] Pointer to 32-byte SHA-256 digest (MAC).
 *  \var atca_mac_in_out::temp_key
 *       \brief [in,out] Pointer to TempKey structure.
 */
struct atca_mac_in_out {
	uint8_t mode;
	uint16_t key_id;
	const uint8_t *challenge;
	const uint8_t *key;
	const uint8_t *otp;
	const uint8_t *sn;
	uint8_t *response;
	struct atca_temp_key *temp_key;
};


/** \struct atca_hmac_in_out
 *  \brief Input/output parameters for function atca_hmac().
 *  \var atca_hmac_in_out::mode
 *       \brief [in] Mode parameter used in HMAC command (Param1).
 *  \var atca_hmac_in_out::key_id
 *       \brief [in] KeyID parameter used in HMAC command (Param2).
 *  \var atca_hmac_in_out::key
 *       \brief [in] Pointer to 32-byte key used to generate HMAC digest.
 *  \var atca_hmac_in_out::otp
 *       \brief [in] Pointer to 11-byte OTP, optionally included in HMAC digest, depending on mode.
 *  \var atca_hmac_in_out::sn
 *       \brief [in] Pointer to 9-byte SN, optionally included in HMAC digest, depending on mode.
 *  \var atca_hmac_in_out::response
 *       \brief [out] Pointer to 32-byte SHA-256 HMAC digest.
 *  \var atca_hmac_in_out::temp_key
 *       \brief [in,out] Pointer to TempKey structure.
 */
struct atca_hmac_in_out {
	uint8_t mode;
	uint16_t key_id;
	const uint8_t *key;
	const uint8_t *otp;
	const uint8_t *sn;
	uint8_t *response;
	struct atca_temp_key *temp_key;
};


/**
 *  \brief Input/output parameters for function atcah_gen_dig().
 */
typedef struct atca_gen_dig_in_out {
	uint8_t zone;                   //!< Zone/Param1 for the GenDig command
	uint16_t key_id;                //!< KeyId/Param2 for the GenDig command
	const uint8_t *sn;              //!< Device serial number SN[0:8]. Only SN[0:1] and SN[8] are required though.
	const uint8_t *stored_value;    //!< 32-byte slot value, config block, or OTP block as specified by the Zone/KeyId parameters
	struct atca_temp_key *temp_key; //!< Current state of TempKey
} atca_gen_dig_in_out_t;

/**
 *  \brief Input/output parameters for function atcah_write_auth_mac() and atcah_privwrite_auth_mac().
 */
typedef struct atca_write_mac_in_out {
	uint8_t zone;                   //!< Zone/Param1 for the Write or PrivWrite command
	uint16_t key_id;                //!< KeyID/Param2 for the Write or PrivWrite command
    const uint8_t *sn;              //!< Device serial number SN[0:8]. Only SN[0:1] and SN[8] are required though.
	const uint8_t *input_data;      //!< Data to be encrypted. 32 bytes for Write command, 36 bytes for PrivWrite command.
	uint8_t *encrypted_data;        //!< Encrypted version of input_data will be returned here. 32 bytes for Write command, 36 bytes for PrivWrite command.
	uint8_t *auth_mac;              //!< Write MAC will be returned here. 32 bytes.
	struct atca_temp_key *temp_key; //!< Current state of TempKey.
} atca_write_mac_in_out_t;

/**
 *  \brief Input/output parameters for function atcah_derive_key().
 */
struct atca_derive_key_in_out {
	uint8_t mode;                   //!< Mode (param 1) of the derive key command
	uint16_t target_key_id;         //!< Key ID (param 2) of the target slot to run the command on
	const uint8_t *sn;              //!< Device serial number SN[0:8]. Only SN[0:1] and SN[8] are required though.
	const uint8_t *parent_key;      //!< Parent key to be used in the derive key calculation (32 bytes).
	uint8_t *target_key;            //!< Derived key will be returned here (32 bytes).
	struct atca_temp_key *temp_key; //!< Current state of TempKey.
};


/**
 *  \brief Input/output parameters for function atcah_derive_key_mac().
 */
struct atca_derive_key_mac_in_out {
	uint8_t mode;                   //!< Mode (param 1) of the derive key command
	uint16_t target_key_id;         //!< Key ID (param 2) of the target slot to run the command on
	const uint8_t *sn;              //!< Device serial number SN[0:8]. Only SN[0:1] and SN[8] are required though.
	const uint8_t *parent_key;      //!< Parent key to be used in the derive key calculation (32 bytes).
	uint8_t *mac;                   //!< DeriveKey MAC will be returned here.
};


/** \struct atca_encrypt_in_out
 *  \brief Input/output parameters for function atca_encrypt().
 *  \var atca_encrypt_in_out::zone
 *       \brief [in] Zone parameter used in Write (Param1).
 *  \var atca_encrypt_in_out::address
 *       \brief [in] Address parameter used in Write command (Param2).
 *  \var atca_encrypt_in_out::crypto_data
 *       \brief [in,out] Pointer to 32-byte data. Input cleartext data, output encrypted data to Write command (Value field).
 *  \var atca_encrypt_in_out::mac
 *       \brief [out] Pointer to 32-byte Mac. Can be set to NULL if input MAC is not required by the Write command (write to OTP, unlocked user zone).
 *  \var atca_encrypt_in_out::temp_key
 *       \brief [in,out] Pointer to TempKey structure.
 */
struct atca_encrypt_in_out {
	uint8_t zone;
	uint16_t address;
	uint8_t *crypto_data;
	uint8_t *mac;
	struct atca_temp_key *temp_key;
};


/** \struct atca_decrypt_in_out
 *  \brief Input/output parameters for function atca_decrypt().
 *  \var atca_decrypt_in_out::crypto_data
 *       \brief [in,out] Pointer to 32-byte data. Input encrypted data from Read command (Contents field), output decrypted.
 *  \var atca_decrypt_in_out::temp_key
 *       \brief [in,out] Pointer to TempKey structure.
 */
struct atca_decrypt_in_out {
	uint8_t *crypto_data;
	struct atca_temp_key *temp_key;
};


/** \brief Input/output parameters for function atcah_check_mac().
 */
typedef struct atca_check_mac_in_out {
	uint8_t mode;                   //!< [in] CheckMac command Mode
    uint16_t key_id;                //!< [in] CheckMac command KeyID
	const uint8_t *sn;              //!< [in] Device serial number SN[0:8]. Only SN[0:1] and SN[8] are required though.
	const uint8_t *client_chal;     //!< [in] ClientChal data, 32 bytes. Can be NULL if mode[0] is 1.
    uint8_t       *client_resp;     //!< [out] Calculated ClientResp will be returned here.
	const uint8_t *other_data;      //!< [in] OtherData, 13 bytes
	const uint8_t *otp;             //!< [in] First 8 bytes of the OTP zone data. Can be NULL is mode[5] is 0.
	const uint8_t *slot_key;        //!< [in] 32 byte key value in the slot specified by slot_id. Can be NULL if mode[1] is 1.
    /// [in] If this is not NULL, it assumes CheckMac copy is enabled for the specified key_id (ReadKey=0). If key_id
    /// is even, this should be the 32-byte key value for the slot key_id+1, otherwise this should be set to slot_key.
    const uint8_t *target_key;      
	struct atca_temp_key *temp_key; //!< [in,out] Current state of TempKey. Required if mode[0] or mode[1] are 1.
} atca_check_mac_in_out_t;


/** \struct atca_verify_in_out
 *  \brief Input/output parameters for function atcah_verify().
 *  \var atca_verify_in_out::curve_type
 *       \brief [in] Curve type used in Verify command (Param2).
 *  \var atca_verify_in_out::signature
 *       \brief [in] Pointer to ECDSA signature to be verified
 *  \var atca_verify_in_out::public_key
 *       \brief [in] Pointer to the public key to be used for verification
 *  \var atca_verify_in_out::temp_key
 *       \brief [in,out] Pointer to TempKey structure.
 */
struct atca_verify_in_out {
	uint16_t curve_type;
	const uint8_t *signature;
	const uint8_t *public_key;
	struct atca_temp_key *temp_key;
};

/** \brief Input/output parameters for calculating the PubKey digest put into
 *         TempKey by the GenKey command with the
 *         atcah_gen_key_msg() function.
 */
typedef struct atca_gen_key_in_out {
	uint8_t mode;                   //!< [in] GenKey Mode
	uint16_t key_id;                //!< [in]  GenKey KeyID
	const uint8_t *public_key;      //!< [in]  Public key to be used in the PubKey digest. X and Y integers in big-endian format. 64 bytes for P256 curve.
	size_t public_key_size;         //!< [in] Total number of bytes in the public key. 64 bytes for P256 curve.
	const uint8_t *other_data;      //!< [in]  3 bytes required when bit 4 of the mode is set. Can be NULL otherwise.
	const uint8_t *sn;              //!< [in] Device serial number SN[0:8] (9 bytes). Only SN[0:1] and SN[8] are required though.
	struct atca_temp_key *temp_key; //!< [in,out] As input the current state of TempKey. As output, the resulting PubKEy digest.
} atca_gen_key_in_out_t;

/** \brief Input/output parameters for calculating the message and digest used
 *         by the Sign(internal) command. Used with the
 *         atcah_sign_internal_msg() function.
 */
typedef struct atca_sign_internal_in_out {
	uint8_t mode;               //!< [in] Sign Mode
	uint16_t key_id;            //!< [in] Sign KeyID
	uint16_t slot_config;       //!< [in] SlotConfig[TempKeyFlags.keyId]
	uint16_t key_config;        //!< [in] KeyConfig[TempKeyFlags.keyId]
	uint8_t use_flag;           //!< [in] UseFlag[TempKeyFlags.keyId], 0x00 for slots 8 and above and for ATECC508A
	uint8_t update_count;       //!< [in] UpdateCount[TempKeyFlags.keyId], 0x00 for slots 8 and above and for ATECC508A
	bool is_slot_locked;        //!< [in] Is TempKeyFlags.keyId slot locked.
	bool for_invalidate;        //!< [in] Set to true if this will be used for the Verify(Invalidate) command.
	const uint8_t *sn;          //!< [in] Device serial number SN[0:8] (9 bytes)
	const struct atca_temp_key *temp_key; //!< [in] The current state of TempKey.
	uint8_t* message;           //!< [out] Full 55 byte message the Sign(internal) command will build. Can be NULL if not required.
	uint8_t* verify_other_data; //!< [out] The 19 byte OtherData bytes to be used with the Verify(In/Validate) command. Can be NULL if not required.
	uint8_t* digest;            //!< [out] SHA256 digest of the full 55 byte message. Can be NULL if not required.
} atca_sign_internal_in_out_t;

#ifdef __cplusplus
extern "C" {
#endif

ATCA_STATUS atcah_nonce(struct atca_nonce_in_out *param);
ATCA_STATUS atcah_mac(struct atca_mac_in_out *param);
ATCA_STATUS atcah_check_mac(struct atca_check_mac_in_out *param);
ATCA_STATUS atcah_hmac(struct atca_hmac_in_out *param);
ATCA_STATUS atcah_gen_dig(struct atca_gen_dig_in_out *param);
ATCA_STATUS atcah_gen_mac(struct atca_gen_dig_in_out *param);
ATCA_STATUS atcah_write_auth_mac(struct atca_write_mac_in_out *param);
ATCA_STATUS atcah_privwrite_auth_mac(struct atca_write_mac_in_out *param);
ATCA_STATUS atcah_derive_key(struct atca_derive_key_in_out *param);
ATCA_STATUS atcah_derive_key_mac(struct atca_derive_key_mac_in_out *param);
ATCA_STATUS atcah_encrypt(struct atca_encrypt_in_out *param);
ATCA_STATUS atcah_decrypt(struct atca_decrypt_in_out *param);
ATCA_STATUS atcah_sha256(int32_t len, const uint8_t *message, uint8_t *digest);
uint8_t *atcah_include_data(struct atca_include_data_in_out *param);
ATCA_STATUS atcah_gen_key_msg(struct atca_gen_key_in_out *param);
ATCA_STATUS atcah_config_to_sign_internal(ATCADeviceType device_type, struct atca_sign_internal_in_out *param, const uint8_t* config);
ATCA_STATUS atcah_sign_internal_msg(ATCADeviceType device_type, struct atca_sign_internal_in_out *param);

#ifdef __cplusplus
}
#endif

/** @} */

#endif //ATCA_HOST_H
