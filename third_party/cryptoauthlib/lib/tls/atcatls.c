/**
 * \file
 * \brief  Collection of functions for hardware abstraction of TLS implementations (e.g. OpenSSL)
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
#include <stdio.h>
#include "atcatls.h"
#include "atcatls_cfg.h"
#include "basic/atca_basic.h"
#include "atcacert/atcacert_client.h"
#include "atcacert/atcacert_host_hw.h"

// File scope defines
// The RSA key will be written to the upper blocks of slot 8
#define RSA_KEY_SLOT            8
#define RSA_KEY_START_BLOCK     5

// File scope global varibles
uint8_t _enckey[ATCA_KEY_SIZE] = { 0 };
atcatlsfn_get_enckey* _fn_get_enckey = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Constants for default tests operation

uint8_t g_CaCert[] =
{
	0x30, 0x82, 0x01, 0xBF, 0x30, 0x82, 0x01, 0x66, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x10, 0x54,
	0x15, 0xE1, 0x96, 0x9E, 0x76, 0xAF, 0xDB, 0x02, 0x87, 0x65, 0x3D, 0x4C, 0x79, 0x8C, 0xE3, 0x30,
	0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x38, 0x31, 0x1A, 0x30,
	0x18, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x11, 0x41, 0x74, 0x6D, 0x65, 0x6C, 0x20, 0x4F, 0x70,
	0x65, 0x6E, 0x53, 0x53, 0x4C, 0x20, 0x44, 0x65, 0x76, 0x31, 0x1A, 0x30, 0x18, 0x06, 0x03, 0x55,
	0x04, 0x03, 0x0C, 0x11, 0x41, 0x54, 0x45, 0x43, 0x43, 0x35, 0x30, 0x38, 0x41, 0x20, 0x52, 0x6F,
	0x6F, 0x74, 0x20, 0x43, 0x41, 0x30, 0x20, 0x17, 0x0D, 0x31, 0x35, 0x31, 0x30, 0x30, 0x37, 0x32,
	0x32, 0x30, 0x39, 0x34, 0x39, 0x5A, 0x18, 0x0F, 0x39, 0x39, 0x39, 0x39, 0x31, 0x32, 0x33, 0x31,
	0x32, 0x33, 0x35, 0x39, 0x35, 0x39, 0x5A, 0x30, 0x38, 0x31, 0x1A, 0x30, 0x18, 0x06, 0x03, 0x55,
	0x04, 0x0A, 0x0C, 0x11, 0x41, 0x74, 0x6D, 0x65, 0x6C, 0x20, 0x4F, 0x70, 0x65, 0x6E, 0x53, 0x53,
	0x4C, 0x20, 0x44, 0x65, 0x76, 0x31, 0x1A, 0x30, 0x18, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x11,
	0x41, 0x54, 0x45, 0x43, 0x43, 0x35, 0x30, 0x38, 0x41, 0x20, 0x52, 0x6F, 0x6F, 0x74, 0x20, 0x43,
	0x41, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08,
	0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0xBD, 0x14, 0x1C, 0x5D,
	0xB1, 0xAC, 0xCE, 0x0F, 0xCC, 0xF1, 0xC2, 0x25, 0x21, 0xEB, 0x80, 0xA4, 0x8B, 0xFB, 0x4D, 0xEB,
	0x69, 0xC7, 0x76, 0x58, 0xED, 0x55, 0x7B, 0x7E, 0xDC, 0x71, 0x5D, 0x57, 0x82, 0xCB, 0x82, 0x77,
	0x80, 0xEE, 0x13, 0xBF, 0x18, 0xAA, 0x87, 0x4F, 0xDA, 0x2A, 0x6A, 0xA5, 0x83, 0x4A, 0x09, 0x1B,
	0xA8, 0x6B, 0x0D, 0x36, 0xD1, 0x98, 0x05, 0x57, 0xE6, 0x8E, 0x89, 0xA0, 0xA3, 0x50, 0x30, 0x4E,
	0x30, 0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0x25, 0x28, 0x33, 0xA6, 0xDF,
	0x72, 0xFD, 0x06, 0x96, 0xD1, 0xA4, 0x56, 0x7E, 0x33, 0xF0, 0x17, 0x5C, 0x46, 0xBC, 0x6F, 0x30,
	0x1F, 0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x25, 0x28, 0x33, 0xA6,
	0xDF, 0x72, 0xFD, 0x06, 0x96, 0xD1, 0xA4, 0x56, 0x7E, 0x33, 0xF0, 0x17, 0x5C, 0x46, 0xBC, 0x6F,
	0x30, 0x0C, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xFF, 0x30, 0x0A,
	0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x03, 0x47, 0x00, 0x30, 0x44, 0x02,
	0x20, 0x57, 0x4B, 0x85, 0x01, 0xCD, 0x78, 0xE8, 0x04, 0xAE, 0xBC, 0x5F, 0x41, 0x87, 0x8E, 0xE0,
	0x14, 0x7B, 0xCF, 0x51, 0x7A, 0x5C, 0x4C, 0xBF, 0x45, 0x26, 0x94, 0x57, 0x58, 0x54, 0x45, 0x6F,
	0x7E, 0x02, 0x20, 0x5F, 0x6B, 0x7C, 0x28, 0x1E, 0x66, 0x95, 0x7D, 0x78, 0xB9, 0x3C, 0x83, 0xA2,
	0xAC, 0x74, 0x27, 0xFB, 0xFD, 0x82, 0xB6, 0x3B, 0x12, 0x7F, 0x76, 0xD7, 0xC3, 0x86, 0xA0, 0xDF,
	0x26, 0xAE, 0x1A
};

uint8_t g_SignerCert[] =
{
	0x30, 0x82, 0x01, 0xC2, 0x30, 0x82, 0x01, 0x69, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x03, 0x40,
	0x10, 0x01, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x38,
	0x31, 0x1A, 0x30, 0x18, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x11, 0x41, 0x74, 0x6D, 0x65, 0x6C,
	0x20, 0x4F, 0x70, 0x65, 0x6E, 0x53, 0x53, 0x4C, 0x20, 0x44, 0x65, 0x76, 0x31, 0x1A, 0x30, 0x18,
	0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x11, 0x41, 0x54, 0x45, 0x43, 0x43, 0x35, 0x30, 0x38, 0x41,
	0x20, 0x52, 0x6F, 0x6F, 0x74, 0x20, 0x43, 0x41, 0x30, 0x20, 0x17, 0x0D, 0x31, 0x35, 0x31, 0x30,
	0x30, 0x39, 0x32, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5A, 0x18, 0x0F, 0x39, 0x39, 0x39, 0x39, 0x31,
	0x32, 0x33, 0x31, 0x32, 0x33, 0x35, 0x39, 0x35, 0x39, 0x5A, 0x30, 0x48, 0x31, 0x1A, 0x30, 0x18,
	0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x11, 0x41, 0x74, 0x6D, 0x65, 0x6C, 0x20, 0x4F, 0x70, 0x65,
	0x6E, 0x53, 0x53, 0x4C, 0x20, 0x44, 0x65, 0x76, 0x31, 0x2A, 0x30, 0x28, 0x06, 0x03, 0x55, 0x04,
	0x03, 0x0C, 0x21, 0x4F, 0x70, 0x65, 0x6E, 0x53, 0x53, 0x4C, 0x20, 0x44, 0x65, 0x76, 0x20, 0x41,
	0x54, 0x45, 0x43, 0x43, 0x35, 0x30, 0x38, 0x41, 0x20, 0x53, 0x69, 0x67, 0x6E, 0x65, 0x72, 0x20,
	0x31, 0x30, 0x30, 0x31, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02,
	0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0x54,
	0x9B, 0xF7, 0x39, 0xDC, 0x88, 0xA9, 0xBA, 0xEF, 0xA3, 0x0E, 0x54, 0xEA, 0x0F, 0x59, 0xDE, 0x82,
	0x07, 0x98, 0x4F, 0x23, 0xB2, 0x44, 0x9B, 0x04, 0xB3, 0x7C, 0xBE, 0xA9, 0x7D, 0xF3, 0xC9, 0x05,
	0x21, 0x16, 0x68, 0xDC, 0x39, 0x1F, 0x79, 0xEE, 0x12, 0x8E, 0xB0, 0xEF, 0xE8, 0x89, 0xA7, 0xEE,
	0x02, 0x89, 0xC5, 0x49, 0x6D, 0x0D, 0x60, 0xCF, 0x09, 0x45, 0x7A, 0x6D, 0xCF, 0x16, 0x00, 0xA3,
	0x50, 0x30, 0x4E, 0x30, 0x0C, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01,
	0xFF, 0x30, 0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0xBB, 0x0F, 0x75, 0x5F,
	0x8F, 0xCC, 0xA3, 0x76, 0x16, 0x97, 0x3B, 0xE1, 0x01, 0xE4, 0x50, 0x98, 0x94, 0xB9, 0x8D, 0xFF,
	0x30, 0x1F, 0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x25, 0x28, 0x33,
	0xA6, 0xDF, 0x72, 0xFD, 0x06, 0x96, 0xD1, 0xA4, 0x56, 0x7E, 0x33, 0xF0, 0x17, 0x5C, 0x46, 0xBC,
	0x6F, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x03, 0x47, 0x00,
	0x30, 0x44, 0x02, 0x20, 0x5E, 0x5E, 0xD2, 0x8C, 0x71, 0x4D, 0xCA, 0x39, 0xE0, 0x93, 0xA3, 0xF6,
	0xDC, 0x3D, 0xA4, 0x87, 0x86, 0x40, 0xCA, 0x7E, 0xDF, 0x03, 0x89, 0xD6, 0x2C, 0x82, 0xC6, 0x2C,
	0x2F, 0xC5, 0x9C, 0x1D, 0x02, 0x20, 0x14, 0x5B, 0xE0, 0x9E, 0xD2, 0xA1, 0x55, 0x46, 0x98, 0xED,
	0x6A, 0xA8, 0xFD, 0x50, 0x55, 0xD2, 0x9B, 0x43, 0x51, 0x2D, 0x71, 0x81, 0xB5, 0xFA, 0xF4, 0xED,
	0x26, 0x17, 0x88, 0x19, 0x83, 0xFD
};

uint8_t g_DeviceCert[] =
{
	0x30, 0x82, 0x01, 0xA8, 0x30, 0x82, 0x01, 0x4E, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x0A, 0x40,
	0x01, 0x23, 0x80, 0x4C, 0xD9, 0x2C, 0xA5, 0x71, 0xEE, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48,
	0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x48, 0x31, 0x1A, 0x30, 0x18, 0x06, 0x03, 0x55, 0x04, 0x0A,
	0x0C, 0x11, 0x41, 0x74, 0x6D, 0x65, 0x6C, 0x20, 0x4F, 0x70, 0x65, 0x6E, 0x53, 0x53, 0x4C, 0x20,
	0x44, 0x65, 0x76, 0x31, 0x2A, 0x30, 0x28, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x21, 0x4F, 0x70,
	0x65, 0x6E, 0x53, 0x53, 0x4C, 0x20, 0x44, 0x65, 0x76, 0x20, 0x41, 0x54, 0x45, 0x43, 0x43, 0x35,
	0x30, 0x38, 0x41, 0x20, 0x53, 0x69, 0x67, 0x6E, 0x65, 0x72, 0x20, 0x31, 0x30, 0x30, 0x31, 0x30,
	0x20, 0x17, 0x0D, 0x31, 0x35, 0x31, 0x31, 0x31, 0x32, 0x31, 0x36, 0x30, 0x30, 0x30, 0x30, 0x5A,
	0x18, 0x0F, 0x39, 0x39, 0x39, 0x39, 0x31, 0x32, 0x33, 0x31, 0x32, 0x33, 0x35, 0x39, 0x35, 0x39,
	0x5A, 0x30, 0x43, 0x31, 0x1A, 0x30, 0x18, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x11, 0x41, 0x74,
	0x6D, 0x65, 0x6C, 0x20, 0x4F, 0x70, 0x65, 0x6E, 0x53, 0x53, 0x4C, 0x20, 0x44, 0x65, 0x76, 0x31,
	0x25, 0x30, 0x23, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x1C, 0x4F, 0x70, 0x65, 0x6E, 0x53, 0x53,
	0x4C, 0x20, 0x44, 0x65, 0x76, 0x20, 0x41, 0x54, 0x45, 0x43, 0x43, 0x35, 0x30, 0x38, 0x41, 0x20,
	0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE,
	0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00,
	0x04, 0xA7, 0x15, 0xDE, 0x0C, 0x04, 0x16, 0x6D, 0xF3, 0xCF, 0x7D, 0x85, 0x5E, 0x3A, 0xD5, 0x74,
	0x02, 0xE6, 0x67, 0xF7, 0xFB, 0x64, 0x22, 0x92, 0x9A, 0xF5, 0x3A, 0x29, 0xE1, 0x1D, 0x0D, 0x03,
	0x95, 0xD5, 0xE4, 0x9E, 0x1D, 0xB8, 0xD9, 0x27, 0x4D, 0x08, 0x5B, 0x6B, 0x7C, 0x0E, 0xD9, 0xD1,
	0x59, 0x32, 0x9E, 0xFC, 0x14, 0x84, 0x2F, 0x93, 0x07, 0x9A, 0xF3, 0xFE, 0x2D, 0x2A, 0xE5, 0x6F,
	0xC5, 0xA3, 0x23, 0x30, 0x21, 0x30, 0x1F, 0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18, 0x30, 0x16,
	0x80, 0x14, 0xBB, 0x0F, 0x75, 0x5F, 0x8F, 0xCC, 0xA3, 0x76, 0x16, 0x97, 0x3B, 0xE1, 0x01, 0xE4,
	0x50, 0x98, 0x94, 0xB9, 0x8D, 0xFF, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04,
	0x03, 0x02, 0x03, 0x48, 0x00, 0x30, 0x45, 0x02, 0x21, 0x00, 0xC0, 0xE8, 0x3B, 0x7C, 0xBF, 0xC5,
	0x96, 0x3D, 0x42, 0x20, 0xAB, 0xDB, 0x97, 0x15, 0x43, 0x48, 0xA1, 0x82, 0xA5, 0x90, 0xF9, 0xCC,
	0xF6, 0x91, 0x12, 0xDD, 0xEE, 0xC7, 0x1B, 0xA3, 0xA7, 0xA6, 0x02, 0x20, 0x27, 0x95, 0xB3, 0xC5,
	0x24, 0x84, 0x04, 0xD8, 0x64, 0x35, 0xF9, 0x7A, 0x0F, 0x8D, 0xFD, 0x91, 0x22, 0x34, 0x81, 0x00,
	0x00, 0x10, 0x37, 0x27, 0xA1, 0x38, 0x8D, 0x26, 0xC4, 0xD8, 0x63, 0xBB
};

///////////////////////////////////////////////////////////////////////////////////////////////////

//#define LOCKABLE_SHA_KEYS

// Data to be written to each Address
uint8_t config_data_default[] = {
	// block 0
	// Not Written: First 16 bytes are not written
	0x01, 0x23, 0x00, 0x00,
	0x00, 0x00, 0x50, 0x00,
	0x04, 0x05, 0x06, 0x07,
	0xEE, 0x00, 0x01, 0x00,
	// I2C, reserved, OtpMode, ChipMode
	0xC0, 0x00, 0xAA, 0x00,
	// SlotConfig
	0x8F, 0x20, 0xC4, 0x44,
	0x87, 0x20, 0xC4, 0x44,
#ifdef LOCKABLE_SHA_KEYS
	0x8F, 0x0F, 0x8F, 0x0F,
	// block 1
	0x9F, 0x0F, 0x82, 0x20,
#else
	0x8F, 0x0F, 0x8F, 0x8F,
	// block 1
	0x9F, 0x8F, 0x82, 0x20,
#endif
	0xC4, 0x44, 0xC4, 0x44,
	0x0F, 0x0F, 0x0F, 0x0F,
	0x0F, 0x0F, 0x0F, 0x0F,
	0x0F, 0x0F, 0x0F, 0x0F,
	// Counters
	0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF,
	// block 2
	0x00, 0x00, 0x00, 0x00,
	// Last Key Use
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	// Not Written: UserExtra, Selector, LockValue, LockConfig (word offset = 5)
	0x00, 0x00, 0x00, 0x00,
	// SlotLock[2], RFU[2]
	0xFF, 0xFF, 0x00, 0x00,
	// X.509 Format
	0x00, 0x00, 0x00, 0x00,
	// block 3
	// KeyConfig
	0x33, 0x00, 0x5C, 0x00,
	0x13, 0x00, 0x5C, 0x00,
#ifdef LOCKABLE_SHA_KEYS
	0x3C, 0x00, 0x3C, 0x00,
	0x3C, 0x00, 0x33, 0x00,
#else
	0x3C, 0x00, 0x1C, 0x00,
	0x1C, 0x00, 0x33, 0x00,
#endif
	0x1C, 0x00, 0x1C, 0x00,
	0x3C, 0x00, 0x3C, 0x00,
	0x3C, 0x00, 0x3C, 0x00,
	0x1C, 0x00, 0x3C, 0x00,
};

/** \brief Configure the ECC508 for use with TLS API funcitons.
 *		The configuration zone is written and locked.
 *		All GenKey and slot initialization is done and then the data zone is locked.
 *		This configuration needs to be performed before the TLS API functions are called
 *		On a locked ECC508 device, this function will check the configuraiton against the default and fail if it does not match.
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_config_default()
{
	ATCA_STATUS status = ATCA_SUCCESS;
	bool isLocked = false;
	bool sameConfig = false;
	uint8_t lockRsp[LOCK_RSP_SIZE] = { 0 };
	uint8_t pubkey[ATCA_PUB_KEY_SIZE] = { 0 };

	do {
		// Get the config lock setting
		if ((status = atcab_is_locked(LOCK_ZONE_CONFIG, &isLocked)) != ATCA_SUCCESS) BREAK(status, "Read of lock byte failed");

		if (isLocked == false) {
			// Configuration zone must be unlocked for the write to succeed
			if ((status = atcab_write_ecc_config_zone(config_data_default)) != ATCA_SUCCESS) BREAK(status, "Write config zone failed");

			// Lock the config zone
			if ((status = atcab_lock_config_zone(lockRsp) != ATCA_SUCCESS)) BREAK(status, "Lock config zone failed");

			// At this point we have a properly configured and locked config zone
			// GenKey all public-private key pairs
			if ((status = atcab_genkey(TLS_SLOT_AUTH_PRIV, pubkey)) != ATCA_SUCCESS) BREAK(status, "Genkey failed:AUTH_PRIV_SLOT");
			if ((status = atcab_genkey(TLS_SLOT_ECDH_PRIV, pubkey)) != ATCA_SUCCESS) BREAK(status, "Genkey failed:ECDH_PRIV_SLOT");
			if ((status = atcab_genkey(TLS_SLOT_FEATURE_PRIV, pubkey)) != ATCA_SUCCESS) BREAK(status, "Genkey failed:FEATURE_PRIV_SLOT");
		}else {
			// If the config zone is locked, compare the bytes to this configuration
			if ((status = atcab_cmp_config_zone(config_data_default, &sameConfig)) != ATCA_SUCCESS) BREAK(status, "Config compare failed");
			if (sameConfig == false) {
				// The device is locked with the wrong configuration, return an error
				status = ATCA_GEN_FAIL;
				BREAK(status, "The device is locked with the wrong configuration");
			}
		}
		// Lock the Data zone
		// Don't get status since it is ok if it's already locked
		atcab_lock_data_zone(lockRsp);

	} while (0);

	return status;
}

/** \brief Initialize the ECC508 for use with the TLS API.  Like a constructor
 *  \param[in] pCfg The ATCAIfaceCfg configuration that defines the HAL layer interface
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_init(ATCAIfaceCfg* pCfg)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	// Insert any other constructor code for TLS here.
	status = atcab_init(pCfg);
	return status;
}

/** \brief Finalize the ECC508 when finished.  Like a destructor
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_finish()
{
	ATCA_STATUS status = ATCA_SUCCESS;

	// Insert any other destructor code for TLS here.
	status = atcab_release();
	return status;
}

/** \brief Get the serial number of this device
 *  \param[out] sn_out Pointer to the buffer that will hold the 9 byte serial number read from this device
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_get_sn(uint8_t sn_out[ATCA_SERIAL_NUM_SIZE])
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Call the basic API to get the serial number
		if ((status = atcab_read_serial_number(sn_out)) != ATCA_SUCCESS) BREAK(status, "Get serial number failed");

	} while (0);

	return status;
}

/** \brief Sign the message with the specified slot and return the signature
 *  \param[in] slotid The private P256 key slot to use for signing
 *  \param[in] message A pointer to the 32 byte message to be signed
 *  \param[out] signature A pointer that will hold the 64 byte P256 signature
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_sign(uint8_t slotid, const uint8_t *message, uint8_t *signature)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Check the inputs
		if (message == NULL || signature == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Bad input parameters");
		}
		// Sign the message
		if ((status = atcab_sign(slotid, message, signature)) != ATCA_SUCCESS) BREAK(status, "Sign Failed");

	} while (0);

	return status;
}

/** \brief Verify the signature of the specified message using the specified public key
 *  \param[in] message A pointer to the 32 byte message to be verified
 *  \param[in] signature A pointer to the 64 byte P256 signature to be verified
 *  \param[in] pubkey A pointer to the 64 byte P256 public key used for verificaion
 *  \param[out] verified A pointer to the boolean result of this verify operation
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_verify(const uint8_t *message, const uint8_t *signature, const uint8_t *pubkey, bool *verified)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Check the inputs
		if (message == NULL || signature == NULL || pubkey == NULL || verified == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Bad input parameters");
		}
		// Verify the signature of the message
		if ((status = atcab_verify_extern(message, signature, pubkey, verified)) != ATCA_SUCCESS) BREAK(status, "Verify Failed");

	} while (0);

	return status;
}

/**
 * \brief Verify a certificate against its certificate authority's public key using the host's ATECC device for crypto functions.
 * \param[in] cert_def       Certificate definition describing how to extract the TBS and signature components from the certificate specified.
 * \param[in] cert           Certificate to verify.
 * \param[in] cert_size      Size of the certificate (cert) in bytes.
 * \param[in] ca_public_key  The ECC P256 public key of the certificate authority that signed this
 *                           certificate. Formatted as the 32 byte X and Y integers concatenated
 *                           together (64 bytes total).
 * \return ATCA_SUCCESS if the verify succeeds, ATCACERT_VERIFY_FAILED or ATCA_EXECUTION_ERROR if it fails to verify.
 */
ATCA_STATUS atcatls_verify_cert(const atcacert_def_t* cert_def, const uint8_t* cert, size_t cert_size, const uint8_t* ca_public_key)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Check the inputs
		if (cert_def == NULL || cert == NULL || ca_public_key == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Bad input parameters");
		}
		// Verify the certificate
		status = atcacert_verify_cert_hw(cert_def, cert, cert_size, ca_public_key);
		if (status != ATCA_SUCCESS) BREAK(status, "Verify Failed");

	} while (0);

	return status;
}

/** \brief Generate a pre-master key (pmk) given a private key slot and a public key that will be shared with
 *  \param[in] Slotid slot of key for ECDH computation
 *  \param[in] Pubkey public to shared with
 *  \param[out] Pmk - computed ECDH key - A buffer with size of ATCA_KEY_SIZE
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_ecdh(uint8_t slotid, const uint8_t* pubkey, uint8_t* pmk)
{
	return atcatls_ecdh_enc(slotid, TLS_SLOT_ENC_PARENT, pubkey, pmk);
}

/** \brief Generate a pre-master key (pmk) given a private key slot and a public key that will be shared with.
 *         This version performs an encrypted read from (slotid + 1)
 *  \param[in] slotid Slot of key for ECDH computation
 *  \param[in] enckeyId Slot of key for the encryption parent
 *  \param[in] pubkey Public to shared with
 *  \param[out] pmk - Computed ECDH key - A buffer with size of ATCA_KEY_SIZE
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_ecdh_enc(uint8_t slotid, uint8_t enckeyId, const uint8_t* pubkey, uint8_t* pmk)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t encKey[ECDH_KEY_SIZE] = { 0 };

	do {
		// Check the inputs
		if (pubkey == NULL || pmk == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Bad input parameters");
		}
		// Get the encryption key for this platform
		if ((status = atcatls_get_enckey(encKey)) != ATCA_SUCCESS) BREAK(status, "Get enckey Failed");

		// Send the encrypted version of the ECDH command with the public key provided
		if ((status = atcab_ecdh_enc(slotid, pubkey, pmk, encKey, enckeyId)) != ATCA_SUCCESS) BREAK(status, "ECDH Failed");

	} while (0);

	return status;
}

/** \brief Generate a pre-master key (pmk) given a private key slot to create and a public key that will be shared with
 *  \param[in] slotid Slot of key for ECDHE computation
 *  \param[in] pubkey Public to share with
 *  \param[out] pubkeyret Public that was created as part of the ECDHE operation
 *  \param[out] pmk - Computed ECDH key - A buffer with size of ATCA_KEY_SIZE
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_ecdhe(uint8_t slotid, const uint8_t* pubkey, uint8_t* pubkeyret, uint8_t* pmk)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Check the inputs
		if ((pubkey == NULL) || (pubkeyret == NULL) || (pmk == NULL)) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Bad input parameters");
		}
		// Create a new key in the ECDH slot
		if ((status = atcab_genkey(slotid, pubkeyret)) != ATCA_SUCCESS) BREAK(status, "Create key failed");

		// Send the ECDH command with the public key provided
		if ((status = atcab_ecdh(slotid, pubkey, pmk)) != ATCA_SUCCESS) BREAK(status, "ECDH failed");


	} while (0);

	return status;
}

/** \brief Create a unique public-private key pair in the specified slot
 *  \param[in] slotid The slot id to create the ECC private key
 *  \param[out] pubkey Pointer the public key bytes that coorespond to the private key that was created
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_create_key(uint8_t slotid, uint8_t* pubkey)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (pubkey == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Call the Genkey command on the specified slot
		if ((status = atcab_genkey(slotid, pubkey)) != ATCA_SUCCESS) BREAK(status, "Create key failed");

	} while (0);

	return status;
}

/** \brief Get the public key from the specified private key slot
 *  \param[in] slotid The slot id containing the private key used to calculate the public key
 *  \param[out] pubkey Pointer the public key bytes that coorespond to the private key
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_calc_pubkey(uint8_t slotid, uint8_t *pubkey)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (pubkey == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Call the GenKey command to return the public key
		if ((status = atcab_get_pubkey(slotid, pubkey)) != ATCA_SUCCESS) BREAK(status, "Gen public key failed");

	} while (0);

	return status;
}

/** \brief reads a pub key from a readable data slot versus atcab_get_pubkey which generates a pubkey from a private key slot
 *  \param[in] slotid Slot number to read, expected value is 0x8 through 0xF
 *  \param[out] pubkey Pointer the public key bytes that were read from the slot
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_read_pubkey(uint8_t slotid, uint8_t *pubkey)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (pubkey == NULL || slotid < 8 || slotid > 0xF) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Bad atcatls_read_pubkey() input parameters");
		}
		// Call the GenKey command to return the public key
		if ((status = atcab_read_pubkey(slotid, pubkey)) != ATCA_SUCCESS) BREAK(status, "Read public key failed");

	} while (0);

	return status;
}

/** \brief Get a random number
 *  \param[out] randout Pointer the 32 random bytes that were returned by the Random Command
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_random(uint8_t* randout)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (randout == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Call the random command
		if ((status = atcab_random(randout)) != ATCA_SUCCESS) BREAK(status, "Random command failed");

	} while (0);

	return status;
}

/** \brief Set the function used to retrieve the unique encryption key for this platform.
 *  \param[in] fn_get_enckey Pointer to a function that will return the platform encryption key
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatlsfn_set_get_enckey(atcatlsfn_get_enckey* fn_get_enckey)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (fn_get_enckey == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Set the get_enckey callback function
		_fn_get_enckey = fn_get_enckey;

	} while (0);

	return status;
}

/** \brief Initialize the unique encryption key for this platform.
 *		Write a random number to the parent encryption key slot
 *		Return the random number for storage on platform
 *  \param[out] enckeyout Pointer to a random 32 byte encryption key that will be stored on the platform and in the device
 *  \param[in] enckeyId Slot id on the ECC508 to store the encryption key
 *  \param[in] lock If this is set to true, the slot that stores the encryption key will be locked
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_init_enckey(uint8_t* enckeyout, uint8_t enckeyId, bool lock)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (enckeyout == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Get a random number
		if ((status = atcatls_random(enckeyout)) != ATCA_SUCCESS) BREAK(status, "Random command failed");

		// Write the random number as the encryption key
		atcatls_set_enckey(enckeyout, enckeyId, lock);

	} while (0);

	return status;
}

/** \brief Initialize the unique encryption key for this platform
 *		Write the provided encryption key to the parent encryption key slot
 *		Function optionally lock the parent encryption key slot after it is written
 *  \param[in] enckeyin Pointer to a 32 byte encryption key that will be stored on the platform and in the device
 *  \param[in] enckeyId Slot id on the ECC508 to store the encryption key
 *  \param[in] lock If this is set to true, the slot that stores the encryption key will be locked
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_set_enckey(uint8_t* enckeyin, uint8_t enckeyId, bool lock)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t block = 0;
	uint8_t offset = 0;
	uint8_t lockSuccess = 0;
	uint8_t enckeyIdByte = (uint8_t)enckeyId;

	do {
		// Verify input parameters
		if (enckeyin == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Write the random number to specified slot
		if ((status = atcab_write_zone(ATCA_ZONE_DATA, enckeyIdByte, block, offset, enckeyin, ATCA_BLOCK_SIZE)) != ATCA_SUCCESS)
			BREAK(status, "Write parent encryption key failed");

		// Optionally lock the key
		if (lock)
			// Send the slot lock command for this slot, ignore the return status
			atcab_lock_data_slot(enckeyIdByte, &lockSuccess);

	} while (0);

	return status;
}

/** \brief Return the random number for storage on platform.
 *		This function reads from platform storage, not the ECC508 device
 *		Therefore, the implementation is platform specific and must be provided at integration
 *  \param[out] enckeyout Pointer to a 32 byte encryption key that is stored on the platform and in the device
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_get_enckey(uint8_t* enckeyout)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (enckeyout == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Memset the output to 0x00
		memset(enckeyout, 0x00, ATCA_KEY_SIZE);

		// Call the function provided by the platform.  The encryption key must be stored in the platform
		if (_fn_get_enckey != NULL)
			_fn_get_enckey(enckeyout, ATCA_KEY_SIZE);
		else
			// Get encryption key funciton is not defined.  Return failure.
			status = ATCA_FUNC_FAIL;
	} while (0);

	return status;
}

/** \brief Read encrypted bytes from the specified slot
 *  \param[in]  slotid    The slot id for the encrypted read
 *  \param[in]  block     The block id in the specified slot
 *  \param[in]  enckeyid  The keyid of the parent encryption key
 *  \param[out] data      The 32 bytes of clear text data that was read encrypted from the slot, then decrypted
 *  \param[inout] bufsize In:Size of data buffer.  Out:Number of bytes read
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_enc_read(uint8_t slotid, uint8_t block, uint8_t enckeyId, uint8_t* data, int16_t* bufsize)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t enckey[ATCA_KEY_SIZE] = { 0 };

	do {
		// Verify input parameters
		if (data == NULL || bufsize == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Get the encryption key from the platform
		if ((status = atcatls_get_enckey(enckey)) != ATCA_SUCCESS) BREAK(status, "Get encryption key failed");

		// Memset the input data buffer
		memset(data, 0x00, *bufsize);

		// todo: implement to account for the correct block on the ECC508
		if ((status = atcab_read_enc(slotid, block, data, enckey, enckeyId)) != ATCA_SUCCESS) BREAK(status, "Read encrypted failed");

	} while (0);

	return status;
}

/** \brief Write encrypted bytes to the specified slot
 *  \param[in]  slotid    The slot id for the encrypted write
 *  \param[in]  block     The block id in the specified slot
 *  \param[in]  enckeyid  The keyid of the parent encryption key
 *  \param[out] data      The 32 bytes of clear text data that will be encrypted to write to the slot.
 *  \param[in] bufsize    Size of data buffer.
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_enc_write(uint8_t slotid, uint8_t block, uint8_t enckeyId, uint8_t* data, int16_t bufsize)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t enckey[ATCA_KEY_SIZE] = { 0 };

	do {
		// Verify input parameters
		if (data == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// Get the encryption key from the platform
		if ((status = atcatls_get_enckey(enckey)) != ATCA_SUCCESS) BREAK(status, "Get encryption key failed");

		// todo: implement to account for the correct block on the ECC508
		if ((status = atcab_write_enc(slotid, block, data, enckey, enckeyId)) != ATCA_SUCCESS) BREAK(status, "Write encrypted failed");

	} while (0);

	return status;
}

/** \brief Read a private RSA key from the device.  The read will be encrypted
 *  \param[in]  enckeyid  The keyid of the parent encryption key
 *  \param[out] rsakey    The RSA key bytes
 *  \param[inout] keysize Size of RSA key.
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_enc_rsakey_read(uint8_t enckeyId, uint8_t* rsakey, int16_t* keysize)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t enckey[ATCA_KEY_SIZE] = { 0 };
	uint8_t slotid = RSA_KEY_SLOT;
	uint8_t startBlock = RSA_KEY_START_BLOCK;
	uint8_t memBlock = 0;
	uint8_t numKeyBlocks = RSA2048_KEY_SIZE / ATCA_BLOCK_SIZE;
	uint8_t block = 0;
	uint8_t memLoc = 0;

	do {
		// Verify input parameters
		if (rsakey == NULL || keysize == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		if (*keysize < RSA2048_KEY_SIZE) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "RSA key buffer too small");
		}

		// Get the encryption key from the platform
		if ((status = atcatls_get_enckey(enckey)) != ATCA_SUCCESS) BREAK(status, "Get encryption key failed");

		// Read the RSA key by blocks
		for (memBlock = 0; memBlock < numKeyBlocks; memBlock++) {
			block = startBlock + memBlock;
			memLoc = ATCA_BLOCK_SIZE * memBlock;
			if ((status = atcab_read_enc(slotid, block, &rsakey[memLoc], enckey, enckeyId)) != ATCA_SUCCESS) BREAK(status, "Read RSA failed");
		}
		*keysize = RSA2048_KEY_SIZE;

	} while (0);

	return status;
}

/** \brief Write a private RSA key from the device.  The write will be encrypted
 *  \param[in] enckeyid  The keyid of the parent encryption key
 *  \param[in] rsakey    The RSA key bytes
 *  \param[in] keysize    Size of RSA key.
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_enc_rsakey_write(uint8_t enckeyId, uint8_t* rsakey, int16_t keysize)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t enckey[ATCA_KEY_SIZE] = { 0 };
	uint8_t slotid = RSA_KEY_SLOT;
	uint8_t startBlock = RSA_KEY_START_BLOCK;
	uint8_t memBlock = 0;
	uint8_t numKeyBlocks = RSA2048_KEY_SIZE / ATCA_BLOCK_SIZE;
	uint8_t block = 0;
	uint8_t memLoc = 0;

	do {
		// Verify input parameters
		if (rsakey == NULL ) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		if (keysize < RSA2048_KEY_SIZE) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "RSA key buffer too small");
		}

		// Get the encryption key from the platform
		if ((status = atcatls_get_enckey(enckey)) != ATCA_SUCCESS) BREAK(status, "Get encryption key failed");

		// Read the RSA key by blocks
		for (memBlock = 0; memBlock < numKeyBlocks; memBlock++) {
			block = startBlock + memBlock;
			memLoc = ATCA_BLOCK_SIZE * memBlock;
			if ((status = atcab_write_enc(slotid, block, &rsakey[memLoc], enckey, enckeyId)) != ATCA_SUCCESS) BREAK(status, "Read RSA failed");
		}

	} while (0);

	return status;
}

/** \brief Get the certificate - ENGINEERING TESTS ONLY since this returns a static buffer.
 *          Use atcatls_get_cert() to get the actual certificates
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_get_signer_cert(uint8_t *certout, size_t* certsize)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	size_t sigCertSize = 0;

	do {
		// Verify input parameters
		if (certout == NULL || certsize == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// For now, return the static buffer
		sigCertSize = sizeof(g_SignerCert);
		if (*certsize < sigCertSize) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Buffer not big enough for memcpy");
		}
		memcpy(certout, g_SignerCert, sigCertSize);
		*certsize = sigCertSize;

	} while (0);

	return status;
}

/** \brief Get the certificate - ENGINEERING TESTS ONLY since this returns a static buffer.
 *         Use atcatls_get_cert() to get the actual certificates
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_get_device_cert(uint8_t *certout, size_t* certsize)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	size_t devCertSize = 0;

	do {
		// Verify input parameters
		if (certout == NULL || certsize == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// For now, return the static buffer
		devCertSize = sizeof(g_DeviceCert);
		if (*certsize < devCertSize) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Buffer not big enough for memcpy");
		}
		memcpy(certout, g_DeviceCert, devCertSize);
		*certsize = devCertSize;

	} while (0);

	return status;
}

/** \brief Get the certificate - ENGINEERING TESTS ONLY since this returns a static buffer.
 *         Use atcatls_get_cert() to get the actual certificate
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_get_ca_cert(uint8_t *certout, size_t* certsize)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	size_t caCertSize = 0;

	do {
		// Verify input parameters
		if (certout == NULL || certsize == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}
		// For now, return the static buffer
		caCertSize = sizeof(g_CaCert);
		if (*certsize < caCertSize) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "Buffer not big enough for memcpy");
		}
		memcpy(certout, g_CaCert, caCertSize);
		*certsize = caCertSize;

	} while (0);

	return status;
}

/** \brief Write a public key from the device.
 *  \param[in] slotid The slot ID to write to
 *  \param[in] pubkey The key bytes
 *  \param[in] lock   If true, lock the slot after writing these bytes.
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_write_pubkey(uint8_t slotid, uint8_t pubkey[PUB_KEY_SIZE], bool lock)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t lock_response = 0x00;

	do {
		// Write the buffer as a public key into the specified slot
		if ((status = atcab_write_pubkey(slotid, pubkey)) != ATCA_SUCCESS) BREAK(status, "Write of public key slot failed");


		// Lock the slot if indicated
		if (lock == true)
			if ((status = atcab_lock_data_slot(slotid, &lock_response)) != ATCA_SUCCESS) BREAK(status, "Lock public key slot failed");

	} while (0);

	return status;

}

/** \brief Write a public key from the device.
 *  \param[in] slotid   The slot ID to write to
 *  \param[in] caPubkey The key bytes
 *  \return ATCA_STATUS
 */
ATCA_STATUS atcatls_read_ca_pubkey(uint8_t slotid, uint8_t caPubkey[PUB_KEY_SIZE])
{
	ATCA_STATUS status = ATCA_GEN_FAIL;

	do {
		// Read public key from the specified slot and return it in the buffer provided
		if ((status = atcab_read_pubkey(slotid, caPubkey)) != ATCA_SUCCESS) BREAK(status, "Read of public key slot failed");


	} while (0);

	return status;
}

/**
 * \brief Reads the certificate specified by the certificate definition from the ATECC508A device.
 *        This process involves reading the dynamic cert data from the device and combining it
 *        with the template found in the certificate definition. Return the certificate int der format
 * \param[in] cert_def Certificate definition describing where to find the dynamic certificate information
 *                     on the device and how to incorporate it into the template.
 * \param[in] ca_public_key The ECC P256 public key of the certificate authority that signed this certificate.
 *                          Formatted as the 32 byte X and Y integers concatenated together (64 bytes total).
 * \param[out] cert Buffer to received the certificate.
 * \param[inout] cert_size As input, the size of the cert buffer in bytes.
 *                         As output, the size of the certificate returned in cert in bytes.
 * \return ATCA_STATUS
 */
ATCA_STATUS atcatls_get_cert(const atcacert_def_t* cert_def, const uint8_t *ca_public_key, uint8_t *certout, size_t* certsize)
{
	ATCA_STATUS status = ATCA_SUCCESS;

	do {
		// Verify input parameters
		if (certout == NULL || certsize == NULL) {
			status = ATCA_BAD_PARAM;
			BREAK(status, "NULL inputs");
		}

		// Build a certificate with signature and public key
		status = atcacert_read_cert(cert_def, ca_public_key, certout, certsize);
		if (status != ATCACERT_E_SUCCESS)
			BREAK(status, "Failed to read certificate");

	} while (0);

	return status;
}



