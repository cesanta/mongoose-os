/**
 * \file
 * \brief Helpers to support the CryptoAuthLib Basic API methods
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

#ifndef ATCA_HELPERS_H_
#define ATCA_HELPERS_H_

#include "cryptoauthlib.h"

/** \defgroup atcab_ Basic Crypto API methods (atcab_)
 *
 * \brief
 * These methods provide the most convenient, simple API to CryptoAuth chips
 *
   @{ */

#ifdef __cplusplus
extern "C" {
#endif

ATCA_STATUS atcab_printbin(uint8_t* binary, int binLen, bool addspace);
ATCA_STATUS atcab_bin2hex(const uint8_t* binary, int binLen, char* asciiHex, int* asciiHexLen);
ATCA_STATUS atcab_bin2hex_(const uint8_t* binary, int binLen, char* asciiHex, int* asciiHexLen, bool addSpace);
ATCA_STATUS atcab_hex2bin(const char* asciiHex, int asciiHexLen, uint8_t* binary, int* binLen);
ATCA_STATUS atcab_printbin_sp(uint8_t* binary, int binLen);
ATCA_STATUS atcab_printbin_label(const uint8_t* label, uint8_t* binary, int binLen);


ATCA_STATUS packHex(const char* asciiHex, int asciiHexLen, char* packedHex, int* packedLen);
bool isDigit(char c);
bool isWhiteSpace(char c);
bool isAlpha(char c);
bool isHexAlpha(char c);
bool isHex(char c);
bool isHexDigit(char c);

ATCA_STATUS packBase64(const char* asciiBase64, int asciiBase64Len, char* packedBase64, int* packedLen);
bool isBase64(char c);
bool isBase64Digit(char c);
char base64Index(char c);
char base64Char(char id);
ATCA_STATUS atcab_base64decode(const char* encoded, size_t encodedLen, uint8_t* byteArray, size_t* arrayLen);
ATCA_STATUS atcab_base64encode(const uint8_t* byteArray, size_t arrayLen, char* encoded, size_t* encodedLen);
ATCA_STATUS atcab_base64encode_(const uint8_t* byteArray, size_t arrayLen, char* encoded, size_t* encodedLen, bool addNewLine);

#ifdef __cplusplus
}
#endif

/** @} */
#endif /* ATCA_HELPERS_H_ */