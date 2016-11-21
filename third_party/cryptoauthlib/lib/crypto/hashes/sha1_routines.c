/**
 * \file
 * \brief Software implementation of the SHA1 algorithm.
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

#include "sha1_routines.h"
#include <string.h>

void CL_hashInit(CL_HashContext *ctx)
{
	static const U32 hashContext_h_init[] = {
		0x67452301,
		0xefcdab89,
		0x98badcfe,
		0x10325476,
		0xc3d2e1f0
	};

	// Initialize context
	memset(ctx, 0, sizeof(*ctx));
	memcpy_P(ctx->h, hashContext_h_init, sizeof(ctx->h));
}

void CL_hashUpdate(CL_HashContext *ctx, const U8 *src, int nbytes)
{

	/*
	   Digest src bytes, updating context.
	 */

	U8 i, freeBytes;
	U32 temp32;

	typedef struct {
		U8 buf[64];
	} Buf64;

	// We are counting on the fact that Buf64 is 64 bytes long.  In
	// principle the compiler may make Buf64 bigger 64 bytes, but this
	// seems unlikely.  Add an assertion check to be sure.  Beware that
	// assert may not be active in release versions.
	//
	//assert(sizeof(Buf64) == 64);

	// Get number of free bytes in the buf
	freeBytes = (U8)(ctx->byteCount);
	freeBytes &= 63;
	freeBytes = (U8)(64 - freeBytes);

	while (nbytes > 0) {

		// Get i, number of bytes to transfer from src
		i = freeBytes;
		if (nbytes < i) i = (U8)nbytes;

		// Copy src bytes to buf
		if (i == 64)
			// This seems to be much faster on IAR than memcpy().
			*(Buf64*)(ctx->buf) = *(Buf64*)src;
		else
			// Have to use memcpy, size is other than 64 bytes.
			memcpy(((U8*)ctx->buf) + 64 - freeBytes, src, i);

		// Adjust for transferred bytes
		src += i;
		nbytes -= i;
		freeBytes -= i;

		// Do SHA crunch if buf is full
		if (freeBytes == 0)
			shaEngine(ctx->buf, ctx->h);

		// Update 64-bit byte count
		temp32 = (ctx->byteCount += i);
		if (temp32 == 0) ++ctx->byteCountHi;

		// Set up for next iteration
		freeBytes = 64;
	}
}

void CL_hashFinal(CL_HashContext *ctx, U8 *dest)
{

	/*
	   Finish a hash calculation and put result in dest.
	 */

	U8 i;
	U8 nbytes;
	U32 temp;
	U8 *ptr;

	/* Append pad byte, clear trailing bytes */
	nbytes = (U8)(ctx->byteCount) & 63;
	((U8*)ctx->buf)[nbytes] = 0x80;
	for (i = (nbytes + 1); i < 64; i++) ((U8*)ctx->buf)[i] = 0;

	/*
	   If no room for an 8-byte count at end of buf, digest the buf,
	   then clear it
	 */
	if (nbytes > (64 - 9)) {
		shaEngine(ctx->buf, ctx->h);
		memset(ctx->buf, 0, 64);
	}

	/*
	   Put the 8-byte bit count at end of buf.  We have been tracking
	   bytes, not bits, so we left-shift our byte count by 3 as we do
	   this.
	 */
	temp = ctx->byteCount << 3; // low 4 bytes of bit count
	ptr = &((U8*)ctx->buf)[63]; // point to low byte of bit count
	for (i = 0; i < 4; i++) {
		*ptr-- = (U8)temp;
		temp >>= 8;
	}
	//
	temp = ctx->byteCountHi << 3;
	temp |= ctx->byteCount >> (32 - 3); // high 4 bytes of bit count
	for (i = 0; i < 4; i++) {
		*ptr-- = (U8)temp;
		temp >>= 8;
	}
	//show("final SHA crunch", ctx->buf, 64);

	/* Final digestion */
	shaEngine(ctx->buf, ctx->h);

	/* Unpack chaining variables to dest bytes. */
	memcpy(dest, ctx->h, 20);
	for (i = 0; i < 5; i++) {
		dest[i * 4 + 0] = (ctx->h[i] >> 24) & 0xFF;
		dest[i * 4 + 1] = (ctx->h[i] >> 16) & 0xFF;
		dest[i * 4 + 2] = (ctx->h[i] >>  8) & 0xFF;
		dest[i * 4 + 3] = (ctx->h[i] >>  0) & 0xFF;
	}
}


void CL_hash(U8 *msg, int msgBytes, U8 *dest)
{
	CL_HashContext ctx;

	CL_hashInit(&ctx);
	CL_hashUpdate(&ctx, msg, msgBytes);
	CL_hashFinal(&ctx, dest);
}

void shaEngine(U32 *buf, U32 *h)
{

	/*
	   SHA-1 Engine.  From FIPS 180.

	   On entry, buf[64] contains the 64 bytes to digest.  These bytes
	   are destroyed.

	   _H[20] contains the 5 chaining variables.  They must have the
	   proper value on entry and are updated on exit.

	   The order of bytes in buf[] and _h[] matches that used by the
	   hardware SHA engine.
	 */

	U8 t;
	U32 a, b, c, d, e;
	U32 temp = 0;
	U8 *p;
	U32 *w = (U32*)buf;

	/*
	   Pack first 64 bytes of buf into w[0,...,15].  Within a word,
	   bytes are big-endian.  Do this in place -- buf[0,...,63]
	   overlays w[0,...,15].
	 */
	p = (U8*)w;
	for (t = 0; t < 16; t++) {
		temp = (temp << 8) | *p++;
		temp = (temp << 8) | *p++;
		temp = (temp << 8) | *p++;
		temp = (temp << 8) | *p++;
		w[t] = temp;
	}

	/*
	   Pack the 20 bytes of _h[] into h[0,...,4].  Do in place using
	   same convention as for buidling w[].
	 */
	//p = (U8*)h;
	//for (t = 0; t < 5; t++) {
	//temp = (temp << 8) | *p++;
	//temp = (temp << 8) | *p++;
	//temp = (temp << 8) | *p++;
	//temp = (temp << 8) | *p++;
	//h[t] = temp;
	//}

	/* Copy the chaining variables to a, b, c, d, e */
	a = h[0];
	b = h[1];
	c = h[2];
	d = h[3];
	e = h[4];

	/* Now do the 80 rounds */
	for (t = 0; t < 80; t++) {

		temp = a;
		leftRotate(temp, 5);
		temp += e;
		temp += w[t & 0xf];

		if (t < 20) {
			temp += (b & c) | (~b & d);
			temp += 0x5a827999L;
		}else if (t < 40) {
			temp += b ^ c ^ d;
			temp += 0x6ed9eba1L;
		}else if (t < 60) {
			temp += (b & c) | (b & d) | (c & d);
			temp += 0x8f1bbcdcL;
		}else {
			temp += b ^ c ^ d;
			temp += 0xca62c1d6L;
		}

		e = d;
		d = c;
		c = b; leftRotate(c, 30);
		b = a;
		a = temp;

		temp = w[t & 0xf] ^ w[(t - 3) & 0xf] ^ w[(t - 8) & 0xf] ^ w[(t - 14) & 0xf];
		leftRotate(temp, 1);
		w[t & 0xf] = temp;

	}

	/* Update the chaining variables */
	h[0] += a;
	h[1] += b;
	h[2] += c;
	h[3] += d;
	h[4] += e;

	/* Unpack the chaining variables into _h[] buffer. */
	//p = (U8*)h;
	//for (t = 0; t < 5; t++) {
	//temp = h[t];
	//p[3] = (U8)temp; temp >>= 8;
	//p[2] = (U8)temp; temp >>= 8;
	//p[1] = (U8)temp; temp >>= 8;
	//p[0] = (U8)temp;
	//p += 4;
	//}

}