/**
 * \file
 * \brief Declarations for certificates related to ECC CryptoAuthentication devices.
 * These are the definitions required to define a certificate and its various
 * elements with regards to the CryptoAuthentication ECC devices.
 *
 * Only the dynamic elements of a certificate (the parts of the certificate
 * that change from device to device) are stored on the ATECC device. The
 * definitions here describe the form of the certificate, and where the
 * dynamic elements can be found both on the ATECC device itself and in the
 * certificate template.
 *
 * This also defines utility functions for working with the certificates and their definitions.
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


#ifndef ATCACERT_DEF_H
#define ATCACERT_DEF_H

#include <stddef.h>
#include <stdint.h>
#include "atcacert.h"
#include "atcacert_date.h"

/** \defgroup atcacert_ Certificate manipulation methods (atcacert_)
 *
 * \brief
 * These methods provide convenient ways to perform certification I/O with
 * CryptoAuth chips and perform certificate manipulation in memory
 *
   @{ */

/**
 * Types of certificates.
 */
typedef enum atcacert_cert_type_e {
	CERTTYPE_X509,  //!< Standard X509 certificate
	CERTTYPE_CUSTOM //!< Custom format
} atcacert_cert_type_t;

/**
 * Sources for the certificate serial number.
 */
typedef enum atcacert_cert_sn_src_e {
	SNSRC_STORED             = 0x0, //!< Cert serial is stored on the device.
	SNSRC_DEVICE_SN          = 0x8, //!< Cert serial number is 0x40(MSB) + 9-byte device serial number. Only applies to device certificates.
	SNSRC_SIGNER_ID          = 0x9, //!< Cert serial number is 0x40(MSB) + 2-byte signer ID. Only applies to signer certificates.
	SNSRC_PUB_KEY_HASH       = 0xA, //!< Cert serial number is the SHA256(Subject public key + Encoded dates), with uppermost 2 bits set to 01.
	SNSRC_DEVICE_SN_HASH     = 0xB, //!< Cert serial number is the SHA256(Device SN + Encoded dates), with uppermost 2 bits set to 01. Only applies to device certificates.
	SNSRC_PUB_KEY_HASH_POS   = 0xC, //!< Depreciated, don't use. Cert serial number is the SHA256(Subject public key + Encoded dates), with MSBit set to 0 to ensure it's positive.
	SNSRC_DEVICE_SN_HASH_POS = 0xD, //!< Depreciated, don't use. Cert serial number is the SHA256(Device SN + Encoded dates), with MSBit set to 0 to ensure it's positive. Only applies to device certificates.
	SNSRC_PUB_KEY_HASH_RAW   = 0xE, //!< Depreciated, don't use. Cert serial number is the SHA256(Subject public key + Encoded dates).
	SNSRC_DEVICE_SN_HASH_RAW = 0xF  //!< Depreciated, don't use. Cert serial number is the SHA256(Device SN + Encoded dates). Only applies to device certificates.
} atcacert_cert_sn_src_t;

/**
 * ATECC device zones. The values match the Zone Encodings as specified in the datasheet.
 */
typedef enum atcacert_device_zone_e {
	DEVZONE_CONFIG = 0x00,  //!< Configuration zone.
	DEVZONE_OTP    = 0x01,  //!< One Time Programmable zone.
	DEVZONE_DATA   = 0x02,  //!< Data zone (slots).
	DEVZONE_NONE   = 0x07   //!< Special value used to indicate there is no device location.
} atcacert_device_zone_t;

/**
 * Standard dynamic certificate elements.
 */
typedef enum atcacert_std_cert_element_e {
	STDCERT_PUBLIC_KEY,
	STDCERT_SIGNATURE,
	STDCERT_ISSUE_DATE,
	STDCERT_EXPIRE_DATE,
	STDCERT_SIGNER_ID,
	STDCERT_CERT_SN,
	STDCERT_AUTH_KEY_ID,
	STDCERT_SUBJ_KEY_ID,
	STDCERT_NUM_ELEMENTS  //!< Special item to give the number of elements in this enum
} atcacert_std_cert_element_t;

// Some of these structures may need to be byte-accurate
#pragma pack(push, 1)

/**
 * Defines a chunk of data in an ATECC device.
 */
typedef struct atcacert_device_loc_s {
	atcacert_device_zone_t zone;        //!< Zone in the device.
	uint8_t slot;                       //!< Slot within the data zone. Only applies if zone is DEVZONE_DATA.
	uint8_t is_genkey;                  //!< If true, use GenKey command to get the contents instead of Read.
	uint16_t offset;                    //!< Byte offset in the zone.
	uint16_t count;                     //!< Byte count.
} atcacert_device_loc_t;

/**
 * Defines a chunk of data in a certificate template.
 */
typedef struct atcacert_cert_loc_s {
	uint16_t offset;    //!< Byte offset in the certificate template.
	uint16_t count;     //!< Byte count. Set to 0 if it doesn't exist.
} atcacert_cert_loc_t;

/**
 * Defines a generic dynamic element for a certificate including the device and template locations.
 */
typedef struct atcacert_cert_element_s {
	char id[16];                        //!< ID identifying this element.
	atcacert_device_loc_t device_loc;   //!< Location in the device for the element.
	atcacert_cert_loc_t cert_loc;       //!< Location in the certificate template for the element.
} atcacert_cert_element_t;

/**
 * Defines a certificate and all the pieces to work with it.
 *
 * If any of the standard certificate elements (std_cert_elements) are not a part of the certificate
 * definition, set their count to 0 to indicate their absence.
 */
typedef struct atcacert_def_s {
	atcacert_cert_type_t type;                                      //!< Certificate type.
	uint8_t template_id;                                            //!< ID for the this certificate definition (4-bit value).
	uint8_t chain_id;                                               //!< ID for the certificate chain this definition is a part of (4-bit value).
	uint8_t private_key_slot;                                       //!< If this is a device certificate template, this is the device slot for the device private key.
	atcacert_cert_sn_src_t sn_source;                               //!< Where the certificate serial number comes from (4-bit value).
	atcacert_device_loc_t cert_sn_dev_loc;                          //!< Only applies when sn_source is SNSRC_STORED. Describes where to get the certificate serial number on the device.
	atcacert_date_format_t issue_date_format;                       //!< Format of the issue date in the certificate.
	atcacert_date_format_t expire_date_format;                      //!< format of the expire date in the certificate.
	atcacert_cert_loc_t tbs_cert_loc;                               //!< Location in the certificate for the TBS (to be signed) portion.
	uint8_t expire_years;                                           //!< Number of years the certificate is valid for (5-bit value). 0 means no expiration.
	atcacert_device_loc_t public_key_dev_loc;                       //!< Where on the device the public key can be found.
	atcacert_device_loc_t comp_cert_dev_loc;                        //!< Where on the device the compressed cert can be found.
	atcacert_cert_loc_t std_cert_elements[STDCERT_NUM_ELEMENTS];    //!< Where in the certificate template the standard cert elements are inserted.
	const atcacert_cert_element_t* cert_elements;                   //!< Additional certificate elements outside of the standard certificate contents.
	uint8_t cert_elements_count;                                    //!< Number of additional certificate elements in cert_elements.
	const uint8_t*         cert_template;                           //!< Pointer to the actual certificate template data.
	uint16_t cert_template_size;                                    //!< Size of the certificate template in cert_template in bytes.
} atcacert_def_t;

/**
 * Tracks the state of a certificate as it's being rebuilt from device information.
 */
typedef struct atcacert_build_state_s {
	const atcacert_def_t* cert_def;             //!< Certificate definition for the certificate being rebuilt.
	uint8_t*              cert;                 //!< Buffer to contain the rebuilt certificate.
	size_t*               cert_size;            //!< Current size of the certificate in bytes.
	size_t max_cert_size;                       //!< Max size of the cert buffer in bytes.
	uint8_t is_device_sn;                       //!< Indicates the structure contains the device SN.
	uint8_t device_sn[9];                       //!< Storage for the device SN, when it's found.
} atcacert_build_state_t;

#pragma pack(pop)

// Inform function naming when compiling in C++
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Add all the device locations required to rebuild the specified certificate (cert_def) to
 *        a device locations list.
 *
 * The block_size parameter will adjust all added device locations to have a offset and count that
 * aligns with that block size. This allows one to generate a list of device locations that matches
 * specific read or write semantics (e.g. 4 byte or 32 byte reads).
 *
 * \param[in]    cert_def               Certificate definition containing all the device locations
 *                                      to add to the list.
 * \param[inout] device_locs            List of device locations to add to.
 * \param[inout] device_locs_count      As input, existing size of the device locations list.
 *                                      As output, the new size of the device locations list.
 * \param[in]    device_locs_max_count  Maximum number of elements device_locs can hold.
 * \param[in]    block_size             Block size to align all offsets and counts to when adding
 *                                      device locations.
 *
 * \return 0 on success
 */
int atcacert_get_device_locs( const atcacert_def_t*  cert_def,
                              atcacert_device_loc_t* device_locs,
                              size_t*                device_locs_count,
                              size_t device_locs_max_count,
                              size_t block_size);

/**
 * \brief Starts the certificate rebuilding process.
 *
 * \param[out] build_state    Structure is initialized to start the certificate building process.
 *                            Will be passed to the other certificate building functions.
 * \param[in]  cert_def       Certificate definition for the certificate being built.
 * \param[in]  cert           Buffer to contain the rebuilt certificate.
 * \param[in]  cert_size      As input, the size of the cert buffer in bytes. This value will be
 *                            adjusted to the current/final size of the certificate through the
 *                            building process.
 * \param[in]  ca_public_key  ECC P256 public key of the certificate authority (issuer) for the
 *                            certificate being built.
 *
 * \return 0 on success
 */
int atcacert_cert_build_start( atcacert_build_state_t* build_state,
                               const atcacert_def_t*   cert_def,
                               uint8_t*                cert,
                               size_t*                 cert_size,
                               const uint8_t ca_public_key[64]);

/**
 * \brief Process information read from the ATECC device. If it contains information for the
 *        certificate, it will be incorporated into the certificate.
 *
 * \param[in] build_state  Current certificate building state.
 * \param[in] device_loc   Device location structure describing where on the device the following
 *                         data came from.
 * \param[in] device_data  Actual data from the device. It should represent the offset and byte
 *                         count specified in the device_loc parameter.
 *
 * \return 0 on success
 */
int atcacert_cert_build_process( atcacert_build_state_t*      build_state,
                                 const atcacert_device_loc_t* device_loc,
                                 const uint8_t*               device_data);

/**
 * \brief Completes any final certificate processing required after all data from the device has
 *        been incorporated.
 *
 * The final certificate and its size in bytes are contained in the cert and cert_size elements
 * of the build_state structure. This will be the same buffers as supplied to the
 * atcacert_cert_build_start function at the beginning of the certificate rebuilding process.
 *
 * \param[in] build_state  Current certificate build state.
 *
 * \return 0 on success
 */
int atcacert_cert_build_finish( atcacert_build_state_t* build_state );

/**
 * \brief Gets the dynamic data that would be saved to the specified device location.  This
 *        function is primarily used to break down a full certificate into the dynamic components
 *        to be saved to a device.
 *
 * The atcacert_add_device_locs function can be used to generate a list of device locations a
 * particular certificate definition requires.
 *
 * \param[in]  cert_def     Certificate definition for the certificate we're getting data from.
 * \param[in]  cert         Certificate to get the device data from.
 * \param[in]  cert_size    Size of the certificate in bytes.
 * \param[in]  device_loc   Device location to request data for.
 * \param[out] device_data  Buffer that represents the device data in device_loc. Required to be
 *                          at least device_loc.count in size.
 *
 * \return 0 on success
 */
int atcacert_get_device_data( const atcacert_def_t*        cert_def,
                              const uint8_t*               cert,
                              size_t cert_size,
                              const atcacert_device_loc_t* device_loc,
                              uint8_t*                     device_data);

/**
 * \brief Sets the subject public key and subject key ID in a certificate.
 *
 * \param[in]    cert_def         Certificate definition for the certificate.
 * \param[inout] cert             Certificate to update.
 * \param[in]    cert_size        Size of the certificate (cert) in bytes.
 * \param[in]    subj_public_key  Subject public key as X and Y integers concatenated together. 64 bytes.
 *
 * \return 0 on success
 */
int atcacert_set_subj_public_key( const atcacert_def_t* cert_def,
                                  uint8_t*              cert,
                                  size_t cert_size,
                                  const uint8_t subj_public_key[64]);

/**
 * \brief Gets the subject public key from a certificate.
 *
 * \param[in]  cert_def         Certificate definition for the certificate.
 * \param[in]  cert             Certificate to get element from.
 * \param[in]  cert_size        Size of the certificate (cert) in bytes.
 * \param[out] subj_public_key  Subject public key is returned in this buffer. Formatted at X and Y
 *                              integers concatenated together. 64 bytes.
 *
 * \return 0 on success
 */
int atcacert_get_subj_public_key( const atcacert_def_t * cert_def,
                                  const uint8_t *        cert,
                                  size_t cert_size,
                                  uint8_t subj_public_key[64]);

/**
 * \brief Gets the subject key ID from a certificate.
 *
 * \param[in]  cert_def     Certificate definition for the certificate.
 * \param[in]  cert         Certificate to get element from.
 * \param[in]  cert_size    Size of the certificate (cert) in bytes.
 * \param[out] subj_key_id  Subject key ID is returned in this buffer. 20 bytes.
 *
 * \return 0 on success
 */
int atcacert_get_subj_key_id( const atcacert_def_t * cert_def,
                              const uint8_t *        cert,
                              size_t cert_size,
                              uint8_t subj_key_id[20]);

/**
 * \brief Sets the signature in a certificate. This may alter the size of the X.509 certificates.
 *
 * \param[in]    cert_def       Certificate definition for the certificate.
 * \param[inout] cert           Certificate to update.
 * \param[inout] cert_size      As input, size of the certificate (cert) in bytes.
 *                              As output, the new size of the certificate.
 * \param[in]    max_cert_size  Maximum size of the cert buffer.
 * \param[in]    signature      Signature as R and S integers concatenated together. 64 bytes.
 *
 * \return 0 on success
 */
int atcacert_set_signature( const atcacert_def_t* cert_def,
                            uint8_t*              cert,
                            size_t*               cert_size,
                            size_t max_cert_size,
                            const uint8_t signature[64]);

/**
 * \brief Gets the signature from a certificate.
 *
 * \param[in]  cert_def   Certificate definition for the certificate.
 * \param[in]  cert       Certificate to get element from.
 * \param[in]  cert_size  Size of the certificate (cert) in bytes.
 * \param[out] signature  Signature is returned in this buffer. Formatted at R and S integers
 *                        concatenated together. 64 bytes.
 *
 * \return 0 on success
 */
int atcacert_get_signature( const atcacert_def_t * cert_def,
                            const uint8_t *        cert,
                            size_t cert_size,
                            uint8_t signature[64]);

/**
 * \brief Sets the issue date (notBefore) in a certificate. Will be formatted according to the date
 *        format specified in the certificate definition.
 *
 * \param[in]    cert_def   Certificate definition for the certificate.
 * \param[inout] cert       Certificate to update.
 * \param[in]    cert_size  Size of the certificate (cert) in bytes.
 * \param[in]    timestamp  Issue date.
 *
 * \return 0 on success
 */
int atcacert_set_issue_date( const atcacert_def_t*     cert_def,
                             uint8_t*                  cert,
                             size_t cert_size,
                             const atcacert_tm_utc_t*  timestamp);

/**
 * \brief Gets the issue date from a certificate. Will be parsed according to the date format
 *        specified in the certificate definition.
 *
 * \param[in]  cert_def   Certificate definition for the certificate.
 * \param[in]  cert       Certificate to get element from.
 * \param[in]  cert_size  Size of the certificate (cert) in bytes.
 * \param[out] timestamp  Issue date is returned in this structure.
 *
 * \return 0 on success
 */
int atcacert_get_issue_date( const atcacert_def_t*  cert_def,
                             const uint8_t*         cert,
                             size_t cert_size,
                             atcacert_tm_utc_t*     timestamp);

/**
 * \brief Sets the expire date (notAfter) in a certificate. Will be formatted according to the date
 *        format specified in the certificate definition.
 *
 * \param[in]    cert_def   Certificate definition for the certificate.
 * \param[inout] cert       Certificate to update.
 * \param[in]    cert_size  Size of the certificate (cert) in bytes.
 * \param[in]    timestamp  Expire date.
 *
 * \return 0 on success
 */
int atcacert_set_expire_date( const atcacert_def_t*     cert_def,
                              uint8_t*                  cert,
                              size_t cert_size,
                              const atcacert_tm_utc_t*  timestamp);

/**
 * \brief Gets the expire date from a certificate. Will be parsed according to the date format
 *        specified in the certificate definition.
 *
 * \param[in]  cert_def   Certificate definition for the certificate.
 * \param[in]  cert       Certificate to get element from.
 * \param[in]  cert_size  Size of the certificate (cert) in bytes.
 * \param[out] timestamp  Expire date is returned in this structure.
 *
 * \return 0 on success
 */
int atcacert_get_expire_date( const atcacert_def_t*  cert_def,
                              const uint8_t*         cert,
                              size_t cert_size,
                              atcacert_tm_utc_t*     timestamp);

/**
 * \brief Sets the signer ID in a certificate. Will be formatted as 4 upper-case hex digits.
 *
 * \param[in]    cert_def   Certificate definition for the certificate.
 * \param[inout] cert       Certificate to update.
 * \param[in]    cert_size  Size of the certificate (cert) in bytes.
 * \param[in]    signer_id  Signer ID.
 *
 * \return 0 on success
 */
int atcacert_set_signer_id( const atcacert_def_t* cert_def,
                            uint8_t*              cert,
                            size_t cert_size,
                            const uint8_t signer_id[2]);

/**
 * \brief Gets the signer ID from a certificate. Will be parsed as 4 upper-case hex digits.
 *
 * \param[in]  cert_def   Certificate definition for the certificate.
 * \param[in]  cert       Certificate to get element from.
 * \param[in]  cert_size  Size of the certificate (cert) in bytes.
 * \param[out] signer_id  Signer ID will be returned in this buffer. 2 bytes.
 *
 * \return 0 on success
 */
int atcacert_get_signer_id( const atcacert_def_t * cert_def,
                            const uint8_t *        cert,
                            size_t cert_size,
                            uint8_t signer_id[2]);

/**
 * \brief Sets the certificate serial number in a certificate.
 *
 * \param[in]    cert_def      Certificate definition for the certificate.
 * \param[inout] cert          Certificate to update.
 * \param[in]    cert_size     Size of the certificate (cert) in bytes.
 * \param[in]    cert_sn       Certificate serial number.
 * \param[in]    cert_sn_size  Size of the certificate serial number in bytes.
 *
 * \return 0 on success
 */
int atcacert_set_cert_sn( const atcacert_def_t* cert_def,
                          uint8_t*              cert,
                          size_t cert_size,
                          const uint8_t*        cert_sn,
                          size_t cert_sn_size);

/**
 * \brief Sets the certificate serial number by generating it from other information in the
 *        certificate using the scheme specified by sn_source in cert_def. See the
 *
 * This method requires certain elements in the certificate be set properly as they're used for
 * generating the serial number. See atcacert_cert_sn_src_t for what elements should be set in the
 * certificate beforehand. If the sn_source is set to SNSRC_STORED, the function will return
 * ATCACERT_E_SUCCESS without making any changes to the certificate.
 *
 * \param[in]    cert_def      Certificate definition for the certificate.
 * \param[inout] cert          Certificate to update.
 * \param[in]    cert_size     Size of the certificate (cert) in bytes.
 * \param[in]    device_sn     Device serial number, only used if required by the sn_source scheme.
 *                             Can be set to NULL, if not required.
 *
 * \return 0 on success
 */
int atcacert_gen_cert_sn( const atcacert_def_t* cert_def,
                          uint8_t*              cert,
                          size_t cert_size,
                          const uint8_t device_sn[9]);

/**
 * \brief Gets the certificate serial number from a certificate.
 *
 * \param[in]    cert_def      Certificate definition for the certificate.
 * \param[in]    cert          Certificate to get element from.
 * \param[in]    cert_size     Size of the certificate (cert) in bytes.
 * \param[out]   cert_sn       Certificate SN will be returned in this buffer.
 * \param[inout] cert_sn_size  As input, the size of the cert_sn buffer.
 *                             As output, the size of the certificate SN (cert_sn) in bytes.
 *
 * \return 0 on success
 */
int atcacert_get_cert_sn( const atcacert_def_t* cert_def,
                          const uint8_t*        cert,
                          size_t cert_size,
                          uint8_t*              cert_sn,
                          size_t*               cert_sn_size);

/**
 * \brief Sets the authority key ID in a certificate. Note that this takes the actual public key
 *        creates a key ID from it.
 *
 * \param[in]    cert_def         Certificate definition for the certificate.
 * \param[inout] cert             Certificate to update.
 * \param[in]    cert_size        Size of the certificate (cert) in bytes.
 * \param[in]    auth_public_key  Authority public key as X and Y integers concatenated together.
 *                                64 bytes.
 *
 * \return 0 on success
 */
int atcacert_set_auth_key_id( const atcacert_def_t* cert_def,
                              uint8_t*              cert,
                              size_t cert_size,
                              const uint8_t auth_public_key[64]);

/**
 * \brief Gets the authority key ID from a certificate.
 *
 * \param[in]  cert_def     Certificate definition for the certificate.
 * \param[in]  cert         Certificate to get element from.
 * \param[in]  cert_size    Size of the certificate (cert) in bytes.
 * \param[out] auth_key_id  Authority key ID is returned in this buffer. 20 bytes.
 *
 * \return 0 on success
 */
int atcacert_get_auth_key_id( const atcacert_def_t * cert_def,
                              const uint8_t *        cert,
                              size_t cert_size,
                              uint8_t auth_key_id[20]);

/**
 * \brief Sets the signature, issue date, expire date, and signer ID found in the compressed
 *        certificate. This also checks fields common between the cert_def and the compressed
 *        certificate to make sure they match.
 *
 * \param[in]    cert_def       Certificate definition for the certificate.
 * \param[inout] cert           Certificate to update.
 * \param[inout] cert_size      As input, size of the certificate (cert) in bytes.
 *                              As output, the new size of the certificate.
 * \param[in]    max_cert_size  Maximum size of the cert buffer.
 * \param[in]    comp_cert      Compressed certificate. 72 bytes.
 *
 * \return 0 on success. ATCACERT_E_WRONG_CERT_DEF if the template ID, chain ID, and/or SN source
 *         don't match between the cert_def and the compressed certificate.
 */
int atcacert_set_comp_cert( const atcacert_def_t* cert_def,
                            uint8_t*              cert,
                            size_t*               cert_size,
                            size_t max_cert_size,
                            const uint8_t comp_cert[72]);

/**
 * \brief Generate the compressed certificate for the given certificate.
 *
 * \param[in]  cert_def     Certificate definition for the certificate.
 * \param[in]  cert         Certificate to generate the compressed certificate for.
 * \param[in]  cert_size    Size of the certificate (cert) in bytes.
 * \param[out] comp_cert    Compressed certificate is returned in this buffer. 72 bytes.
 *
 * \return 0 on success
 */
int atcacert_get_comp_cert( const atcacert_def_t * cert_def,
                            const uint8_t *        cert,
                            size_t cert_size,
                            uint8_t comp_cert[72]);

/**
 * \brief Get a pointer to the TBS data in a certificate.
 *
 * \param[in]  cert_def   Certificate definition for the certificate.
 * \param[in]  cert       Certificate to get the TBS data pointer for.
 * \param[in]  cert_size  Size of the certificate (cert) in bytes.
 * \param[out] tbs        Pointer to a const pointer that will be set the start of the TBS data.
 * \param[out] tbs_size   Size of the TBS data will be returned here.
 *
 * \return 0 on success
 */
int atcacert_get_tbs( const atcacert_def_t* cert_def,
                      const uint8_t*        cert,
                      size_t cert_size,
                      const uint8_t**       tbs,
                      size_t*               tbs_size);

/**
 * \brief Get the SHA256 digest of certificate's TBS data.
 *
 * \param[in]  cert_def    Certificate definition for the certificate.
 * \param[in]  cert        Certificate to get the TBS data pointer for.
 * \param[in]  cert_size   Size of the certificate (cert) in bytes.
 * \param[out] tbs_digest  TBS data digest will be returned here. 32 bytes.
 *
 * \return 0 on success
 */
int atcacert_get_tbs_digest( const atcacert_def_t * cert_def,
                             const uint8_t *        cert,
                             size_t cert_size,
                             uint8_t tbs_digest[32]);

/**
 * \brief Sets an element in a certificate. The data_size must match the size in cert_loc.
 *
 * \param[in]    cert_loc       Certificate location for this element.
 * \param[inout] cert           Certificate to update.
 * \param[in]    cert_size      Size of the certificate (cert) in bytes.
 * \param[in]    data           Element data to insert into the certificate. Buffer must contain
 *                              cert_loc.count bytes to be copied into the certificate.
 * \param[in]    data_size      Size of the data in bytes.
 *
 * \return 0 on success
 */
int atcacert_set_cert_element( const atcacert_cert_loc_t* cert_loc,
                               uint8_t*                   cert,
                               size_t cert_size,
                               const uint8_t*             data,
                               size_t data_size);

/**
 * \brief Gets an element from a certificate.
 *
 * \param[in]    cert_loc   Certificate location for this element.
 * \param[in]    cert       Certificate to get element from.
 * \param[in]    cert_size  Size of the certificate (cert) in bytes.
 * \param[out]   data       Element data will be returned in this buffer. This buffer must be large
 *                          enough to hold cert_loc.count bytes.
 * \param[in]    data_size  Expected size of the cert element data.
 *
 * \return 0 on success
 */
int atcacert_get_cert_element( const atcacert_cert_loc_t* cert_loc,
                               const uint8_t*             cert,
                               size_t cert_size,
                               uint8_t*                   data,
                               size_t data_size);


// Below are utility functions for dealing with various bits for data conversion and wrangling

/**
 * \brief Calculates the key ID for a given public ECC P256 key.
 *
 * Uses method 1 for calculating the keyIdentifier as specified by RFC 5280, section 4.2.1.2:
 *   (1) The keyIdentifier is composed of the 160-bit SHA-1 hash of the
 *        value of the BIT STRING subjectPublicKey (excluding the tag,
 *        length, and number of unused bits).
 *
 * \param[in] public_key  ECC P256 public key to calculate key key ID for. Formatted as the X and
 *                        Y integers concatenated together. 64 bytes.
 * \param[in] key_id      Calculated key ID will be returned in this buffer. 20 bytes.
 *
 * \return 0 on success
 */
int atcacert_get_key_id( const uint8_t public_key[64], uint8_t key_id[20] );

/**
 * \brief Merge a new device location into a list of device locations. If the new location overlaps
 *        with an existing location, the existing one will be modified to encompass both. Otherwise
 *        the new location is appended to the end of the list.
 *
 * The block_size parameter will adjust all added device locations to have an offset and count that
 * aligns with that block size. This allows one to generate a list of device locations that matches
 * specific read/write semantics (e.g. 4 byte or 32 byte reads). Note that this block_size only
 * applies to the device_loc being added. Existing device locations in the list won't be modified
 * to match the block size.
 *
 * \param[inout] device_locs            Existing device location list to merge the new device
 *                                      location into.
 * \param[inout] device_locs_count      As input, the existing number of items in the device_locs
 *                                      list. As output, the new size of the device_locs list.
 * \param[in]    device_locs_max_count  Maximum number of items the device_locs list can hold.
 * \param[in]    device_loc             New device location to be merged into the device_locs list.
 * \param[in]    block_size             Block size to align all offsets and counts to when adding
 *                                      device location.
 *
 * \return 0 on success
 */
int atcacert_merge_device_loc( atcacert_device_loc_t*       device_locs,
                               size_t*                      device_locs_count,
                               size_t device_locs_max_count,
                               const atcacert_device_loc_t* device_loc,
                               size_t block_size);

int atcacert_is_device_loc_overlap( const atcacert_device_loc_t* device_loc1,
                                    const atcacert_device_loc_t* device_loc2);

/**
 * \brief Takes a raw P256 ECC public key and converts it to the padded version used by ATECC
 *        devices. Input and output buffers can point to the same location to do an in-place
 *        transform.
 *
 * \param[in]  raw_key     Public key as X and Y integers concatenated together. 64 bytes.
 * \param[out] padded_key  Padded key is returned in this buffer. X and Y integers are padded
 *                         with 4 bytes of 0 in the MSB. 72 bytes.
 *
 * \return 0 on success
 */
void atcacert_public_key_add_padding( const uint8_t raw_key[64], uint8_t padded_key[72] );

/**
 * \brief Takes a padded public key used by ATECC devices and converts it to a raw P256 ECC public
 *        key. Input and output buffers can point to the same location to do an in-place transform.
 *
 * \param[out] padded_key  X and Y integers are padded with 4 bytes of 0 in the MSB. 72 bytes.
 * \param[in]  raw_key     Raw key is returned in this buffer. Public key as X and Y integers
 *                         concatenated together. 64 bytes.
 *
 * \return 0 on success
 */
void atcacert_public_key_remove_padding( const uint8_t padded_key[72], uint8_t raw_key[64] );

/** @} */
#ifdef __cplusplus
}
#endif

#endif