/**
 * \file
 * \brief Main certificate definition implementation.
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

#include "atcacert_def.h"
#include "crypto/atca_crypto_sw_sha1.h"
#include "crypto/atca_crypto_sw_sha2.h"
#include "atcacert_der.h"
#include "atcacert_date.h"
#include <string.h>

#define ATCACERT_MIN(x, y) ((x) < (y) ? (x) : (y))
#define ATCACERT_MAX(x, y) ((x) >= (y) ? (x) : (y))

int atcacert_merge_device_loc( atcacert_device_loc_t*       device_locs,
                               size_t*                      device_locs_count,
                               size_t device_locs_max_count,
                               const atcacert_device_loc_t* device_loc,
                               size_t block_size)
{
	size_t i = 0;
	size_t new_offset;
	size_t new_end;

	if (device_locs == NULL || device_locs_count == NULL || device_loc == NULL || block_size <= 0)
		return ATCACERT_E_BAD_PARAMS;

	if (device_loc->zone == DEVZONE_NONE || device_loc->count == 0)
		return ATCACERT_E_SUCCESS;                                                                      // New device location doesn't exist

	new_offset = (device_loc->offset / block_size) * block_size;                                        // Round down to block_size
	new_end = device_loc->offset + device_loc->count;
	new_end = ((new_end % block_size) ? new_end / block_size + 1 : new_end / block_size) * block_size;  // Round up to block size

	// Try to merge with an existing device location
	for (i = 0; i < *device_locs_count; ++i) {
		atcacert_device_loc_t* cur_device_loc = &device_locs[i];
		size_t cur_end = cur_device_loc->offset + cur_device_loc->count;

		if (device_loc->zone != cur_device_loc->zone)
			continue;   // Not the same zone, can't merge
		if (device_loc->zone == DEVZONE_DATA && device_loc->slot != cur_device_loc->slot)
			continue;   // Not the same slot, can't merge
		if (device_loc->zone == DEVZONE_DATA && device_loc->is_genkey != cur_device_loc->is_genkey)
			continue;   // Not the same read method, can't merge.
		if (new_end < cur_device_loc->offset || new_offset > cur_end)
			continue;   // Same zone, but non-continuous areas

		if (device_loc->offset < cur_device_loc->offset)
			cur_device_loc->offset = device_loc->offset;

		if (new_end > cur_end)
			cur_device_loc->count = (uint16_t)(new_end - cur_device_loc->offset);
		else
			cur_device_loc->count = (uint16_t)(cur_end - cur_device_loc->offset);
		break;
	}

	if (i == *device_locs_count) {
		// New device_loc wasn't merged into an existing one, add to the end of the list
		if (*device_locs_count >= device_locs_max_count)
			return ATCACERT_E_BUFFER_TOO_SMALL; // No room to add to list

		device_locs[*device_locs_count] = *device_loc;
		// Adjust for block size
		device_locs[*device_locs_count].offset = (uint16_t)new_offset;
		device_locs[*device_locs_count].count = (uint16_t)(new_end - new_offset);
		(*device_locs_count)++;
	}

	return ATCACERT_E_SUCCESS;
}

int atcacert_get_device_locs( const atcacert_def_t*  cert_def,
                              atcacert_device_loc_t* device_locs,
                              size_t*                device_locs_count,
                              size_t device_locs_max_count,
                              size_t block_size)
{
	int ret = 0;
	size_t i;

	if (cert_def == NULL || device_locs == NULL || device_locs_count == NULL)
		return ATCACERT_E_BAD_PARAMS;

	ret = atcacert_merge_device_loc(
	    device_locs,
	    device_locs_count,
	    device_locs_max_count,
	    &cert_def->cert_sn_dev_loc,
	    block_size);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_merge_device_loc(
	    device_locs,
	    device_locs_count,
	    device_locs_max_count,
	    &cert_def->public_key_dev_loc,
	    block_size);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_merge_device_loc(
	    device_locs,
	    device_locs_count,
	    device_locs_max_count,
	    &cert_def->comp_cert_dev_loc,
	    block_size);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	if (cert_def->cert_elements_count > 0 && cert_def->cert_elements == NULL)
		return ATCACERT_E_BAD_CERT; // Cert def is in an invalid state

	for (i = 0; i < cert_def->cert_elements_count; i++) {
		ret = atcacert_merge_device_loc(
		    device_locs,
		    device_locs_count,
		    device_locs_max_count,
		    &cert_def->cert_elements[i].device_loc,
		    block_size);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
	}

	// Add the device SN to the list if the cert serial number scheme requires it
	if (   cert_def->sn_source == SNSRC_DEVICE_SN
	       || cert_def->sn_source == SNSRC_DEVICE_SN_HASH
	       || cert_def->sn_source == SNSRC_DEVICE_SN_HASH_POS
	       || cert_def->sn_source == SNSRC_DEVICE_SN_HASH_RAW) {
		// Device SN is config zone bytes 0-3 and 8-12
		atcacert_device_loc_t device_sn_loc = {
			.zone		= DEVZONE_CONFIG,
			.slot		= 0,                // Ignored
			.is_genkey	= FALSE,            // Ignored
			.offset		= 0,
			.count		= 13
		};
		ret = atcacert_merge_device_loc(
		    device_locs,
		    device_locs_count,
		    device_locs_max_count,
		    &device_sn_loc,
		    block_size);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
	}

	return ATCACERT_E_SUCCESS;
}

static const uint8_t* atcacert_is_device_loc_match( const atcacert_device_loc_t* device_loc_dest,
                                                    const atcacert_device_loc_t* device_loc_src,
                                                    const uint8_t* src_data)
{
	size_t dest_end = device_loc_dest->offset + device_loc_dest->count;
	size_t src_end = device_loc_src->offset + device_loc_src->count;

	if (device_loc_dest->zone == DEVZONE_NONE || device_loc_dest->count <= 0)
		return NULL;    // device_loc_dest is a null location
	if (device_loc_src->zone == DEVZONE_NONE || device_loc_src->count <= 0)
		return NULL;    // device_loc_src is a null location
	if (device_loc_dest->zone != device_loc_src->zone)
		return NULL;
	if (device_loc_dest->zone == DEVZONE_DATA && device_loc_dest->slot != device_loc_src->slot)
		return NULL;
	if (device_loc_dest->zone == DEVZONE_DATA && device_loc_dest->is_genkey != device_loc_src->is_genkey)
		return NULL;

	if (device_loc_dest->offset < device_loc_src->offset || dest_end > src_end)
		return NULL;

	// device_loc_dest is encompassed by device_loc_src, return a pointer to the data for device_loc_dest
	return src_data + (device_loc_dest->offset - device_loc_src->offset);
}

int atcacert_cert_build_start( atcacert_build_state_t* build_state,
                               const atcacert_def_t*   cert_def,
                               uint8_t*                cert,
                               size_t*                 cert_size,
                               const uint8_t ca_public_key[64])
{
	int ret = 0;

	if (build_state == NULL || cert_def == NULL || cert == NULL || cert_size == NULL || ca_public_key == NULL)
		return ATCACERT_E_BAD_PARAMS;

	memset(build_state, 0, sizeof(*build_state));

	build_state->cert_def      = cert_def;
	build_state->cert          = cert;
	build_state->cert_size     = cert_size;
	build_state->max_cert_size = *cert_size;
	build_state->is_device_sn  = FALSE;

	if (build_state->max_cert_size < build_state->cert_def->cert_template_size) {
		*build_state->cert_size = build_state->cert_def->cert_template_size;
		return ATCACERT_E_BUFFER_TOO_SMALL; // cert buffer is too small to contain the template
	}

	// Initialize the cert buffer with the cert template
	*build_state->cert_size = build_state->cert_def->cert_template_size;
	memcpy(build_state->cert, build_state->cert_def->cert_template, build_state->cert_def->cert_template_size);

	if (build_state->cert_def->type == CERTTYPE_X509) {
		// Set a fake signature that should result in the largest X.509 cert. This will ensure
		// the cert buffer is large enough early in the cert rebuilding process.
		uint8_t large_sig[64];
		memset(large_sig, 0xFF, sizeof(large_sig));
		ret = atcacert_set_signature(
		    build_state->cert_def,
		    build_state->cert,
		    build_state->cert_size,
		    build_state->max_cert_size,
		    large_sig);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
	}

	// Set the authority key ID
	ret = atcacert_set_auth_key_id(
	    build_state->cert_def,
	    build_state->cert,
	    *build_state->cert_size,
	    ca_public_key);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	return ATCACERT_E_SUCCESS;
}

int atcacert_cert_build_process( atcacert_build_state_t*      build_state,
                                 const atcacert_device_loc_t* device_loc,
                                 const uint8_t*               device_data)
{
	int ret = 0;
	size_t i = 0;
	const uint8_t* data = NULL;
	uint8_t public_key[64];
	static const atcacert_device_loc_t device_sn_dev_loc = {
		.zone		= DEVZONE_CONFIG,
		.slot		= 0,
		.is_genkey	= FALSE,
		.offset		= 0,
		.count		= 13
	};

	if (build_state == NULL || device_loc == NULL || device_data == NULL)
		return ATCACERT_E_BAD_PARAMS;

	data = atcacert_is_device_loc_match(&build_state->cert_def->cert_sn_dev_loc, device_loc, device_data);
	if (data != NULL) {
		ret = atcacert_set_cert_sn(
		    build_state->cert_def,
		    build_state->cert,
		    *build_state->cert_size,
		    data,
		    build_state->cert_def->cert_sn_dev_loc.count);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
	}

	data = atcacert_is_device_loc_match(&build_state->cert_def->public_key_dev_loc, device_loc, device_data);
	if (data != NULL) {
		if (build_state->cert_def->public_key_dev_loc.count == 72) {
			// Public key is formatted with padding bytes in front of the X and Y components
			atcacert_public_key_remove_padding(data, public_key);
			ret = atcacert_set_subj_public_key(
			    build_state->cert_def,
			    build_state->cert,
			    *build_state->cert_size,
			    public_key);
		}else if (build_state->cert_def->public_key_dev_loc.count == 64) {
			ret = atcacert_set_subj_public_key(
			    build_state->cert_def,
			    build_state->cert,
			    *build_state->cert_size,
			    data);
		}else
			return ATCACERT_E_BAD_CERT; // Unexpected public key size

		if (ret != ATCACERT_E_SUCCESS)
			return ret;
	}

	data = atcacert_is_device_loc_match(&build_state->cert_def->comp_cert_dev_loc, device_loc, device_data);
	if (data != NULL) {
		if (build_state->cert_def->comp_cert_dev_loc.count != 72)
			return ATCACERT_E_BAD_CERT; // Unexpected compressed certificate size

		ret = atcacert_set_comp_cert(
		    build_state->cert_def,
		    build_state->cert,
		    build_state->cert_size,
		    build_state->max_cert_size,
		    data);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
	}

	if (build_state->cert_def->cert_elements_count > 0 && build_state->cert_def->cert_elements == NULL)
		return ATCACERT_E_BAD_CERT;
	for (i = 0; i < build_state->cert_def->cert_elements_count; i++) {
		data = atcacert_is_device_loc_match(&build_state->cert_def->cert_elements[i].device_loc, device_loc, device_data);
		if (data != NULL) {
			if (build_state->cert_def->cert_elements[i].device_loc.count != build_state->cert_def->cert_elements[i].cert_loc.count)
				return ATCACERT_E_BAD_CERT;
			ret = atcacert_set_cert_element(
			    &build_state->cert_def->cert_elements[i].cert_loc,
			    build_state->cert,
			    *build_state->cert_size,
			    data,
			    build_state->cert_def->cert_elements[i].cert_loc.count);
			if (ret != ATCACERT_E_SUCCESS)
				return ret;
		}
	}

	data = atcacert_is_device_loc_match(&device_sn_dev_loc, device_loc, device_data);
	if (data != NULL) {
		// Get the device SN
		build_state->is_device_sn = TRUE;
		memcpy(&build_state->device_sn[0], &data[0], 4);
		memcpy(&build_state->device_sn[4], &data[8], 5);
	}

	return ATCACERT_E_SUCCESS;
}

int atcacert_cert_build_finish( atcacert_build_state_t* build_state )
{
	int ret = 0;
	const uint8_t* device_sn = NULL;

	if (build_state == NULL)
		return ATCACERT_E_BAD_PARAMS;

	if (build_state->is_device_sn)
		device_sn = build_state->device_sn;

	ret = atcacert_gen_cert_sn(build_state->cert_def, build_state->cert, *build_state->cert_size, device_sn);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	return ret;
}

int atcacert_is_device_loc_overlap( const atcacert_device_loc_t* device_loc1,
                                    const atcacert_device_loc_t* device_loc2)
{
	if (device_loc1->zone != device_loc2->zone)
		return FALSE;   // Zones don't match, can't overlap
	if (device_loc1->zone == DEVZONE_DATA && device_loc1->slot != device_loc2->slot)
		return FALSE;   // Data zone, slots don't match, can't overlap
	if (device_loc1->zone == DEVZONE_DATA && device_loc1->is_genkey != device_loc2->is_genkey)
		return FALSE;   // Data zone, same slot, but read method doesn't match, can't overlap

	return !(   device_loc1->offset + device_loc1->count <= device_loc2->offset
	            || device_loc1->offset >= device_loc2->offset + device_loc2->count);
}

static void atcacert_copy_device_loc_data( const atcacert_device_loc_t* device_loc_src,
                                           const uint8_t*               data_src,
                                           const atcacert_device_loc_t* device_loc_dest,
                                           uint8_t*                     data_dest)
{
	size_t offset = ATCACERT_MAX(device_loc_src->offset, device_loc_dest->offset);
	size_t end    = ATCACERT_MIN(device_loc_src->offset + device_loc_src->count, device_loc_dest->offset + device_loc_dest->count);

	memcpy(&data_dest[offset - device_loc_dest->offset], &data_src[offset - device_loc_src->offset], end - offset);
}

int atcacert_get_device_data( const atcacert_def_t*        cert_def,
                              const uint8_t*               cert,
                              size_t cert_size,
                              const atcacert_device_loc_t* device_loc,
                              uint8_t*                     device_data)
{
	int ret = 0;
	int i = 0;
	uint8_t temp_buf[256]; // Must be at least 72 bytes
	size_t temp_buf_size = sizeof(temp_buf);

	if (cert_def == NULL || cert == NULL || device_loc == NULL || device_data == NULL)
		return ATCACERT_E_BAD_PARAMS;

	// Certificate serial number
	if (atcacert_is_device_loc_overlap(&cert_def->cert_sn_dev_loc, device_loc)) {
		temp_buf_size = sizeof(temp_buf);
		ret = atcacert_get_cert_sn(cert_def, cert, cert_size, temp_buf, &temp_buf_size);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
		atcacert_copy_device_loc_data(&cert_def->cert_sn_dev_loc, temp_buf, device_loc, device_data);
	}

	// Subject public key
	if (atcacert_is_device_loc_overlap(&cert_def->public_key_dev_loc, device_loc)) {
		ret = atcacert_get_subj_public_key(cert_def, cert, cert_size, temp_buf);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
		if (cert_def->public_key_dev_loc.count == 72)
			// Public key is formatted with padding bytes in front of the X and Y components
			atcacert_public_key_add_padding(temp_buf, temp_buf);
		else if (cert_def->public_key_dev_loc.count != 64)
			return ATCACERT_E_BAD_CERT; // Unexpected public key size
		atcacert_copy_device_loc_data(&cert_def->public_key_dev_loc, temp_buf, device_loc, device_data);
	}

	// Compressed certificate
	if (atcacert_is_device_loc_overlap(&cert_def->comp_cert_dev_loc, device_loc)) {
		ret = atcacert_get_comp_cert(cert_def, cert, cert_size, temp_buf);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
		if (cert_def->comp_cert_dev_loc.count != 72)
			return ATCACERT_E_BAD_CERT; // Unexpected compressed certificate size
		atcacert_copy_device_loc_data(&cert_def->comp_cert_dev_loc, temp_buf, device_loc, device_data);
	}

	// Additional custom certificate elements
	if (cert_def->cert_elements_count > 0 && cert_def->cert_elements == NULL)
		return ATCACERT_E_BAD_CERT;
	for (i = 0; i < cert_def->cert_elements_count; i++) {
		if (atcacert_is_device_loc_overlap(&cert_def->cert_elements[i].device_loc, device_loc)) {
			if (sizeof(temp_buf) < cert_def->cert_elements[i].device_loc.count)
				return ATCACERT_E_BUFFER_TOO_SMALL;
			ret = atcacert_get_cert_element(
			    &cert_def->cert_elements[i].cert_loc,
			    cert,
			    cert_size,
			    temp_buf,
			    cert_def->cert_elements[i].device_loc.count);
			if (ret != ATCACERT_E_SUCCESS)
				return ret;
			atcacert_copy_device_loc_data(&cert_def->cert_elements[i].device_loc, temp_buf, device_loc, device_data);
		}
	}

	return ATCACERT_E_SUCCESS;
}

int atcacert_set_subj_public_key( const atcacert_def_t* cert_def,
                                  uint8_t*              cert,
                                  size_t cert_size,
                                  const uint8_t subj_public_key[64])
{
	int ret = 0;
	uint8_t key_id[20];

	if (cert_def == NULL || cert == NULL || subj_public_key == NULL)
		return ATCACERT_E_BAD_PARAMS;

	ret = atcacert_set_cert_element(&cert_def->std_cert_elements[STDCERT_PUBLIC_KEY], cert, cert_size, subj_public_key, 64);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_get_key_id(subj_public_key, key_id);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_set_cert_element(&cert_def->std_cert_elements[STDCERT_SUBJ_KEY_ID], cert, cert_size, key_id, 20);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	return ATCACERT_E_SUCCESS;
}

int atcacert_get_subj_public_key( const atcacert_def_t* cert_def,
                                  const uint8_t*        cert,
                                  size_t cert_size,
                                  uint8_t subj_public_key[64])
{
	if (cert_def == NULL || cert == NULL || subj_public_key == NULL)
		return ATCACERT_E_BAD_PARAMS;

	return atcacert_get_cert_element(&cert_def->std_cert_elements[STDCERT_PUBLIC_KEY], cert, cert_size, subj_public_key, 64);
}

int atcacert_get_subj_key_id( const atcacert_def_t* cert_def,
                              const uint8_t*        cert,
                              size_t cert_size,
                              uint8_t subj_key_id[20])
{
	if (cert_def == NULL || cert == NULL || subj_key_id == NULL)
		return ATCACERT_E_BAD_PARAMS;

	return atcacert_get_cert_element(&cert_def->std_cert_elements[STDCERT_SUBJ_KEY_ID], cert, cert_size, subj_key_id, 20);
}

int atcacert_set_signature( const atcacert_def_t* cert_def,
                            uint8_t*              cert,
                            size_t*               cert_size,
                            size_t max_cert_size,
                            const uint8_t signature[64])
{
	int ret = 0;
	uint16_t sig_offset;
	size_t cur_der_sig_size;
	size_t new_der_sig_size;
	size_t old_cert_der_length_size;
	size_t new_cert_der_length_size;
	size_t new_cert_length;
	uint8_t new_cert_der_length[5];

	if (cert_def == NULL || cert == NULL || cert_size == NULL || signature == NULL)
		return ATCACERT_E_BAD_PARAMS;

	sig_offset = cert_def->std_cert_elements[STDCERT_SIGNATURE].offset;

	// Non X.509 signatures are treated like normal certificate elements
	if (cert_def->type != CERTTYPE_X509)
		return atcacert_set_cert_element(&cert_def->std_cert_elements[STDCERT_SIGNATURE], cert, *cert_size, signature, 64);

	if (sig_offset >= *cert_size)
		return ATCACERT_E_ELEM_OUT_OF_BOUNDS; // Signature element is shown as past the end of the certificate

	// Current size of the signature is from its offset to the end of the cert
	cur_der_sig_size = *cert_size - sig_offset;

	// Find the size of buffer available for the new DER signature
	new_der_sig_size = max_cert_size - sig_offset;

	// Set the new signature
	ret = atcacert_der_enc_ecdsa_sig_value(signature, &cert[sig_offset], &new_der_sig_size);
	if (ret != ATCACERT_E_SUCCESS) {
		if (ret == ATCACERT_E_BUFFER_TOO_SMALL)
			*cert_size += (int)new_der_sig_size - (int)cur_der_sig_size; // Report the size needed
		return ret;
	}

	*cert_size += (int)new_der_sig_size - (int)cur_der_sig_size;

	old_cert_der_length_size = *cert_size - 1;
	ret = atcacert_der_dec_length(&cert[1], &old_cert_der_length_size, NULL);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	if ((1 + old_cert_der_length_size) > *cert_size)
		return ATCACERT_E_BAD_CERT;
	new_cert_length = *cert_size - (1 + old_cert_der_length_size);

	new_cert_der_length_size = sizeof(new_cert_der_length);
	ret = atcacert_der_enc_length(new_cert_length, new_cert_der_length, &new_cert_der_length_size);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	if (old_cert_der_length_size != new_cert_der_length_size)
		return ATCACERT_E_BAD_CERT;

	memcpy(&cert[1], new_cert_der_length, new_cert_der_length_size);

	return ATCACERT_E_SUCCESS;
}

int atcacert_get_signature( const atcacert_def_t* cert_def,
                            const uint8_t*        cert,
                            size_t cert_size,
                            uint8_t signature[64])
{
	uint16_t sig_offset;
	size_t der_sig_size = 0;

	if (cert_def == NULL || cert == NULL || signature == NULL)
		return ATCACERT_E_BAD_PARAMS;

	sig_offset = cert_def->std_cert_elements[STDCERT_SIGNATURE].offset;

	// Non X.509 signatures are treated like normal certificate elements
	if (cert_def->type != CERTTYPE_X509)
		return atcacert_get_cert_element(&cert_def->std_cert_elements[STDCERT_SIGNATURE], cert, cert_size, signature, 64);

	if (sig_offset >= cert_size)
		return ATCACERT_E_ELEM_OUT_OF_BOUNDS; // Signature element is shown as past the end of the certificate

	der_sig_size = cert_size - sig_offset;
	return atcacert_der_dec_ecdsa_sig_value(&cert[sig_offset], &der_sig_size, signature);
}

int atcacert_set_issue_date( const atcacert_def_t*     cert_def,
                             uint8_t*                  cert,
                             size_t cert_size,
                             const atcacert_tm_utc_t*  timestamp)
{
	int ret = 0;
	uint8_t formatted_date[DATEFMT_MAX_SIZE];
	size_t formatted_date_size = sizeof(formatted_date);

	if (cert_def == NULL || cert == NULL || timestamp == NULL)
		return ATCACERT_E_BAD_PARAMS;

	if (cert_def->std_cert_elements[STDCERT_ISSUE_DATE].count == 0)
		return ATCACERT_E_SUCCESS; // No issue date to be set

	ret = atcacert_date_enc(cert_def->issue_date_format, timestamp, formatted_date, &formatted_date_size);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_set_cert_element(&cert_def->std_cert_elements[STDCERT_ISSUE_DATE], cert, cert_size, formatted_date, formatted_date_size);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	return ATCACERT_E_SUCCESS;
}

int atcacert_get_issue_date( const atcacert_def_t*  cert_def,
                             const uint8_t*         cert,
                             size_t cert_size,
                             atcacert_tm_utc_t*     timestamp)
{
	int ret = 0;
	uint8_t formatted_date[DATEFMT_MAX_SIZE];
	size_t formatted_date_size = sizeof(formatted_date);

	if (cert_def == NULL || cert == NULL || timestamp == NULL)
		return ATCACERT_E_BAD_PARAMS;

	if (cert_def->issue_date_format > sizeof(ATCACERT_DATE_FORMAT_SIZES) / sizeof(ATCACERT_DATE_FORMAT_SIZES[0]))
		return ATCACERT_E_ERROR; // Format is out of range

	formatted_date_size = ATCACERT_DATE_FORMAT_SIZES[cert_def->issue_date_format];
	if (formatted_date_size > sizeof(formatted_date))
		return ATCACERT_E_ERROR; // DATEFMT_MAX_SIZE is wrong

	ret = atcacert_get_cert_element(&cert_def->std_cert_elements[STDCERT_ISSUE_DATE], cert, cert_size, formatted_date, formatted_date_size);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_date_dec(cert_def->issue_date_format, formatted_date, formatted_date_size, timestamp);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	return ATCACERT_E_SUCCESS;
}

int atcacert_set_expire_date( const atcacert_def_t*     cert_def,
                              uint8_t*                  cert,
                              size_t cert_size,
                              const atcacert_tm_utc_t*  timestamp)
{
	int ret = 0;
	uint8_t formatted_date[DATEFMT_MAX_SIZE];
	size_t formatted_date_size = sizeof(formatted_date);

	if (cert_def == NULL || cert == NULL || timestamp == NULL)
		return ATCACERT_E_BAD_PARAMS;

	if (cert_def->std_cert_elements[STDCERT_EXPIRE_DATE].count == 0)
		return ATCACERT_E_SUCCESS; // No expire date to be set

	ret = atcacert_date_enc(cert_def->expire_date_format, timestamp, formatted_date, &formatted_date_size);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_set_cert_element(&cert_def->std_cert_elements[STDCERT_EXPIRE_DATE], cert, cert_size, formatted_date, formatted_date_size);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	return ATCACERT_E_SUCCESS;
}

int atcacert_get_expire_date( const atcacert_def_t*  cert_def,
                              const uint8_t*         cert,
                              size_t cert_size,
                              atcacert_tm_utc_t*     timestamp)
{
	int ret = 0;
	uint8_t formatted_date[DATEFMT_MAX_SIZE];
	size_t formatted_date_size = sizeof(formatted_date);

	if (cert_def == NULL || cert == NULL || timestamp == NULL)
		return ATCACERT_E_BAD_PARAMS;

	if (cert_def->expire_date_format > sizeof(ATCACERT_DATE_FORMAT_SIZES) / sizeof(ATCACERT_DATE_FORMAT_SIZES[0]))
		return ATCACERT_E_ERROR; // Format is out of range

	formatted_date_size = ATCACERT_DATE_FORMAT_SIZES[cert_def->expire_date_format];
	if (formatted_date_size > sizeof(formatted_date))
		return ATCACERT_E_ERROR; // DATEFMT_MAX_SIZE is wrong

	ret = atcacert_get_cert_element(&cert_def->std_cert_elements[STDCERT_EXPIRE_DATE], cert, cert_size, formatted_date, formatted_date_size);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_date_dec(cert_def->expire_date_format, formatted_date, formatted_date_size, timestamp);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	return ATCACERT_E_SUCCESS;
}

static void uint8_to_hex(uint8_t num, uint8_t* hex_str)
{
	uint8_t nibble = (num >> 4) & 0x0F;

	if (nibble < 10)
		*(hex_str++) = '0' + nibble;
	else
		*(hex_str++) = 'A' + (nibble - 10);
	nibble = num & 0x0F;
	if (nibble < 10)
		*(hex_str++) = '0' + nibble;
	else
		*(hex_str++) = 'A' + (nibble - 10);
}

int atcacert_set_signer_id( const atcacert_def_t* cert_def,
                            uint8_t*              cert,
                            size_t cert_size,
                            const uint8_t signer_id[2])
{
	uint8_t hex_str[4];

	if (cert_def == NULL || cert == NULL || signer_id == NULL)
		return ATCACERT_E_BAD_PARAMS;

	uint8_to_hex(signer_id[0], &hex_str[0]);
	uint8_to_hex(signer_id[1], &hex_str[2]);

	return atcacert_set_cert_element(&cert_def->std_cert_elements[STDCERT_SIGNER_ID], cert, cert_size, hex_str, 4);
}

static int hex_to_uint8(const uint8_t hex_str[2], uint8_t* num)
{
	*num = 0;

	if (hex_str[0] >= '0' && hex_str[0] <= '9')
		*num += (hex_str[0] - '0') << 4;
	else if (hex_str[0] >= 'A' && hex_str[0] <= 'F')
		*num += (hex_str[0] - 'A' + 10) << 4;
	else if (hex_str[0] >= 'a' && hex_str[0] <= 'f')
		*num += (hex_str[0] - 'a' + 10) << 4;
	else
		return ATCACERT_E_DECODING_ERROR;

	if (hex_str[1] >= '0' && hex_str[1] <= '9')
		*num += (hex_str[1] - '0');
	else if (hex_str[1] >= 'A' && hex_str[1] <= 'F')
		*num += (hex_str[1] - 'A' + 10);
	else if (hex_str[1] >= 'a' && hex_str[1] <= 'f')
		*num += (hex_str[1] - 'a' + 10);
	else
		return ATCACERT_E_DECODING_ERROR;

	return ATCACERT_E_SUCCESS;
}

int atcacert_get_signer_id( const atcacert_def_t* cert_def,
                            const uint8_t*        cert,
                            size_t cert_size,
                            uint8_t signer_id[2])
{
	int ret = 0;
	uint8_t hex_str[4];

	if (cert_def == NULL || cert == NULL || signer_id == NULL)
		return ATCACERT_E_BAD_PARAMS;

	ret = atcacert_get_cert_element(&cert_def->std_cert_elements[STDCERT_SIGNER_ID], cert, cert_size, hex_str, 4);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = hex_to_uint8(&hex_str[0], &signer_id[0]);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = hex_to_uint8(&hex_str[2], &signer_id[1]);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	return ATCACERT_E_SUCCESS;
}

int atcacert_set_cert_sn( const atcacert_def_t* cert_def,
                          uint8_t*              cert,
                          size_t cert_size,
                          const uint8_t*        cert_sn,
                          size_t cert_sn_size)
{
	if (cert_def == NULL || cert == NULL || cert_sn == NULL)
		return ATCACERT_E_BAD_PARAMS;

	return atcacert_set_cert_element(&cert_def->std_cert_elements[STDCERT_CERT_SN], cert, cert_size, cert_sn, cert_sn_size);
}

int atcacert_gen_cert_sn( const atcacert_def_t* cert_def,
                          uint8_t*              cert,
                          size_t cert_size,
                          const uint8_t device_sn[9])
{
	int ret = 0;
	size_t sn_size = 0;
	uint8_t msg[64 + 3];
	uint8_t sn[32];
	atcacert_tm_utc_t issue_date;

	if (cert_def == NULL || cert == NULL)
		return ATCACERT_E_BAD_PARAMS;

	if (cert_def->sn_source == SNSRC_STORED || cert_def->std_cert_elements[STDCERT_CERT_SN].count == 0)
		return ATCACERT_E_SUCCESS; // Certificate serial number is not generated or not in the certificate

	switch (cert_def->sn_source) {
	case SNSRC_DEVICE_SN:     // Cert serial number is 0x40(MSB) + 9-byte device serial number. Only applies to device certificates.
		if (device_sn == NULL)
			return ATCACERT_E_BAD_PARAMS;
		sn_size = 1 + 9;
		sn[0] = 0x40;
		memcpy(&sn[1], device_sn, 9);
		break;

	case SNSRC_SIGNER_ID:     // Cert serial number is 0x40(MSB) + 2-byte signer ID. Only applies to signer certificates.
		sn_size = 1 + 2;
		sn[0] = 0x40;
		ret = atcacert_get_signer_id(cert_def, cert, cert_size, &sn[1]);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
		break;

	case SNSRC_PUB_KEY_HASH_RAW:     // Cert serial number is the SHA256(Subject public key + Encoded dates)
	case SNSRC_PUB_KEY_HASH_POS:
	case SNSRC_PUB_KEY_HASH:
		if (cert_def->std_cert_elements[STDCERT_CERT_SN].count > 32)
			return ATCACERT_E_UNEXPECTED_ELEM_SIZE;
		sn_size = cert_def->std_cert_elements[STDCERT_CERT_SN].count;

		// Add public key to hash input
		ret = atcacert_get_subj_public_key(cert_def, cert, cert_size, &msg[0]);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;

		// Add compressed/encoded dates to hash input
		ret = atcacert_get_issue_date(cert_def, cert, cert_size, &issue_date);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
		ret = atcacert_date_enc_compcert(&issue_date, cert_def->expire_years, &msg[64]);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
		ret = atcac_sw_sha2_256(msg, 64 + 3, sn);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;

		if (cert_def->sn_source == SNSRC_PUB_KEY_HASH_POS || cert_def->sn_source == SNSRC_PUB_KEY_HASH)
			sn[0] &= 0x7F;      // Ensure the SN is positive
		if (cert_def->sn_source == SNSRC_PUB_KEY_HASH)
			sn[0] |= 0x40;      // Ensure the SN doesn't have any trimmable bytes
		break;

	case SNSRC_DEVICE_SN_HASH_RAW:     // Cert serial number is the SHA256(Device SN + Encoded dates). Only applies to device certificates.
	case SNSRC_DEVICE_SN_HASH_POS:
	case SNSRC_DEVICE_SN_HASH:
		if (device_sn == NULL)
			return ATCACERT_E_BAD_PARAMS;
		if (cert_def->std_cert_elements[STDCERT_CERT_SN].count > 32)
			return ATCACERT_E_UNEXPECTED_ELEM_SIZE;
		sn_size = cert_def->std_cert_elements[STDCERT_CERT_SN].count;

		// Add device SN to the hash input
		memcpy(&msg[0], device_sn, 9);

		// Add compressed/encoded dates to hash input
		ret = atcacert_get_issue_date(cert_def, cert, cert_size, &issue_date);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
		ret = atcacert_date_enc_compcert(&issue_date, cert_def->expire_years, &msg[9]);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
		ret = atcac_sw_sha2_256(msg, 9 + 3, sn);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;

		if (cert_def->sn_source == SNSRC_DEVICE_SN_HASH_POS || cert_def->sn_source == SNSRC_DEVICE_SN_HASH)
			sn[0] &= 0x7F;      // Ensure the SN is positive
		if (cert_def->sn_source == SNSRC_DEVICE_SN_HASH)
			sn[0] |= 0x40;      // Ensure the SN doesn't have any trimmable bytes
		break;

	default:
		return ATCACERT_E_BAD_PARAMS;
	}

	return atcacert_set_cert_element(&cert_def->std_cert_elements[STDCERT_CERT_SN], cert, cert_size, sn, sn_size);
}

int atcacert_get_cert_sn( const atcacert_def_t* cert_def,
                          const uint8_t*        cert,
                          size_t cert_size,
                          uint8_t*              cert_sn,
                          size_t*               cert_sn_size)
{
	if (cert_def == NULL || cert == NULL || cert_sn == NULL || cert_sn_size == NULL)
		return ATCACERT_E_BAD_PARAMS;

	if (*cert_sn_size < cert_def->std_cert_elements[STDCERT_CERT_SN].count) {
		*cert_sn_size = cert_def->std_cert_elements[STDCERT_CERT_SN].count;
		return ATCACERT_E_BUFFER_TOO_SMALL;
	}

	*cert_sn_size = cert_def->std_cert_elements[STDCERT_CERT_SN].count;

	return atcacert_get_cert_element(&cert_def->std_cert_elements[STDCERT_CERT_SN], cert, cert_size, cert_sn, *cert_sn_size);
}

int atcacert_set_auth_key_id( const atcacert_def_t* cert_def,
                              uint8_t*              cert,
                              size_t cert_size,
                              const uint8_t auth_public_key[64])
{
	int ret = 0;
	uint8_t key_id[20];

	if (cert_def == NULL || cert == NULL || auth_public_key == NULL)
		return ATCACERT_E_BAD_PARAMS;

	ret = atcacert_get_key_id(auth_public_key, key_id);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_set_cert_element(&cert_def->std_cert_elements[STDCERT_AUTH_KEY_ID], cert, cert_size, key_id, 20);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	return ATCACERT_E_SUCCESS;
}

int atcacert_get_auth_key_id( const atcacert_def_t* cert_def,
                              const uint8_t*        cert,
                              size_t cert_size,
                              uint8_t auth_key_id[20])
{
	if (cert_def == NULL || cert == NULL || auth_key_id == NULL)
		return ATCACERT_E_BAD_PARAMS;

	return atcacert_get_cert_element(&cert_def->std_cert_elements[STDCERT_AUTH_KEY_ID], cert, cert_size, auth_key_id, 20);
}

int atcacert_set_comp_cert( const atcacert_def_t* cert_def,
                            uint8_t*              cert,
                            size_t*               cert_size,
                            size_t max_cert_size,
                            const uint8_t comp_cert[72])
{
	int ret = 0;
	uint8_t enc_dates[3];
	uint8_t signer_id[2];
	uint8_t template_id;
	uint8_t chain_id;
	uint8_t sn_source;
	uint8_t format;
	atcacert_tm_utc_t issue_date;
	atcacert_tm_utc_t expire_date;

	if (cert_def == NULL || cert == NULL || cert_size == NULL || comp_cert == NULL)
		return ATCACERT_E_BAD_PARAMS;

	format = comp_cert[70] & 0x0F;
	if (format != 0)
		return ATCACERT_E_DECODING_ERROR; // Unknown format

	memcpy(enc_dates, &comp_cert[64], 3);
	memcpy(signer_id, &comp_cert[67], 2);
	template_id = (comp_cert[69] >> 4) & 0x0F;
	chain_id = comp_cert[69] & 0x0F;
	sn_source = (comp_cert[70] >> 4) & 0x0F;

	if (template_id != cert_def->template_id || chain_id != cert_def->chain_id || sn_source != cert_def->sn_source)
		return ATCACERT_E_WRONG_CERT_DEF;

	ret = atcacert_set_signature(
	    cert_def,
	    cert,
	    cert_size,
	    max_cert_size,
	    &comp_cert[0]);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_date_dec_compcert(enc_dates, cert_def->expire_date_format, &issue_date, &expire_date);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_set_issue_date(cert_def, cert, *cert_size, &issue_date);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_set_expire_date(cert_def, cert, *cert_size, &expire_date);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_set_signer_id(cert_def, cert, *cert_size, signer_id);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	return ATCACERT_E_SUCCESS;
}

int atcacert_get_comp_cert( const atcacert_def_t* cert_def,
                            const uint8_t*        cert,
                            size_t cert_size,
                            uint8_t comp_cert[72])
{
	int ret = 0;
	atcacert_tm_utc_t issue_date;

	if (cert_def == NULL || cert == NULL || comp_cert == NULL)
		return ATCACERT_E_BAD_PARAMS;

	ret = atcacert_get_signature(cert_def, cert, cert_size, &comp_cert[0]);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_get_issue_date(cert_def, cert, cert_size, &issue_date);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;
	ret = atcacert_date_enc_compcert(&issue_date, cert_def->expire_years, &comp_cert[64]);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_get_signer_id(cert_def, cert, cert_size, &comp_cert[67]);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	comp_cert[69] = ((cert_def->template_id & 0x0F) << 4) | (cert_def->chain_id & 0x0F);
	comp_cert[70] = ((uint8_t)(cert_def->sn_source & 0x0F) << 4) | 0;
	comp_cert[71] = 0;

	return ATCACERT_E_SUCCESS;
}

int atcacert_get_tbs( const atcacert_def_t* cert_def,
                      const uint8_t*        cert,
                      size_t cert_size,
                      const uint8_t**       tbs,
                      size_t*               tbs_size)
{
	if (cert_def == NULL || cert == NULL || tbs == NULL || tbs_size == NULL)
		return ATCACERT_E_BAD_PARAMS;

	if ((size_t)cert_def->tbs_cert_loc.offset + (size_t)cert_def->tbs_cert_loc.count > cert_size)
		return ATCACERT_E_BAD_CERT;

	*tbs      = &cert[cert_def->tbs_cert_loc.offset];
	*tbs_size = cert_def->tbs_cert_loc.count;

	return ATCACERT_E_SUCCESS;
}

int atcacert_get_tbs_digest( const atcacert_def_t* cert_def,
                             const uint8_t*        cert,
                             size_t cert_size,
                             uint8_t tbs_digest[32])
{
	int ret = ATCACERT_E_SUCCESS;
	const uint8_t* tbs = NULL;
	size_t tbs_size = 0;

	if (cert_def == NULL || cert == NULL || tbs_digest == NULL)
		return ATCACERT_E_BAD_PARAMS;

	ret = atcacert_get_tbs(cert_def, cert, cert_size, &tbs, &tbs_size);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcac_sw_sha2_256(tbs, tbs_size, tbs_digest);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	return ret;
}

int atcacert_set_cert_element( const atcacert_cert_loc_t* cert_loc,
                               uint8_t*                   cert,
                               size_t cert_size,
                               const uint8_t*             data,
                               size_t data_size)
{
	if (cert_loc == NULL || cert == NULL)
		return ATCACERT_E_BAD_PARAMS;

	if (cert_loc->count == 0)
		return ATCACERT_E_SUCCESS; // This element doesn't exist in the cert, but we treat this as a success

	if (data == NULL)
		return ATCACERT_E_BAD_PARAMS;

	if (data_size != cert_loc->count)
		return ATCACERT_E_UNEXPECTED_ELEM_SIZE;

	if ((size_t)cert_loc->offset + cert_loc->count > cert_size)
		return ATCACERT_E_ELEM_OUT_OF_BOUNDS;

	memcpy(&cert[cert_loc->offset], data, data_size);

	return ATCACERT_E_SUCCESS;
}

int atcacert_get_cert_element( const atcacert_cert_loc_t* cert_loc,
                               const uint8_t*             cert,
                               size_t cert_size,
                               uint8_t*                   data,
                               size_t data_size)
{
	if (cert_loc == NULL || cert == NULL || data == NULL)
		return ATCACERT_E_BAD_PARAMS;

	if (cert_loc->count == 0)
		return ATCACERT_E_ELEM_MISSING; // This element doesn't exist in the certificate

	if (cert_loc->count != data_size)
		return ATCACERT_E_UNEXPECTED_ELEM_SIZE;

	if ((size_t)cert_loc->offset + cert_loc->count > cert_size)
		return ATCACERT_E_ELEM_OUT_OF_BOUNDS;

	memcpy(data, &cert[cert_loc->offset], data_size);

	return ATCACERT_E_SUCCESS;
}

int atcacert_get_key_id(const uint8_t public_key[64], uint8_t key_id[20])
{
	uint8_t msg[65];

	if (public_key == NULL || key_id == NULL)
		return ATCACERT_E_BAD_PARAMS;

	msg[0] = 0x04;
	memcpy(&msg[1], public_key, 64);

	return atcac_sw_sha1(msg, sizeof(msg), key_id);
}

void atcacert_public_key_add_padding(const uint8_t raw_key[64], uint8_t padded_key[72])
{
	memmove(&padded_key[40], &raw_key[32], 32); // Move Y to padded position
	memset(&padded_key[36], 0, 4);              // Add Y padding bytes
	memmove(&padded_key[4], &raw_key[0], 32);   // Move X to padded position
	memset(&padded_key[0], 0, 4);               // Add X padding bytes
}

void atcacert_public_key_remove_padding(const uint8_t padded_key[72], uint8_t raw_key[64])
{
	memmove(&raw_key[0], &padded_key[4], 32);   // Move X
	memmove(&raw_key[32], &padded_key[40], 32); // Move Y
}