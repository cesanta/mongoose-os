/*
 * atcatls_tests.c
 *
 */ 

#include "cryptoauthlib.h"
#include "atcatls.h"
#include "basic/atca_basic.h"
#include "atcatls_tests.h"
#include "atcatls_cfg.h"

/*---------------------------------------------------------------------------------------------
GenKey Command Sent:
07 40 00 00 00 00 05
GenKeyCmd Received:
43 
3C 30 7F 3A 1B 05 96 19 21 EB 35 50 09 1D 1D 48 5C 68 D4 A4 40 21 05 90 21 F6 A7 F2 A4 7F 2B 8E 
DD 28 1B 0A A8 F4 5A F4 AC DC 85 D9 9A D0 34 6B 60 B1 7F E6 D8 43 26 D9 39 48 C6 34 CF 45 DE 81 
70 0B
Nonce Command Sent:
27 16 03 00 00 
8C 61 64 CE FD 38 06 05 F7 29 44 E3 B6 5B 9A 33 34 94 63 2D 2E 16 FD 9E 77 98 F6 F2 67 32 A1 76 
0B 11
NonceCommand Received:
04 00 03 40
Sign Command Sent:
07 41 80 00 00 28 05
SignCmd Received:
43 
CC 58 BC B6 7D 8D 82 28 6B F4 9A 22 88 71 2B 57 99 73 51 56 9E E6 98 0C 06 CD 70 EB 82 B5 4D 58 
D1 06 F0 BE DF BC 9E 00 3E 56 53 C6 33 6D FA 9E B5 3E C1 7E 37 E6 66 E8 68 CF B7 7E 49 1E BA BB 
51 20
Nonce Command Sent:
27 16 03 00 00 
8C 61 64 CE FD 38 06 05 F7 29 44 E3 B6 5B 9A 33 34 94 63 2D 2E 16 FD 9E 77 98 F6 F2 67 32 A1 76 
0B 11
NonceCommand Received:
04 00 03 40
Verify Command Sent:
87 45 02 04 00 
CC 58 BC B6 7D 8D 82 28 6B F4 9A 22 88 71 2B 57 99 73 51 56 9E E6 98 0C 06 CD 70 EB 82 B5 4D 58 
D1 06 F0 BE DF BC 9E 00 3E 56 53 C6 33 6D FA 9E B5 3E C1 7E 37 E6 66 E8 68 CF B7 7E 49 1E BA BB 
3C 30 7F 3A 1B 05 96 19 21 EB 35 50 09 1D 1D 48 5C 68 D4 A4 40 21 05 90 21 F6 A7 F2 A4 7F 2B 8E 
DD 28 1B 0A A8 F4 5A F4 AC DC 85 D9 9A D0 34 6B 60 B1 7F E6 D8 43 26 D9 39 48 C6 34 CF 45 DE 81 
0E FF
VerifyCmd Received:
04 00 03 40
-----------------------------------------------------------------------------------------------
*/

// Test vectors
uint8_t serverPubKey[] =
{
	// X coordinate of the elliptic curve.
	0xF4, 0x57, 0x47, 0x0E, 0x47, 0x8A, 0x0C, 0xC1, 0xC2, 0x0F, 0x06, 0x58, 0x36, 0x82, 0x55, 0xBD, 
	0xAE, 0x39, 0x44, 0x24, 0x1E, 0xFF, 0xB2, 0x2F, 0x8D, 0xD3, 0xA9, 0x8B, 0x05, 0x8D, 0xBA, 0xF1, 

	// Y coordinate of the elliptic curve.
	0xFA, 0x8E, 0x4A, 0xD1, 0x87, 0xDB, 0x01, 0xB9, 0xB8, 0xF4, 0xB2, 0x1F, 0x2F, 0x18, 0x03, 0xEB, 
	0x64, 0xD8, 0x98, 0xE2, 0xCB, 0x3B, 0xC4, 0x97, 0x84, 0xEB, 0x64, 0xB4, 0x90, 0xE3, 0x78, 0x12
};

uint8_t pubKey1[] =
{
	// X coordinate of the elliptic curve.
	0x3C, 0x30, 0x7F, 0x3A, 0x1B, 0x05, 0x96, 0x19, 0x21, 0xEB, 0x35, 0x50, 0x09, 0x1D, 0x1D, 0x48, 
	0x5C, 0x68, 0xD4, 0xA4, 0x40, 0x21, 0x05, 0x90, 0x21, 0xF6, 0xA7, 0xF2, 0xA4, 0x7F, 0x2B, 0x8E, 

	// Y coordinate of the elliptic curve.
	0xDD, 0x28, 0x1B, 0x0A, 0xA8, 0xF4, 0x5A, 0xF4, 0xAC, 0xDC, 0x85, 0xD9, 0x9A, 0xD0, 0x34, 0x6B, 
	0x60, 0xB1, 0x7F, 0xE6, 0xD8, 0x43, 0x26, 0xD9, 0x39, 0x48, 0xC6, 0x34, 0xCF, 0x45, 0xDE, 0x81
};

uint8_t msg1[] =
{
	0x8C, 0x61, 0x64, 0xCE, 0xFD, 0x38, 0x06, 0x05, 0xF7, 0x29, 0x44, 0xE3, 0xB6, 0x5B, 0x9A, 0x33, 
	0x34, 0x94, 0x63, 0x2D, 0x2E, 0x16, 0xFD, 0x9E, 0x77, 0x98, 0xF6, 0xF2, 0x67, 0x32, 0xA1, 0x76
};

uint8_t sig1[] =
{
	// R coordinate of the signature.
	0xCC, 0x58, 0xBC, 0xB6, 0x7D, 0x8D, 0x82, 0x28, 0x6B, 0xF4, 0x9A, 0x22, 0x88, 0x71, 0x2B, 0x57, 
	0x99, 0x73, 0x51, 0x56, 0x9E, 0xE6, 0x98, 0x0C, 0x06, 0xCD, 0x70, 0xEB, 0x82, 0xB5, 0x4D, 0x58, 

	// S coordinate of the signature.
	0xD1, 0x06, 0xF0, 0xBE, 0xDF, 0xBC, 0x9E, 0x00, 0x3E, 0x56, 0x53, 0xC6, 0x33, 0x6D, 0xFA, 0x9E, 
	0xB5, 0x3E, 0xC1, 0x7E, 0x37, 0xE6, 0x66, 0xE8, 0x68, 0xCF, 0xB7, 0x7E, 0x49, 0x1E, 0xBA, 0xBB
};

void atcatls_test_runner(void)
{
	UnityBegin("atcatls_tests.c");

	// Call get SN first
	RUN_TEST(test_atcatls_get_sn);

	// Configure the device
	RUN_TEST(test_atcatls_config_device);

	// Call random after configuration
	RUN_TEST(test_atcatls_random);

	RUN_TEST(test_atcatls_init_finish);
	RUN_TEST(test_atcatls_init_enc_key);

	RUN_TEST(test_atcatls_create_key);
	RUN_TEST(test_atcatls_sign);
	RUN_TEST(test_atcatls_verify);
	RUN_TEST(test_atcatls_ecdh);
	RUN_TEST(test_atcatls_ecdhe);

	RUN_TEST(test_atcatls_get_cert);

	RUN_TEST(test_atcatls_enc_write_read);

	UnityEnd();
}

void test_atcatls_config_device(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	status = atcatls_init();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatls_config_device();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatls_finish();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_atcatls_init_finish(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	// The atcatls_init() function will call atcatls_com_init(), test for success
	status = atcatls_init();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// The atcatls_finish() function will call atcatls_com_release(), test for success
	status = atcatls_finish();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_atcatls_create_key(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t pubkey[ATCA_PUB_KEY_SIZE] = { 0 };
	uint8_t pubkeyNull[ATCA_PUB_KEY_SIZE] = { 0 };
	int cmpResult = 0;

	status = atcatls_init();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatls_create_key(AUTH_PRIV_SLOT, pubkey);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Compare pubkey memory, it should have changed.  If all bytes are equal memcmp will return 0.
	cmpResult = memcmp(pubkey, pubkeyNull, ATCA_PUB_KEY_SIZE);
	TEST_ASSERT_NOT_EQUAL(cmpResult, 0);

	status = atcatls_finish();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_atcatls_sign(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t signature[ATCA_SIG_SIZE] = { 0 };
	uint8_t sigNull[ATCA_SIG_SIZE] = { 0 };
	int cmpResult = 0;

	status = atcatls_init();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatls_sign(AUTH_PRIV_SLOT, msg1, signature);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Compare signature memory, it should have changed.  If all bytes are equal memcmp will return 0.
	cmpResult = memcmp(signature, sigNull, ATCA_SIG_SIZE);
	TEST_ASSERT_NOT_EQUAL(cmpResult, 0);

	status = atcatls_finish();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_atcatls_verify(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	bool verified = false;

	status = atcatls_init();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatls_verify(msg1, sig1, pubKey1, &verified);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// See if the buffers actually verified
	TEST_ASSERT_TRUE(verified);

	status = atcatls_finish();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_atcatls_ecdh(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t pmk[ATCA_KEY_SIZE] = { 0 };
	uint8_t pmkNull[ATCA_KEY_SIZE] = { 0 };
	int cmpResult = 0;

	status = atcatls_init();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatls_ecdh(ECDH_PRIV_SLOT, pubKey1, pmk);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Compare pmk memory, it should have changed.  If all bytes are equal memcmp will return 0.
	cmpResult = memcmp(pmk, pmkNull, ATCA_KEY_SIZE);
	TEST_ASSERT_NOT_EQUAL(cmpResult, 0);

	status = atcatls_finish();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_atcatls_ecdhe(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t pmk[ATCA_KEY_SIZE] = { 0 };
	uint8_t pmkNull[ATCA_KEY_SIZE] = { 0 };
	uint8_t pubKeyRet[ATCA_PUB_KEY_SIZE] = { 0 };
	uint8_t pubKeyNull[ATCA_PUB_KEY_SIZE] = { 0 };
	int cmpResult = 0;

	status = atcatls_init();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatls_ecdhe(ECDH_PRIV_SLOT, pubKey1, pubKeyRet, pmk);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Compare pmk memory, it should have changed.  If all bytes are equal memcmp will return 0.
	cmpResult = memcmp(pmk, pmkNull, ATCA_KEY_SIZE);
	TEST_ASSERT_NOT_EQUAL(cmpResult, 0);

	// Compare pmk memory, it should have changed.  If all bytes are equal memcmp will return 0.
	cmpResult = memcmp(pubKeyRet, pubKeyNull, ATCA_PUB_KEY_SIZE);
	TEST_ASSERT_NOT_EQUAL(cmpResult, 0);

	status = atcatls_finish();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_atcatls_random(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t randomNum[RANDOM_RSP_SIZE] = { 0 };
	uint8_t randomNumNull[RANDOM_RSP_SIZE] = { 0 };
	int cmpResult = 0;

	status = atcatls_init();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcab_random(randomNum);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Compare randomNum memory, it should have changed.  If all bytes are equal memcmp will return 0.
	cmpResult = memcmp(randomNum, randomNumNull, RANDOM_RSP_SIZE);
	TEST_ASSERT_NOT_EQUAL(cmpResult, 0);

	status = atcatls_finish();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

// Testing the write of the encryption key
ATCA_STATUS get_enc_key(uint8_t* enckey, int16_t keysize);
// global variable for key
uint8_t _enckeytest[ATCA_KEY_SIZE] = { 0 };

ATCA_STATUS get_enc_key(uint8_t* enckey, int16_t keysize)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	if (enckey == NULL) return status;
	memcpy(enckey, _enckeytest, sizeof(_enckeytest));
	return ATCA_SUCCESS;
}

void test_atcatls_init_enc_key(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t enckey_ret[ATCA_KEY_SIZE] = { 0 };
	bool lock = false;

	status = atcatls_init();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatlsfn_set_get_enckey(&get_enc_key);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatls_init_enckey(_enckeytest, lock);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatls_get_enckey(enckey_ret);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
	// todo: Test that these are equal  (_enckeytest == enckey_ret)

	status = atcatls_finish();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_atcatls_enc_write_read(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t writeBytes[] = { 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
							 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19};
	int16_t writeSize = sizeof(writeBytes);
	uint8_t readBytes[ATCA_BLOCK_SIZE] = { 0 };
	int16_t readSize = sizeof(readBytes);

	status = atcatls_init();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatls_enc_write(ENC_STORE416_SLOT, writeBytes, writeSize);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatls_enc_read(ENC_STORE416_SLOT, readBytes, &readSize);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Check the equivalence of the buffers
	TEST_ASSERT_EQUAL(readSize, writeSize);
	TEST_ASSERT_EQUAL_MEMORY(readBytes, writeBytes, readSize);

	status = atcatls_finish();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_atcatls_get_cert(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t *certout = NULL;
	int16_t certsize = 0;

	status = atcatls_init();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatls_get_cert(certout, &certsize);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatls_finish();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}

void test_atcatls_get_sn(void)
{
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t snOut[ATCA_SERIAL_NUM_SIZE] = { 0 };
	uint8_t snOutNull[ATCA_SERIAL_NUM_SIZE] = { 0 };
	int cmpResult = 0;

	status = atcatls_init();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	status = atcatls_get_sn(snOut);
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);

	// Compare snOut memory, it should have changed.  If all bytes are equal memcmp will return 0.
	cmpResult = memcmp(snOut, snOutNull, ATCA_SERIAL_NUM_SIZE);
	TEST_ASSERT_NOT_EQUAL(cmpResult, 0);

	status = atcatls_finish();
	TEST_ASSERT_EQUAL(ATCA_SUCCESS, status);
}





