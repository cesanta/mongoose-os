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

#include "cryptoauthlib.h"
#include "atca_helpers.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef ATCAPRINTF



/** \brief convert a binary buffer to a hex string suitable for human reading
 *  \param[in] binary input buffer to convert
 *  \param[in] binLen length of buffer to convert
 *  \param[out] asciihex buffer that receives hex string
 *  \param[out] hexlen the length of the asciihex buffer
 * \return ATCA_STATUS
 */
ATCA_STATUS atcab_bin2hex(const uint8_t* binary, int binLen, char* asciihex, int* asciihexlen)
{
	return atcab_bin2hex_(binary, binLen, asciihex, asciihexlen, true);
}

/** \brief convert a binary buffer to a hex string suitable for human reading
 *  \param[in] inbuff input buffer to convert
 *  \param[in] inbuffLen length of buffer to convert
 *  \param[out] asciihex buffer that receives hex string
 *  \param[inout] asciihexlen the length of the asciihex buffer
 *  \param[inout] addspace indicates whether spaces and returns should be added for pretty printing
 * \return ATCA_STATUS
 */
ATCA_STATUS atcab_bin2hex_(const uint8_t* binary, int binLen, char* asciihex, int* asciihexlen, bool addspace)
{
	int i;
	int hexlen = 0;

	// Verify the inputs
	if ((binary == NULL) || (asciihex == NULL) || (asciihexlen == NULL))
		return ATCA_BAD_PARAM;

	// Initialize the return bytes to all 0s
	memset(asciihex, 0, *asciihexlen);

	// Convert one byte at a time
	for (i = 0; i < binLen; i++) {
		if (hexlen > *asciihexlen) break;
		if ((i % 16 == 0 && i != 0) && addspace) {
			sprintf(&asciihex[hexlen], "\r\n");
			hexlen += 2;
		}
		if (addspace) {
			sprintf(&asciihex[hexlen], "%02X ", *binary++);
			hexlen += 3;
		}else {
			sprintf(&asciihex[hexlen], "%02X", *binary++);
			hexlen += 2;
		}
	}
	*asciihexlen = (int)strlen(asciihex);

	return ATCA_SUCCESS;
}

/** \brief convert a binary buffer to a hex string suitable for human reading
 *  \param[in] inbuff input buffer to convert
 *  \param[in] inbuffLen length of buffer to convert
 *  \param[out] outbuff buffer that receives hex string
 *  \return string length of the output buffer
 */
ATCA_STATUS atcab_hex2bin(const char* asciiHex, int asciiHexLen, uint8_t* binary, int* binLen)
{
	int i = 0;
	int j = 0;
	uint32_t byt;
	char* packedHex = NULL;
	int packedLen = asciiHexLen;
	char hexByte[3];

	// Verify the inputs
	if ((binary == NULL) || (asciiHex == NULL) || (binLen == NULL))
		return ATCA_BAD_PARAM;

	// Pack the bytes (remove white space & make even number of characters)
	packedHex = (char*)malloc(packedLen);
	memset(packedHex, 0, packedLen);
	packHex(asciiHex, asciiHexLen, packedHex, &packedLen);

	// Initialize the binary buffer to all 0s
	memset(binary, 0, *binLen);
	memset(hexByte, 0, 3);

	// Convert the ascii bytes to binary
	for (i = 0, j = 0; i < packedLen; i += 2, j++) {
		if (i > packedLen || j > *binLen) break;
		// Copy two characters to be scanned
		memcpy(hexByte, &packedHex[i], 2);
		sscanf(hexByte, "%x", (unsigned int*)&byt);
		// take the msb of the uint32_t
		binary[j] = byt;
	}
	*binLen = j;
	free(packedHex);
	return ATCA_SUCCESS;
}

//#else


#endif

/**
 * \brief Checks to see if a character is an ASCII representation of a digit ((c ge '0') and (c le '9'))
 * \param[in] c  character to check
 * \return True if the character is a digit
 */
bool isDigit(char c)
{
	return (c >= '0') && (c <= '9');
}

/**
 * \brief Checks to see if a character is whitespace ((c == '\n') || (c == '\r') || (c == '\t') || (c == ' '))
 * \param[in] c  character to check
 * \return True if the character is whitespace
 */
bool isWhiteSpace(char c)
{
	return (c == '\n') || (c == '\r') || (c == '\t') || (c == ' ');
}

/**
 * \brief Checks to see if a character is an ASCII representation of hex ((c >= 'A') and (c <= 'F')) || ((c >= 'a') and (c <= 'f'))
 * \param[in] c  character to check
 * \return True if the character is a hex
 */
bool isAlpha(char c)
{
	return ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
}

/**
 * \brief Checks to see if a character is an ASCII representation of hex ((c >= 'A') and (c <= 'F')) || ((c >= 'a') and (c <= 'f'))
 * \param[in] c  character to check
 * \return True if the character is a hex
 */
bool isHexAlpha(char c)
{
	return ((c >= 'A') && (c <= 'F')) || ((c >= 'a') && (c <= 'f'));
}

/**
* \brief Returns true if this character is a valid hex character or if this is whitespace (The character can be
*        included in a valid hexstring).
* \param[in] c  character to check
* \return True if the character can be included in a valid hexstring
*/
bool isHex(char c)
{
	return isHexDigit(c) || isWhiteSpace(c);
}

/**
* \brief Returns true if this character is a valid hex character.
* \param[in] c  character to check
* \return True if the character can be included in a valid hexstring
*/
bool isHexDigit(char c)
{
	return isDigit(c) || isHexAlpha(c);
}

/**
* \brief Remove white space from a ASCII hex string.
* \param[in] asciiHex		Initial hex string to remove white space from
* \param[in] asciiHexLen	Length of the initial hex string
* \param[in] packedHex		Resulting hex string without white space
* \param[inout] packedLen	In: Size to packedHex buffer
*							Out: Number of bytes in the packed hex string
* \return ATCA_SUCCESS if asciiHex was packed without errors
*/
ATCA_STATUS packHex(const char* asciiHex, int asciiHexLen, char* packedHex, int* packedLen)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	int i = 0;
	int j = 0;

	do
	{
		// Verify the inputs
		if ((asciiHex == NULL) || (packedHex == NULL) || (packedLen == NULL))
		{
			status = ATCA_BAD_PARAM;
			BREAK(status, "Null input parameter");
		}
		// Loop through each character and only add the hex characters
		for (i = 0; i < asciiHexLen; i++) {
			if (isHexDigit(asciiHex[i])) {
				if (j > *packedLen) break;
				packedHex[j++] = asciiHex[i];
			}
		}
		*packedLen = j;
	} while (false);
	// TODO: If there are not an even number of characters, then pad with a '0'
	(void) status;
	return ATCA_SUCCESS;
}

/** \brief Print each hex character in the binary buffer with spaces between bytes
*  \param[in] label label to print
*  \param[in] binary input buffer to print
*  \param[in] binLen length of buffer to print
* \return ATCA_STATUS
*/
ATCA_STATUS atcab_printbin_label(const uint8_t* label, uint8_t* binary, int binLen)
{
	printf((const char*)label);
	return atcab_printbin(binary, binLen, true);
}

/** \brief Print each hex character in the binary buffer with spaces between bytes
*  \param[in] binary input buffer to print
*  \param[in] binLen length of buffer to print
* \return ATCA_STATUS
*/
ATCA_STATUS atcab_printbin_sp(uint8_t* binary, int binLen)
{
	return atcab_printbin(binary, binLen, true);
}

/** \brief Print each hex character in the binary buffer
*  \param[in] binary input buffer to print
*  \param[in] binLen length of buffer to print
*  \param[in] addspace indicates whether spaces and returns should be added for pretty printing
* \return ATCA_STATUS
*/
ATCA_STATUS atcab_printbin(uint8_t* binary, int binLen, bool addspace)
{
	int i = 0;
	int lineLen = 16;

	// Verify the inputs
	if ((binary == NULL))
		return ATCA_BAD_PARAM;

	// Set the line length
	lineLen = addspace ? 16 : 32;

	// Print the bytes
	for (i = 0; i < binLen; i++)
	{
		// Print the byte
		if (addspace)		printf("%02X ", binary[i]);
		else				printf("%02X", binary[i]);

		// Break at the lineLen
		if ((i + 1) % lineLen == 0) printf("\r\n");
	}
	// Print the last carriage return
	printf("\r\n");

	return ATCA_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// Base 64 Encode/Decode

//#define CODES		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="
#define IS_EQUAL	(char)64
#define IS_INVALID	(char)0xFF

/**
* \brief Returns true if this character is a valid base 64 character or if this is whitespace (A character can be
*        included in a valid base 64 string).
* \param[in] c  character to check
* \return True if the character can be included in a valid base 64 string
*/
bool isBase64(char c)
{
	return isBase64Digit(c) || isWhiteSpace(c);
}

/**
* \brief Returns true if this character is a valid base 64 character.
* \param[in] c  character to check
* \return True if the character can be included in a valid base 64 string
*/
bool isBase64Digit(char c)
{
	return isDigit(c) || isAlpha(c) || c == '+' || c == '/' || c == '=';
}

/**
* \brief Remove white space from a base 64 string.
* \param[in] asciiBase64	Initial base 64 string to remove white space from
* \param[in] asciiBase64Len	Length of the initial base 64 string
* \param[in] packedBase64	Resulting base 64 string without white space
* \param[inout] packedLen	In: Size to packedHex buffer
*							Out: Number of bytes in the packed base 64 string
* \return ATCA_SUCCESS if asciiBase64 was packed without errors
*/
ATCA_STATUS packBase64(const char* asciiBase64, int asciiBase64Len, char* packedBase64, int* packedLen)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	int i = 0;
	int j = 0;

	do
	{
		// Verify the inputs
		if ((asciiBase64 == NULL) || (packedBase64 == NULL) || (packedLen == NULL))
		{
			status = ATCA_BAD_PARAM;
			BREAK(status, "Null input parameter");
		}
		// Loop through each character and only add the base 64 characters
		for (i = 0; i < asciiBase64Len; i++)
		{
			if (isBase64Digit(asciiBase64[i]))
			{
				if (j > *packedLen) break;
				packedBase64[j++] = asciiBase64[i];
			}
		}
		*packedLen = j;
	} while (false);

	return status;
}

/**
* \brief Returns the base 64 index of the given character.
* \param[in] c  character to check
* \return the base 64 index of the given character
*/
char base64Index(char c)
{
	if ((c >= 'A') && (c <= 'Z'))	return (char)(c - 'A');
	if ((c >= 'a') && (c <= 'z'))	return (char)(26 + c - 'a');
	if ((c >= '0') && (c <= '9'))	return (char)(52 + c - '0');
	if (c == '+')					return (char)62;
	if (c == '/')					return (char)63;

	if (c == '=')					return IS_EQUAL;
	return IS_INVALID;
}

/**
* \brief Returns the base 64 character of the given index.
* \param[in] id  index to check
* \return the base 64 character of the given index
*/
char base64Char(char id)
{
	if (id < 26)		return (char)('A' + id);
	if ((id >= 26) && (id < 52))	return (char)('a' + id - 26);
	if ((id >= 52) && (id < 62))	return (char)('0' + id - 52);
	if (id == 62)					return (char)'+';
	if (id == 63)					return (char)'/';

	if (id == IS_EQUAL)				return (char)'=';
	return IS_INVALID;
}

/**
* \brief Decode base 64 encoded characters into a byte array
* \param[in]    encoded		The input base 64 encoded characters that will be decoded.
* \param[in]    encodedLen	The length of the encoded characters
* \param[out]   byteArray	The output buffer that contains the decoded byte array
* \param[inout] arrayLen	Input: The size of the decoded buffer
*							Output: The length of the decoded byte array
* \return ATCA_STATUS
*/
ATCA_STATUS atcab_base64decode(const char* encoded, size_t encodedLen, uint8_t* byteArray, size_t* arrayLen)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	int id[4];
	int i = 0;
	int j = 0;
	char* packedBase64 = NULL;
	int packedLen = encodedLen;

	// Set the output length.
	size_t outLen = (encodedLen * 3) / 4;

	// Allocate the memory for the packed base 64 buffer
	packedBase64 = (char*)malloc(packedLen);
	memset(packedBase64, 0, packedLen);

	do
	{
		// Check the input parameters
		if (encoded == NULL || byteArray == NULL || arrayLen == NULL)
		{
			status = ATCA_BAD_PARAM;
			BREAK(status, "Null input parameter");
		}
		// Pack the encoded characters (remove the white space from the encoded characters)
		packBase64(encoded, encodedLen, packedBase64, &packedLen);
		// Packed length must be divisible by 4
		if (packedLen % 4 != 0)
		{
			status = ATCA_BAD_PARAM;
			BREAK(status, "Invalid base64 input");
		}
		if (*arrayLen < outLen)
		{
			status = ATCA_BAD_PARAM;
			BREAK(status, "Length of decoded buffer too small");
		}
		// Initialize the return length to 0
		*arrayLen = 0;

		// Take the encoded bytes in groups of 4 and decode them into 3 bytes
		for (i = 0; i < packedLen; i += 4)
		{
			id[0] = base64Index(packedBase64[i]);
			id[1] = base64Index(packedBase64[i + 1]);
			id[2] = base64Index(packedBase64[i + 2]);
			id[3] = base64Index(packedBase64[i + 3]);
			byteArray[j++] = (uint8_t)((id[0] << 2) | (id[1] >> 4));
			if (id[2] < 64)
			{
				byteArray[j++] = (uint8_t)((id[1] << 4) | (id[2] >> 2));
				if (id[3] < 64)
				{
					byteArray[j++] = (uint8_t)((id[2] << 6) | id[3]);
				}
			}
		}
		*arrayLen = j;
	} while (false);

	// Deallocate the packed buffer
	free(packedBase64);
	return status;
}

/**
* \brief Encode a byte array into base 64 encoded characters with \n character every 64 bytes
* \param[in]    byteArray	The input byte array that will be converted to base 64 encoded characters.
* \param[in]    arrayLen	The length of the byte array
* \param[out]   encoded		The output converted to base 64 encoded characters.  The \n will be added every 64 characters.
* \param[inout] encodedLen	Input: The size of the encoded buffer
*							Output: The length of the encoded base 64 character string including the additional \n characters
* \return ATCA_STATUS
*/
ATCA_STATUS atcab_base64encode(const uint8_t* byteArray, size_t arrayLen, char* encoded, size_t* encodedLen)
{
	return atcab_base64encode_(byteArray, arrayLen, encoded, encodedLen, true);
}

/**
* \brief Encode a byte array into base 64 encoded characters. Optionally with \n character every 64 bytes
* \param[in]    byteArray	The input byte array that will be converted to base 64 encoded characters.
* \param[in]    arrayLen	The length of the byte array
* \param[out]   encoded		The output converted to base 64 encoded characters.  The \n will optionally be added every 64 characters.
* \param[inout] encodedLen	Input: The size of the encoded buffer
*							Output: The length of the encoded base 64 character string including the additional \n characters
* \param[in]    addNewLine	Optionally add a \n every 64 characters
* \return ATCA_STATUS
*/
ATCA_STATUS atcab_base64encode_(const uint8_t* byteArray, size_t arrayLen, char* encoded, size_t* encodedLen, bool addNewLine)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	size_t i = 0;
	size_t j = 0;
	size_t offset = 0;
	int id = 0;

	// Set the output length.  Add the \n every 64 characters
	size_t r3 = (arrayLen % 3);
	size_t b64Len = ((arrayLen * 4) / 3) + r3;
	size_t outLen = b64Len + (b64Len / 64);

	do
	{
		// Check the input parameters
		if (encoded == NULL || byteArray == NULL || encodedLen == NULL)
		{
			status = ATCA_BAD_PARAM;
			BREAK(status, "Null input parameter");
		}
		if (*encodedLen < outLen)
		{
			status = ATCA_BAD_PARAM;
			BREAK(status, "Length of encoded buffer too small");
		}
		// Initialize the return length to 0
		*encodedLen = 0;

		// Loop through the byte array by 3 then map to 4 base 64 encoded characters
		for (i = 0; i < arrayLen; i += 3)
		{
			id = (byteArray[i] & 0xFC) >> 2;
			encoded[j++] = base64Char(id);
			id = (byteArray[i] & 0x03) << 4;
			if (i + 1 < arrayLen)
			{
				id |= (byteArray[i + 1] & 0xF0) >> 4;
				encoded[j++] = base64Char(id);
				id = (byteArray[i + 1] & 0x0F) << 2;
				if (i + 2 < arrayLen)
				{
					id |= (byteArray[i + 2] & 0xC0) >> 6;
					encoded[j++] = base64Char(id);
					id = byteArray[i + 2] & 0x3F;
					encoded[j++] = base64Char(id);
				}
				else
				{
					encoded[j++] = base64Char(id);
					encoded[j++] = base64Char(IS_EQUAL);
				}
			}
			else
			{
				encoded[j++] = base64Char(id);
				encoded[j++] = base64Char(IS_EQUAL);
				encoded[j++] = base64Char(IS_EQUAL);
			}
			// Add \n every 64 bytes if specified
			if (addNewLine && ((j-offset) % 64 == 0))
			{
				// as soon as we do this, we introduce an offset
				encoded[j++] = '\n';
				offset++;
			}
		}
		// Set the final encoded length
		*encodedLen = j;
	} while (false);
	return status;
}



