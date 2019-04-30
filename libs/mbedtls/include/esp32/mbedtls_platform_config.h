/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 */

#pragma once

#define MBEDTLS_AES_ALT
#define MBEDTLS_MPI_MUL_MPI_ALT
#define MBEDTLS_MPI_EXP_MOD_ALT

#define MBEDTLS_CIPHER_MODE_XTS
#define ESP32_MBEDTLS_DYN_BUF_CANARY

/* no_extern_c_check */
