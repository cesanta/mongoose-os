/**
 * \file
 * \brief Declarations common to all atcacert code.
 *
 * These are common definitions used by all the atcacert code.
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

#ifndef ATCACERT_H
#define ATCACERT_H

#include <stddef.h>
#include <stdint.h>

/** \defgroup atcacert_ Certificate manipulation methods (atcacert_)
 *
 * \brief
 * These methods provide convenient ways to perform certification I/O with
 * CryptoAuth chips and perform certificate manipulation in memory
 *
   @{ */
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (1)
#endif

#define ATCACERT_E_SUCCESS              0   //!< Operation completed successfully.
#define ATCACERT_E_ERROR                1   //!< General error.
#define ATCACERT_E_BAD_PARAMS           2   //!< Invalid/bad parameter passed to function.
#define ATCACERT_E_BUFFER_TOO_SMALL     3   //!< Supplied buffer for output is too small to hold the result.
#define ATCACERT_E_DECODING_ERROR       4   //!< Data being decoded/parsed has an invalid format.
#define ATCACERT_E_INVALID_DATE         5   //!< Date is invalid.
#define ATCACERT_E_UNIMPLEMENTED        6   //!< Function is unimplemented for the current configuration.
#define ATCACERT_E_UNEXPECTED_ELEM_SIZE 7   //!< A certificate element size was not what was expected.
#define ATCACERT_E_ELEM_MISSING         8   //!< The certificate element isn't defined for the certificate definition.
#define ATCACERT_E_ELEM_OUT_OF_BOUNDS   9   //!< Certificate element is out of bounds for the given certificate.
#define ATCACERT_E_BAD_CERT             10  //!< Certificate structure is bad in some way.
#define ATCACERT_E_WRONG_CERT_DEF       11
#define ATCACERT_E_VERIFY_FAILED        12  //!< Certificate or challenge/response verification failed.

/** @} */
#endif