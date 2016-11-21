/**
 * \file
 * \brief cert DER length tests
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



#include "atcacert/atcacert_der.h"
#include "test/unity.h"
#include "test/unity_fixture.h"

TEST_GROUP(atcacert_der_enc_length);

TEST_SETUP(atcacert_der_enc_length)
{
}

TEST_TEAR_DOWN(atcacert_der_enc_length)
{
}

TEST(atcacert_der_enc_length, short_form)
{
	uint32_t length;
	uint8_t der_length[8];
	size_t der_length_size = sizeof(der_length);
	int ret = 0;

	// Smallest shot form
	length = 0x00;
	der_length_size = sizeof(der_length);
	ret = atcacert_der_enc_length(length, der_length, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(1, der_length_size);
	TEST_ASSERT_EQUAL(length, der_length[0]);

	// Largest short form
	length = 0x7F;
	der_length_size = sizeof(der_length);
	ret = atcacert_der_enc_length(length, der_length, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(1, der_length_size);
	TEST_ASSERT_EQUAL(length, der_length[0]);

	// Size only
	length = 127;
	der_length_size = 0;
	ret = atcacert_der_enc_length(length, NULL, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(1, der_length_size);
}

TEST(atcacert_der_enc_length, long_form_2byte)
{
	uint32_t length;
	uint8_t der_len_min[] = { 0x81, 0x80 }; // 0x80
	uint8_t der_len_max[] = { 0x81, 0xFF }; // 0xFF
	uint8_t der_length[8];
	size_t der_length_size = sizeof(der_length);
	int ret = 0;

	// Smallest 2-byte long form
	length = 0x80;
	der_length_size = sizeof(der_length);
	ret = atcacert_der_enc_length(length, der_length, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_len_min), der_length_size);
	TEST_ASSERT_EQUAL_MEMORY(der_len_min, der_length, sizeof(der_len_min));

	// Largest 2-byte long form
	length = 0xFF;
	der_length_size = sizeof(der_length);
	ret = atcacert_der_enc_length(length, der_length, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_len_max), der_length_size);
	TEST_ASSERT_EQUAL_MEMORY(der_len_max, der_length, sizeof(der_len_max));

	// Size only
	length = 0xFF;
	der_length_size = 0;
	ret = atcacert_der_enc_length(length, NULL, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_len_max), der_length_size);
}

TEST(atcacert_der_enc_length, long_form_3byte)
{
	uint32_t length;
	uint8_t der_len_min[] = { 0x82, 0x01, 0x00 };   // 0x0100
	uint8_t der_len_max[] = { 0x82, 0xFF, 0xFF };   // 0xFFFF
	uint8_t der_length[8];
	size_t der_length_size = sizeof(der_length);
	int ret = 0;

	// Smallest 3-byte long form
	length = 0x0100;
	der_length_size = sizeof(der_length);
	ret = atcacert_der_enc_length(length, der_length, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_len_min), der_length_size);
	TEST_ASSERT_EQUAL_MEMORY(der_len_min, der_length, sizeof(der_len_min));

	// Largest 3-byte long form
	length = 0xFFFF;
	der_length_size = sizeof(der_length);
	ret = atcacert_der_enc_length(length, der_length, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_len_max), der_length_size);
	TEST_ASSERT_EQUAL_MEMORY(der_len_max, der_length, sizeof(der_len_max));

	// Size only
	length = 0xFFFF;
	der_length_size = 0;
	ret = atcacert_der_enc_length(length, NULL, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_len_max), der_length_size);
}

TEST(atcacert_der_enc_length, long_form_4byte)
{
	uint32_t length;
	uint8_t der_len_min[] = { 0x83, 0x01, 0x00, 0x00 }; // 0x010000
	uint8_t der_len_max[] = { 0x83, 0xFF, 0xFF, 0xFF }; // 0xFFFFFF
	uint8_t der_length[8];
	size_t der_length_size = sizeof(der_length);
	int ret = 0;

	// Smallest 4-byte long form
	length = 0x010000;
	der_length_size = sizeof(der_length);
	ret = atcacert_der_enc_length(length, der_length, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_len_min), der_length_size);
	TEST_ASSERT_EQUAL_MEMORY(der_len_min, der_length, sizeof(der_len_min));

	// Largest 4-byte long form
	length = 0xFFFFFF;
	der_length_size = sizeof(der_length);
	ret = atcacert_der_enc_length(length, der_length, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_len_max), der_length_size);
	TEST_ASSERT_EQUAL_MEMORY(der_len_max, der_length, sizeof(der_len_max));

	// Size only
	length = 0xFFFFFF;
	der_length_size = 0;
	ret = atcacert_der_enc_length(length, NULL, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_len_max), der_length_size);
}

TEST(atcacert_der_enc_length, long_form_5byte)
{
	uint32_t length;
	uint8_t der_len_min[] = { 0x84, 0x01, 0x00, 0x00, 0x00 };   // 0x01000000
	uint8_t der_len_max[] = { 0x84, 0xFF, 0xFF, 0xFF, 0xFF };   // 0xFFFFFFFF
	uint8_t der_length[8];
	size_t der_length_size = sizeof(der_length);
	int ret = 0;

	// Smallest 5-byte long form
	length = 0x01000000;
	der_length_size = sizeof(der_length);
	ret = atcacert_der_enc_length(length, der_length, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_len_min), der_length_size);
	TEST_ASSERT_EQUAL_MEMORY(der_len_min, der_length, sizeof(der_len_min));

	// Largest 5-byte long form
	length = 0xFFFFFFFF;
	der_length_size = sizeof(der_length);
	ret = atcacert_der_enc_length(length, der_length, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_len_max), der_length_size);
	TEST_ASSERT_EQUAL_MEMORY(der_len_max, der_length, sizeof(der_len_max));

	// Size only
	length = 0xFFFFFFFF;
	der_length_size = 0;
	ret = atcacert_der_enc_length(length, NULL, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_len_max), der_length_size);
}

TEST(atcacert_der_enc_length, small_buf)
{
	uint32_t length;
	uint8_t der_length[3];
	size_t der_length_size = sizeof(der_length);
	int ret = 0;

	length = 0x01000000;
	der_length_size = sizeof(der_length);
	ret = atcacert_der_enc_length(length, der_length, &der_length_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BUFFER_TOO_SMALL, ret);
	TEST_ASSERT_EQUAL(5, der_length_size);
}

TEST(atcacert_der_enc_length, bad_params)
{
	uint32_t length;
	uint8_t der_length[8];
	int ret = 0;

	length = 0x01000000;
	ret = atcacert_der_enc_length(length, der_length, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}

TEST_GROUP(atcacert_der_dec_length);

TEST_SETUP(atcacert_der_dec_length)
{
}

TEST_TEAR_DOWN(atcacert_der_dec_length)
{
}

struct atcacert_der_dec_length__good_s {
	uint32_t length;
	size_t der_length_size;
	uint8_t der_length[5];
};

TEST(atcacert_der_dec_length, good)
{
	int ret = 0;
	size_t i;
	const struct atcacert_der_dec_length__good_s tests[] = {
		{ 0x00000000, 1, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
		{ 0x0000003F, 1, { 0x3F, 0x00, 0x00, 0x00, 0x00 } },
		{ 0x0000007F, 1, { 0x7F, 0x00, 0x00, 0x00, 0x00 } },
		{ 0x00000080, 2, { 0x81, 0x80, 0x00, 0x00, 0x00 } },
		{ 0x00000085, 2, { 0x81, 0x85, 0x00, 0x00, 0x00 } },
		{ 0x000000FF, 2, { 0x81, 0xFF, 0x00, 0x00, 0x00 } },
		{ 0x00000100, 3, { 0x82, 0x01, 0x00, 0x00, 0x00 } },
		{ 0x000055AA, 3, { 0x82, 0x55, 0xAA, 0x00, 0x00 } },
		{ 0x0000FFFF, 3, { 0x82, 0xFF, 0xFF, 0x00, 0x00 } },
		{ 0x00010000, 4, { 0x83, 0x01, 0x00, 0x00, 0x00 } },
		{ 0x0055AA55, 4, { 0x83, 0x55, 0xAA, 0x55, 0x00 } },
		{ 0x00FFFFFF, 4, { 0x83, 0xFF, 0xFF, 0xFF, 0x00 } },
		{ 0x01000000, 5, { 0x84, 0x01, 0x00, 0x00, 0x00 } },
		{ 0x55AA55AA, 5, { 0x84, 0x55, 0xAA, 0x55, 0xAA } },
		{ 0xFFFFFFFF, 5, { 0x84, 0xFF, 0xFF, 0xFF, 0xFF } },
	};

	for (i = 0; i < sizeof(tests) / sizeof(struct atcacert_der_dec_length__good_s); i++) {
		size_t der_length_size = sizeof(tests[i].der_length);
		uint32_t length = 0;

		ret = atcacert_der_dec_length(tests[i].der_length, &der_length_size, &length);
		TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
		TEST_ASSERT_EQUAL(tests[i].der_length_size, der_length_size);
		TEST_ASSERT_EQUAL_UINT32(tests[i].length, length);

		// Size only
		der_length_size = sizeof(tests[i].der_length);
		ret = atcacert_der_dec_length(tests[i].der_length, &der_length_size, NULL);
		TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
		TEST_ASSERT_EQUAL(tests[i].der_length_size, der_length_size);
	}
}

TEST(atcacert_der_dec_length, zero_size)
{
	int ret = 0;
	const uint8_t der_length[] = { 0x00 }; // Just needed for a valid pointer
	size_t der_length_size = 0;
	uint32_t length = 0;

	ret = atcacert_der_dec_length(der_length, &der_length_size, &length);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);
}

TEST(atcacert_der_dec_length, not_enough_data)
{
	int ret = 0;
	const uint8_t der_length[] = { 0x82, 0x01 }; // Encoding indicates more data than is supplied
	size_t der_length_size = sizeof(der_length);
	uint32_t length = 0;

	ret = atcacert_der_dec_length(der_length, &der_length_size, &length);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);
}

TEST(atcacert_der_dec_length, indefinite_form)
{
	int ret = 0;
	const uint8_t der_length[] = { 0x80, 0x01 }; // Indefinite form not supported
	size_t der_length_size = sizeof(der_length);
	uint32_t length = 0;

	ret = atcacert_der_dec_length(der_length, &der_length_size, &length);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);
}

TEST(atcacert_der_dec_length, too_large)
{
	int ret = 0;
	const uint8_t der_length[] = { 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 }; // Too large for the return type (64 bit size_t)
	size_t der_length_size = sizeof(der_length);
	uint32_t length = 0;

	ret = atcacert_der_dec_length(der_length, &der_length_size, &length);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);
}

TEST(atcacert_der_dec_length, bad_params)
{
	int ret = 0;
	const uint8_t der_length[] = { 0x82, 0x01, 0x0 };
	size_t der_length_size = sizeof(der_length);
	uint32_t length = 0;

	der_length_size = sizeof(der_length);
	ret = atcacert_der_dec_length(NULL, &der_length_size, &length);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	der_length_size = sizeof(der_length);
	ret = atcacert_der_dec_length(der_length, NULL, &length);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_dec_length(NULL, NULL, &length);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}