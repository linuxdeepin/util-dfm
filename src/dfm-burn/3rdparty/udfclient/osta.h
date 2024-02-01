/* $NetBSD$ */

/*
 * Prototypes for the OSTA functions
 *
 */


#ifndef _OSTA_H_
#define _OSTA_H_


#include <ctype.h>
#include <sys/types.h>
#include <inttypes.h>

#ifndef UNIX
#define	UNIX
#endif

#ifndef MAXLEN
#define	MAXLEN	255
#endif


/***********************************************************************
 * The following two typedef's are to remove compiler dependancies.
 * byte needs to be unsigned 8-bit, and unicode_t needs to be
 * unsigned 16-bit.
 */
typedef uint16_t unicode_t;
typedef uint8_t  byte;


int      udf_UncompressUnicode(int, byte *, unicode_t *);
int      udf_CompressUnicode(int, int, unicode_t *, byte *);
uint16_t udf_cksum(uint8_t *s, int n);
uint16_t udf_unicode_cksum(uint16_t *s, int n);
uint16_t udf_ea_cksum(uint8_t *);
int      UDFTransName(unicode_t *, unicode_t *, int);
int      UnicodeLength(unicode_t *string, int maxlength);


#endif /* _OSTA_H_ */

