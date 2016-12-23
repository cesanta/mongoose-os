/**
 * \file
 * \brief Unity tests for the CryptoAuthLib software crypto API.
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

#include "atca_crypto_sw_tests.h"
#include "crypto/atca_crypto_sw_sha1.h"
#include "crypto/atca_crypto_sw_sha2.h"
#ifdef WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

static const uint8_t nist_hash_msg1[] = "abc";
static const uint8_t nist_hash_msg2[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
static const uint8_t nist_hash_msg3[] = "a";

void atca_crypto_sw_tests(void)
{
    UnityBegin("atca_crypto_sw_tests.c");
    
	RUN_TEST(test_atcac_sw_sha1_nist1);
	RUN_TEST(test_atcac_sw_sha1_nist2);
	RUN_TEST(test_atcac_sw_sha1_nist3);
	RUN_TEST(test_atcac_sw_sha1_nist_short);
	RUN_TEST(test_atcac_sw_sha1_nist_long);
	RUN_TEST(test_atcac_sw_sha1_nist_monte);


	RUN_TEST(test_atcac_sw_sha2_256_nist1);
	RUN_TEST(test_atcac_sw_sha2_256_nist2);
	RUN_TEST(test_atcac_sw_sha2_256_nist3);
	RUN_TEST(test_atcac_sw_sha2_256_nist_short);
	RUN_TEST(test_atcac_sw_sha2_256_nist_long);
	RUN_TEST(test_atcac_sw_sha2_256_nist_monte);
    
    UnityEnd();
}

void test_atcac_sw_sha1_nist1(void)
{
	const uint8_t digest_ref[] = {
		0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e, 0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c,
		0x9c, 0xd0, 0xd8, 0x9d
	};
	uint8_t digest[ATCA_SHA1_DIGEST_SIZE];
	int ret;

	TEST_ASSERT_EQUAL(ATCA_SHA1_DIGEST_SIZE, sizeof(digest_ref));

	ret = atcac_sw_sha1(nist_hash_msg1, sizeof(nist_hash_msg1) - 1, digest);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(digest_ref, digest, sizeof(digest_ref));
}

void test_atcac_sw_sha1_nist2(void)
{
	const uint8_t digest_ref[] = {
		0x84, 0x98, 0x3e, 0x44, 0x1c, 0x3b, 0xd2, 0x6e, 0xba, 0xae, 0x4a, 0xa1, 0xf9, 0x51, 0x29, 0xe5,
		0xe5, 0x46, 0x70, 0xf1
	};
	uint8_t digest[ATCA_SHA1_DIGEST_SIZE];
	int ret;

	TEST_ASSERT_EQUAL(ATCA_SHA1_DIGEST_SIZE, sizeof(digest_ref));

	ret = atcac_sw_sha1(nist_hash_msg2, sizeof(nist_hash_msg2) - 1, digest);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(digest_ref, digest, sizeof(digest_ref));
}

void test_atcac_sw_sha1_nist3(void)
{
	const uint8_t digest_ref[] = {
		0x34, 0xaa, 0x97, 0x3c, 0xd4, 0xc4, 0xda, 0xa4, 0xf6, 0x1e, 0xeb, 0x2b, 0xdb, 0xad, 0x27, 0x31,
		0x65, 0x34, 0x01, 0x6f
	};
	uint8_t digest[ATCA_SHA1_DIGEST_SIZE];
	int ret;
	atcac_sha1_ctx ctx;
	uint32_t i;

	TEST_ASSERT_EQUAL(ATCA_SHA1_DIGEST_SIZE, sizeof(digest_ref));

	ret = atcac_sw_sha1_init(&ctx);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	for (i = 0; i < 1000000; i++) {
		ret = atcac_sw_sha1_update(&ctx, nist_hash_msg3, 1);
		TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	}
	ret = atcac_sw_sha1_finish(&ctx, digest);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(digest_ref, digest, sizeof(digest_ref));
}

#ifdef WIN32
static void hex_to_uint8(const char hex_str[2], uint8_t* num)
{
	*num = 0;

	if (hex_str[0] >= '0' && hex_str[0] <= '9')
		*num += (hex_str[0] - '0') << 4;
	else if (hex_str[0] >= 'A' && hex_str[0] <= 'F')
		*num += (hex_str[0] - 'A' + 10) << 4;
	else if (hex_str[0] >= 'a' && hex_str[0] <= 'f')
		*num += (hex_str[0] - 'a' + 10) << 4;
	else
		TEST_FAIL_MESSAGE("Not a hex digit.");

	if (hex_str[1] >= '0' && hex_str[1] <= '9')
		*num += (hex_str[1] - '0');
	else if (hex_str[1] >= 'A' && hex_str[1] <= 'F')
		*num += (hex_str[1] - 'A' + 10);
	else if (hex_str[1] >= 'a' && hex_str[1] <= 'f')
		*num += (hex_str[1] - 'a' + 10);
	else
		TEST_FAIL_MESSAGE("Not a hex digit.");
}

static void hex_to_data(const char* hex_str, uint8_t* data, size_t data_size)
{
	size_t i = 0;

	TEST_ASSERT_EQUAL_MESSAGE(data_size * 2, strlen(hex_str) - 1, "Hex string unexpected length.");

	for (i = 0; i < data_size; i++)
		hex_to_uint8(&hex_str[i * 2], &data[i]);
}

static int read_rsp_hex_value(FILE* file, const char* name, uint8_t* data, size_t data_size)
{
	char line[16384];
	char* str = NULL;
	size_t name_size = strlen(name);

	do {
		str = fgets(line, sizeof(line), file);
		if (str == NULL)
			continue;

		if (memcmp(line, name, name_size) == 0)
			str = &line[name_size];
		else
			str = NULL;
	} while (str == NULL && !feof(file));
	if (str == NULL)
		return ATCA_GEN_FAIL;
	hex_to_data(str, data, data_size);

	return ATCA_SUCCESS;
}

static int read_rsp_int_value(FILE* file, const char* name, int* value)
{
	char line[2048];
	char* str = NULL;
	size_t name_size = strlen(name);

	do {
		str = fgets(line, sizeof(line), file);
		if (str == NULL)
			continue;

		if (memcmp(line, name, name_size) == 0)
			str = &line[name_size];
		else
			str = NULL;
	} while (str == NULL && !feof(file));
	if (str == NULL)
		return ATCA_GEN_FAIL;
	*value = atoi(str);

	return ATCA_SUCCESS;
}

#endif

static void test_atcac_sw_sha1_nist_simple(const char* filename)
{
#ifndef WIN32
	TEST_IGNORE_MESSAGE("Test only available under windows.");
#else
	FILE* rsp_file = NULL;
	int ret = ATCA_SUCCESS;
	uint8_t md_ref[ATCA_SHA1_DIGEST_SIZE];
	uint8_t md[sizeof(md_ref)];
	int len_bits = 0;
	uint8_t* msg = NULL;
	size_t count = 0;

	rsp_file = fopen(filename, "r");
	TEST_ASSERT_NOT_NULL_MESSAGE(rsp_file, "Failed to  open file");

	do {
		ret = read_rsp_int_value(rsp_file, "Len = ", &len_bits);
		if (ret != ATCA_SUCCESS)
			continue;

		msg = malloc(len_bits == 0 ? 1 : len_bits / 8);
		TEST_ASSERT_NOT_NULL_MESSAGE(msg, "malloc failed");

		ret = read_rsp_hex_value(rsp_file, "Msg = ", msg, len_bits == 0 ? 1 : len_bits / 8);
		TEST_ASSERT_EQUAL(ret, ATCA_SUCCESS);

		ret = read_rsp_hex_value(rsp_file, "MD = ", md_ref, sizeof(md_ref));
		TEST_ASSERT_EQUAL(ret, ATCA_SUCCESS);

		ret = atcac_sw_sha1(msg, len_bits / 8, md);
		TEST_ASSERT_EQUAL(ret, ATCA_SUCCESS);
		TEST_ASSERT_EQUAL_MEMORY(md_ref, md, sizeof(md_ref));

		free(msg);
		msg = NULL;
		count++;
	} while (ret == ATCA_SUCCESS);
	TEST_ASSERT_MESSAGE(count > 0, "No long tests found in file.");
#endif
}

void test_atcac_sw_sha1_nist_short(void)
{
	test_atcac_sw_sha1_nist_simple("cryptoauthlib/test/sha-byte-test-vectors/SHA1ShortMsg.rsp");
}

void test_atcac_sw_sha1_nist_long(void)
{
	test_atcac_sw_sha1_nist_simple("cryptoauthlib/test/sha-byte-test-vectors/SHA1LongMsg.rsp");
}

void test_atcac_sw_sha1_nist_monte(void)
{
#ifndef WIN32
	TEST_IGNORE_MESSAGE("Test only available under windows.");
#else
	FILE* rsp_file = NULL;
	int ret = ATCA_SUCCESS;
	uint8_t seed[ATCA_SHA1_DIGEST_SIZE];
	uint8_t md[4][sizeof(seed)];
	int i, j;
	uint8_t m[sizeof(seed) * 3];
	uint8_t md_ref[sizeof(seed)];

	rsp_file = fopen("cryptoauthlib/test/sha-byte-test-vectors/SHA1Monte.rsp", "r");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, rsp_file, "Failed to open sha-byte-test-vectors/SHA1Monte.rsp");

	// Find the seed value
	ret = read_rsp_hex_value(rsp_file, "Seed = ", seed, sizeof(seed));
	TEST_ASSERT_EQUAL_MESSAGE(ATCA_SUCCESS, ret, "Failed to find Seed value in file.");

	for (j = 0; j < 100; j++) {
		memcpy(&md[0], seed, sizeof(seed));
		memcpy(&md[1], seed, sizeof(seed));
		memcpy(&md[2], seed, sizeof(seed));
		for (i = 0; i < 1000; i++) {
			memcpy(m, md, sizeof(m));
			ret = atcac_sw_sha1(m, sizeof(m), &md[3][0]);
			TEST_ASSERT_EQUAL_MESSAGE(ATCA_SUCCESS, ret, "atcac_sw_sha1 failed");
			memmove(&md[0], &md[1], sizeof(seed) * 3);
		}
		ret = read_rsp_hex_value(rsp_file, "MD = ", md_ref, sizeof(md_ref));
		TEST_ASSERT_EQUAL_MESSAGE(ATCA_SUCCESS, ret, "Failed to find MD value in file.");
		TEST_ASSERT_EQUAL_MEMORY(md_ref, &md[2], sizeof(md_ref));
		memcpy(seed, &md[2], sizeof(seed));
	}
#endif
}



void test_atcac_sw_sha2_256_nist1(void)
{
	const uint8_t digest_ref[] = {
		0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA, 0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
		0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C, 0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD
	};
	uint8_t digest[ATCA_SHA2_256_DIGEST_SIZE];
	int ret;

	TEST_ASSERT_EQUAL(ATCA_SHA2_256_DIGEST_SIZE, sizeof(digest_ref));

	ret = atcac_sw_sha2_256(nist_hash_msg1, sizeof(nist_hash_msg1) - 1, digest);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(digest_ref, digest, sizeof(digest_ref));
}

void test_atcac_sw_sha2_256_nist2(void)
{
	const uint8_t digest_ref[] = {
		0x24, 0x8D, 0x6A, 0x61, 0xD2, 0x06, 0x38, 0xB8, 0xE5, 0xC0, 0x26, 0x93, 0x0C, 0x3E, 0x60, 0x39,
		0xA3, 0x3C, 0xE4, 0x59, 0x64, 0xFF, 0x21, 0x67, 0xF6, 0xEC, 0xED, 0xD4, 0x19, 0xDB, 0x06, 0xC1
	};
	uint8_t digest[ATCA_SHA2_256_DIGEST_SIZE];
	int ret;

	TEST_ASSERT_EQUAL(ATCA_SHA2_256_DIGEST_SIZE, sizeof(digest_ref));

	ret = atcac_sw_sha2_256(nist_hash_msg2, sizeof(nist_hash_msg2) - 1, digest);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(digest_ref, digest, sizeof(digest_ref));
}

void test_atcac_sw_sha2_256_nist3(void)
{
	const uint8_t digest_ref[] = {
		0xCD, 0xC7, 0x6E, 0x5C, 0x99, 0x14, 0xFB, 0x92, 0x81, 0xA1, 0xC7, 0xE2, 0x84, 0xD7, 0x3E, 0x67,
		0xF1, 0x80, 0x9A, 0x48, 0xA4, 0x97, 0x20, 0x0E, 0x04, 0x6D, 0x39, 0xCC, 0xC7, 0x11, 0x2C, 0xD0
	};
	uint8_t digest[ATCA_SHA2_256_DIGEST_SIZE];
	int ret;
	atcac_sha2_256_ctx ctx;
	uint32_t i;

	TEST_ASSERT_EQUAL(ATCA_SHA2_256_DIGEST_SIZE, sizeof(digest_ref));

	ret = atcac_sw_sha2_256_init(&ctx);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	for (i = 0; i < 1000000; i++) {
		ret = atcac_sw_sha2_256_update(&ctx, nist_hash_msg3, 1);
		TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	}
	ret = atcac_sw_sha2_256_finish(&ctx, digest);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(digest_ref, digest, sizeof(digest_ref));
}

static void test_atcac_sw_sha2_256_nist_simple(const char* filename)
{
#ifndef WIN32
	TEST_IGNORE_MESSAGE("Test only available under windows.");
#else
	FILE* rsp_file = NULL;
	int ret = ATCA_SUCCESS;
	uint8_t md_ref[ATCA_SHA2_256_DIGEST_SIZE];
	uint8_t md[sizeof(md_ref)];
	int len_bits = 0;
	uint8_t* msg = NULL;
	size_t count = 0;

	rsp_file = fopen(filename, "r");
	TEST_ASSERT_NOT_NULL_MESSAGE(rsp_file, "Failed to  open file");

	do {
		ret = read_rsp_int_value(rsp_file, "Len = ", &len_bits);
		if (ret != ATCA_SUCCESS)
			continue;

		msg = malloc(len_bits == 0 ? 1 : len_bits / 8);
		TEST_ASSERT_NOT_NULL_MESSAGE(msg, "malloc failed");

		ret = read_rsp_hex_value(rsp_file, "Msg = ", msg, len_bits == 0 ? 1 : len_bits / 8);
		TEST_ASSERT_EQUAL(ret, ATCA_SUCCESS);

		ret = read_rsp_hex_value(rsp_file, "MD = ", md_ref, sizeof(md_ref));
		TEST_ASSERT_EQUAL(ret, ATCA_SUCCESS);

		ret = atcac_sw_sha2_256(msg, len_bits / 8, md);
		TEST_ASSERT_EQUAL(ret, ATCA_SUCCESS);
		TEST_ASSERT_EQUAL_MEMORY(md_ref, md, sizeof(md_ref));

		free(msg);
		msg = NULL;
		count++;
	} while (ret == ATCA_SUCCESS);
	TEST_ASSERT_MESSAGE(count > 0, "No long tests found in file.");
#endif
}

void test_atcac_sw_sha2_256_nist_short(void)
{
	test_atcac_sw_sha2_256_nist_simple("cryptoauthlib/test/sha-byte-test-vectors/SHA256ShortMsg.rsp");
}

void test_atcac_sw_sha2_256_nist_long(void)
{
	test_atcac_sw_sha2_256_nist_simple("cryptoauthlib/test/sha-byte-test-vectors/SHA256LongMsg.rsp");
}

void test_atcac_sw_sha2_256_nist_monte(void)
{
#ifndef WIN32
	TEST_IGNORE_MESSAGE("Test only available under windows.");
#else
	FILE* rsp_file = NULL;
	int ret = ATCA_SUCCESS;
	uint8_t seed[ATCA_SHA2_256_DIGEST_SIZE];
	uint8_t md[4][sizeof(seed)];
	int i, j;
	uint8_t m[sizeof(seed) * 3];
	uint8_t md_ref[sizeof(seed)];

	rsp_file = fopen("cryptoauthlib/test/sha-byte-test-vectors/SHA256Monte.rsp", "r");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, rsp_file, "Failed to  open sha-byte-test-vectors/SHA256Monte.rsp");

	// Find the seed value
	ret = read_rsp_hex_value(rsp_file, "Seed = ", seed, sizeof(seed));
	TEST_ASSERT_EQUAL_MESSAGE(ATCA_SUCCESS, ret, "Failed to find Seed value in file.");

	for (j = 0; j < 100; j++) {
		memcpy(&md[0], seed, sizeof(seed));
		memcpy(&md[1], seed, sizeof(seed));
		memcpy(&md[2], seed, sizeof(seed));
		for (i = 0; i < 1000; i++) {
			memcpy(m, md, sizeof(m));
			ret = atcac_sw_sha2_256(m, sizeof(m), &md[3][0]);
			TEST_ASSERT_EQUAL_MESSAGE(ATCA_SUCCESS, ret, "atcac_sw_sha1 failed");
			memmove(&md[0], &md[1], sizeof(seed) * 3);
		}
		ret = read_rsp_hex_value(rsp_file, "MD = ", md_ref, sizeof(md_ref));
		TEST_ASSERT_EQUAL_MESSAGE(ATCA_SUCCESS, ret, "Failed to find MD value in file.");
		TEST_ASSERT_EQUAL_MEMORY(md_ref, &md[2], sizeof(md_ref));
		memcpy(seed, &md[2], sizeof(seed));
	}
#endif
}