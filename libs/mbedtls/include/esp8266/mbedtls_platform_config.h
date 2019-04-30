/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 */

#pragma once

/* Allow 1024 bit RSA on ESP8266 for performance reasons. */
#define MBEDTLS_RSA_MIN_BITS 1024
/* These are way too slow on ESP8266. */
#undef MBEDTLS_ECP_DP_SECP384R1_ENABLED
#undef MBEDTLS_ECP_DP_SECP521R1_ENABLED
/* We use AES in ESP8266 ROM. Note: Only AES128 is supported. */
#define MBEDTLS_AES_SETKEY_ENC_ALT
#define MBEDTLS_AES_SETKEY_DEC_ALT
#define MBEDTLS_AES_ENCRYPT_ALT
#define MBEDTLS_AES_DECRYPT_ALT

/* no_extern_c_check */
