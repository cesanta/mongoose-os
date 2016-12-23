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


#include <stdlib.h>
#include "atcacert_client.h"
#include "cryptoauthlib.h"
#include "basic/atca_basic.h"


int atcacert_get_response( uint8_t device_private_key_slot,
						   const uint8_t challenge[32],
						   uint8_t response[64])
{
	if (device_private_key_slot > 15 || challenge == NULL || response == NULL)
		return ATCACERT_E_BAD_PARAMS;

	return atcab_sign(device_private_key_slot, challenge, response);
}

int atcacert_read_cert(const atcacert_def_t* cert_def,
	const uint8_t ca_public_key[64],
	uint8_t*              cert,
	size_t*               cert_size)
{
	int ret = 0;
	atcacert_device_loc_t device_locs[16];
	size_t device_locs_count = 0;
	size_t i = 0;
	atcacert_build_state_t build_state;

	if (cert_def == NULL || cert == NULL || cert_size == NULL)
		return ATCACERT_E_BAD_PARAMS;

	ret = atcacert_get_device_locs(
		cert_def,
		device_locs,
		&device_locs_count,
		sizeof(device_locs) / sizeof(device_locs[0]),
		32);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	ret = atcacert_cert_build_start(&build_state, cert_def, cert, cert_size, ca_public_key);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	for (i = 0; i < device_locs_count; i++) {
		uint8_t data[416];
		if (device_locs[i].zone == DEVZONE_DATA && device_locs[i].is_genkey) {
			ret = atcab_get_pubkey(device_locs[i].slot, data);
			if (ret != ATCA_SUCCESS)
				return ret;
		}
		else {
			size_t start_block = device_locs[i].offset / 32;
			uint8_t block;
			size_t end_block = (device_locs[i].offset + device_locs[i].count) / 32;
			for (block = (uint8_t)start_block; block < end_block; block++) {
				ret = atcab_read_zone(device_locs[i].zone, device_locs[i].slot, block, 0, &data[block * 32 - device_locs[i].offset], 32);
				if (ret != ATCA_SUCCESS)
					return ret;
			}
		}

		ret = atcacert_cert_build_process(&build_state, &device_locs[i], data);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
	}

	ret = atcacert_cert_build_finish(&build_state);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;

	return ATCACERT_E_SUCCESS;
}

int atcacert_write_cert(const atcacert_def_t* cert_def,
						const uint8_t*        cert,
						size_t                cert_size)
{
	int ret = 0;
	atcacert_device_loc_t device_locs[16];
	size_t device_locs_count = 0;
	size_t i = 0;
	
	if (cert_def == NULL || cert == NULL)
		return ATCACERT_E_BAD_PARAMS;
	
	ret = atcacert_get_device_locs(cert_def, device_locs, &device_locs_count, sizeof(device_locs) / sizeof(device_locs[0]), 32);
	if (ret != ATCACERT_E_SUCCESS)
		return ret;
	
	for (i = 0; i < device_locs_count; i++)
	{
		size_t end_block;
		size_t start_block;
		uint8_t data[416];
		uint8_t block;
		
		if (device_locs[i].zone == DEVZONE_CONFIG)
			continue; // Cert data isn't written to the config zone, only read
		if (device_locs[i].zone == DEVZONE_DATA && device_locs[i].is_genkey)
			continue; // Public key is generated not written
		
		ret = atcacert_get_device_data(cert_def, cert, cert_size, &device_locs[i], data);
		if (ret != ATCACERT_E_SUCCESS)
			return ret;
		
		start_block = device_locs[i].offset / 32;
		end_block = (device_locs[i].offset + device_locs[i].count) / 32;
		for (block = (uint8_t)start_block; block < end_block; block++)
		{
			ret = atcab_write_zone(device_locs[i].zone, device_locs[i].slot, block, 0, &data[(block - start_block) * 32], 32);
			if (ret != ATCA_SUCCESS)
				return ret;
		}
	}
	
	return ATCACERT_E_SUCCESS;
}

int atcacert_decode_pem_cert(const char* pem_cert, size_t pem_cert_size, uint8_t* cert_bytes, size_t* cert_bytes_size)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	const char certHeader[] = PEM_CERT_BEGIN;
	const char certFooter[] = PEM_CERT_END;
	size_t certHeaderSize = sizeof(certHeader);
	size_t certFooterSize = sizeof(certFooter);
	size_t max_size = (pem_cert_size * 3/4) - sizeof(certHeader) - sizeof(certFooter);
	char* certPtr = NULL;
	size_t cert_begin = 0;
	size_t cert_end = 0;
	size_t cert_size = 0;

	do
	{
		// Check the pointers
		if (pem_cert == NULL || cert_bytes == NULL || cert_bytes_size == NULL)
		{
			status = ATCACERT_E_BAD_PARAMS;
			BREAK(status, "Null input parameter");
		}
		// Check the buffer size
		if (*cert_bytes_size < max_size)
		{
			status = ATCACERT_E_BAD_PARAMS;
			BREAK(status, "buffer size too small");
		}
		// Strip the certificate begin & end tags
		// Find the start byte location
		certPtr = strstr(pem_cert, certHeader);
		cert_begin = certPtr == NULL ? 0 : (certPtr - pem_cert) + certHeaderSize;

		// Find the end byte location
		certPtr = strstr(pem_cert, certFooter);
		cert_end = certPtr == NULL ? pem_cert_size : (size_t)(certPtr - pem_cert);

		// Decode the base 64 bytes
		cert_size = cert_end - cert_begin;
		atcab_base64decode(&pem_cert[cert_begin], cert_size, cert_bytes, cert_bytes_size);

	} while (false);

	return status;
}

int atcacert_encode_pem_cert(const uint8_t* cert_bytes, size_t cert_bytes_size, char* pem_cert, size_t* pem_cert_size)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	const char certHeader[] = PEM_CERT_BEGIN_EOL;
	const char certFooter[] = PEM_CERT_END_EOL;
	size_t certHeaderSize = sizeof(certHeader);
	size_t certFooterSize = sizeof(certFooter);
	size_t pem_max_size = (cert_bytes_size * 4/3) + certHeaderSize + certFooterSize;
	size_t cpyLoc = 0;
	size_t encodedLen = 0;

	do
	{
		// Check the pointers
		if (pem_cert == NULL || cert_bytes == NULL || pem_cert_size == NULL)
		{
			status = ATCACERT_E_BAD_PARAMS;
			BREAK(status, "Null input parameter");
		}
		// Check the buffer size
		if (*pem_cert_size < pem_max_size)
		{
			status = ATCACERT_E_BAD_PARAMS;
			BREAK(status, "buffer size too small");
		}
		//// Allocate the buffer to hold the PEM encoded cert
		//csrEncoded = (char*)malloc(encodedLen);
		//memset(csrEncoded, 0, encodedLen);

		// Clear the pem buffer
		memset(pem_cert, 0x00, *pem_cert_size);

		// Add the certificate begin tag
		memcpy(pem_cert, certHeader, certHeaderSize);
		cpyLoc += certHeaderSize - 1; // Subtract the null terminator

		// Base 64 encode the bytes
		encodedLen = pem_max_size - cpyLoc;
		status = atcab_base64encode(cert_bytes, cert_bytes_size, &pem_cert[cpyLoc], &encodedLen);
		if (status != ATCA_SUCCESS) BREAK(status, "Base 64 encoding failed");
		cpyLoc += encodedLen;

		// Copy the certificate end tag
		if ((cpyLoc + certFooterSize) > *pem_cert_size)
		{
			status = ATCACERT_E_BAD_PARAMS;
			BREAK(status, "buffer too small");
		}
		memcpy(&pem_cert[cpyLoc], certFooter, certFooterSize);
		cpyLoc += certFooterSize - 1; // Subtract the null terminator
		*pem_cert_size = cpyLoc;

	} while (false);

	return status;
}

int atcacert_create_csr_pem(const atcacert_def_t* csr_def, char* csr, size_t* csr_size)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	size_t csrSize = 0;
	uint8_t* csrbytes = (uint8_t*)csr;
	char* csrEncoded = NULL;
	size_t encodedLen = 0;
	const char csrHeader[] = PEM_CSR_BEGIN_EOL;
	const char csrFooter[] = PEM_CSR_END_EOL;
	size_t cpyLoc = 0;

	do
	{
		// Check the pointers
		if (csr_def == NULL || csr == NULL || csr == NULL || csr_size == NULL)
		{
			status = ATCACERT_E_BAD_PARAMS;
			BREAK(status, "Null input parameter");
		}

		// Call the create csr function to get the csr bytes
		csrSize = *csr_size;
		status = atcacert_create_csr(csr_def, csrbytes, &csrSize);
		if (status != ATCA_SUCCESS) BREAK(status, "Failed to create CSR");

		// Allocate the buffer to hold the fully wrapped CSR
		encodedLen = *csr_size;
		csrEncoded = malloc(encodedLen);
		memset(csrEncoded, 0, encodedLen);

		// Wrap the CSR in the header/footer
		if ((cpyLoc + sizeof(csrHeader)) > *csr_size)
		{
			status = ATCACERT_E_BAD_PARAMS;
			BREAK(status, "CSR buffer too small");
		}
		// Copy the header into the PEM CSR
		memcpy(&csrEncoded[cpyLoc], csrHeader, sizeof(csrHeader));
		cpyLoc += sizeof(csrHeader) - 1; // Subtract the null terminator

		// Base 64 encode the bytes
		encodedLen -= cpyLoc;
		status = atcab_base64encode(csrbytes, csrSize, &csrEncoded[cpyLoc], &encodedLen);
		if (status != ATCA_SUCCESS) BREAK(status, "Base 64 encoding failed");
		cpyLoc += encodedLen;

		// Copy the footer into the PEM CSR
		if ((cpyLoc + sizeof(csrFooter)) > *csr_size)
		{
			status = ATCACERT_E_BAD_PARAMS;
			BREAK(status, "CSR buffer too small");
		}
		memcpy(&csrEncoded[cpyLoc], csrFooter, sizeof(csrFooter));
		cpyLoc += sizeof(csrFooter) - 1; // Subtract the null terminator

		// Copy the wrapped CSR
		memcpy(csr, csrEncoded, cpyLoc);
		*csr_size = cpyLoc;

	} while (false);

	// Deallocate the buffer if needed
	if (csrEncoded != NULL) free(csrEncoded);

	return status;
}


int atcacert_create_csr(const atcacert_def_t* csr_def, uint8_t* csr, size_t* csr_size)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t pubkey[ATCA_PUB_KEY_SIZE] = { 0 };
	uint8_t sig[ATCA_SIG_SIZE] = { 0 };
	const atcacert_device_loc_t* pubDevLoc = NULL;
	const atcacert_cert_loc_t* pubLoc = NULL;
	uint8_t keySlot = 0;
	uint8_t privKeySlot = 0;
	uint8_t tbsDigest[ATCA_BLOCK_SIZE] = { 0 };
	size_t csr_max_size = 0;

	do
	{
		// Check the pointers
		if (csr_def == NULL || csr == NULL || csr == NULL || csr_size == NULL)
		{
			status = ATCACERT_E_BAD_PARAMS;
			BREAK(status, "Null input parameter");
		}
		// Check the csr buffer size
		if (*csr_size < csr_def->cert_template_size)
		{
			status = ATCACERT_E_BAD_PARAMS;
			BREAK(status, "CSR buffer size too small");
		}
		// Copy the CSR template into the CSR that will be returned
		memcpy(csr, csr_def->cert_template, csr_def->cert_template_size);
		csr_max_size = *csr_size;
		*csr_size = csr_def->cert_template_size;

		// Get a few elements from the csr_def structure
		pubLoc = &(csr_def->std_cert_elements[STDCERT_PUBLIC_KEY]);
		pubDevLoc = &(csr_def->public_key_dev_loc);
		keySlot = pubDevLoc->slot;
		privKeySlot = csr_def->private_key_slot;

		// Get the public key from the device
		if (pubDevLoc->is_genkey)
		{
			// Calculate the public key from the private key
			status = atcab_get_pubkey(keySlot, pubkey);
			if (status != ATCA_SUCCESS) BREAK(status, "Could not generate public key");
		}
		else
		{
			// Read the public key from a slot
			status = atcab_read_pubkey(keySlot, pubkey);
			if (status != ATCA_SUCCESS) BREAK(status, "Could not read public key");
		}
		// Insert the public key into the CSR template
		status = atcacert_set_cert_element(csr_def, pubLoc, csr, *csr_size, pubkey, ATCA_PUB_KEY_SIZE);
		if (status != ATCA_SUCCESS) BREAK(status, "Setting CSR public key failed");

		// Get the CSR TBS digest
		status = atcacert_get_tbs_digest(csr_def, csr, *csr_size, tbsDigest);
		if (status != ATCA_SUCCESS) BREAK(status, "Get TBS digest failed");

		// Sign the TBS digest
		status = atcab_sign(privKeySlot, tbsDigest, sig);
		if (status != ATCA_SUCCESS) BREAK(status, "Signing CSR failed");

		// Insert the signature into the CSR template
		status = atcacert_set_signature(csr_def, csr, csr_size, csr_max_size, sig);
		if (status != ATCA_SUCCESS) BREAK(status, "Setting CSR signature failed");

		// The exact size of the csr cannot be determined until after adding the signature
		// it is returned in the csr_size parameter.  (*csr_size = *csr_size;)

	} while (false);

	return status;
}


