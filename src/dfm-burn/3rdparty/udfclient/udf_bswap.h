/*	$NetBSD: udf_bswap.h,v 1.12 2003/04/16 14:27:03 yamt Exp $	*/

/*
 * Copyright (c) 1998 Manuel Bouyer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * `ported' for UDF by Reinoud Zandijk <reinoud@netbsd.org>
 *
 */

#ifndef _UDF_BSWAP_H_
#define _UDF_BSWAP_H_

#if HAVE_ENDIAN_H
#include <endian.h>
#else
#if HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#else
#if HAVE_MACHINE_ENDIAN_H
#include <machine/endian.h>
#endif
#endif
#endif


/* rest only relevant for big endian machines */
#if (BYTE_ORDER == BIG_ENDIAN)
//#warning BIG ENDIAN
//#if (BYTE_ORDER == LITTLE_ENDIAN)	/* ENDIAN SWAP... only for testing!!!!! */

/* inlines for access to swapped data */
static __inline uint16_t udf_rw16 __P((uint16_t));
static __inline uint32_t udf_rw32 __P((uint32_t));
static __inline uint64_t udf_rw64 __P((uint64_t));


#ifdef HAVE_SYS_BSWAP_H
#include <sys/bswap.h>
#endif

#ifdef HAVE_MACHINE_BSWAP_H
#include <machine/bswap.h>
#endif

#if (defined(HAVE_SYS_BSWAP_H) || defined(HAVE_MACHINE_BSWAP_H))

/* bswap macro's defined */
// #warning BSWAP's defined

static __inline uint16_t
udf_rw16(a)
	uint16_t a;
{
	return bswap16(a);
}


static __inline uint32_t
udf_rw32(a)
	uint32_t a;
{
	return bswap32(a);
}


static __inline uint64_t
udf_rw64(a)
	uint64_t a;
{
	return bswap64(a);
}

#else	/* no bswap macro's */

// #warning _NO_ BSWAP's defined

union _bswap_data {
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;
	uint8_t  b[8];
};

static __inline uint16_t
udf_rw16(uint16_t a)
{
	union _bswap_data b;
	b.u16 = a;
	return b.b[0] | (b.b[1] << 8);
}


static __inline uint32_t
udf_rw32(uint32_t a)
{
	union _bswap_data b;
	b.u32 = a;
	return b.b[0] | (b.b[1] << 8) | (b.b[2] << 16) | (b.b[3] << 24);
}


static __inline uint64_t
udf_rw64(uint64_t a)
{
	union _bswap_data b;
	uint32_t low, high;
	b.u64 = a;

	high = b.b[4] | (b.b[5] << 8) | (b.b[6] << 16) | (b.b[7] << 24);
	low  = b.b[0] | (b.b[1] << 8) | (b.b[2] << 16) | (b.b[3] << 24);
	return ((uint64_t) low) | ((uint64_t) high) << 32;
}

#endif /* bswap macro's */

#else /* little endian */

#define udf_rw16(a) ((uint16_t)(a))
#define udf_rw32(a) ((uint32_t)(a))
#define udf_rw64(a) ((uint64_t)(a))

#endif


#define udf_add16(a, b) \
	(a) = udf_rw16(udf_rw16((a)) + (b))
#define udf_add32(a, b) \
	(a) = udf_rw32(udf_rw32((a)) + (b))
#define udf_add64(a, b) \
	(a) = udf_rw64(udf_rw64((a)) + (b))

	
#endif /* !_UDF_BSWAP_H_ */

