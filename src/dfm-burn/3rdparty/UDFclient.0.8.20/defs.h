/* $NetBSD$ */

/*
 * File "udf.h" is part of the UDFclient toolkit.
 * File $Id: defs.h,v 1.8 2016/04/26 19:35:20 reinoud Exp $ $Name:  $
 *
 * Copyright (c) 2003, 2004, 2005, 2006, 2011
 * 	Reinoud Zandijk <reinoud@netbsd.org>
 * All rights reserved.
 *
 * The UDFclient toolkit is distributed under the Clarified Artistic Licence.
 * A copy of the licence is included in the distribution as
 * `LICENCE.clearified.artistic' and a copy of the licence can also be
 * requested at the GNU foundantion's website.
 *
 * Visit the UDFclient toolkit homepage http://www.13thmonkey.org/udftoolkit/
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
 */


#ifndef _DEFS_H_
#define _DEFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <inttypes.h>


#ifdef NO_INT_FMTIO
	/* assume 32 bits :-/ */
#ifndef PRIu32
#	define PRIu32  "u"
#	define PRIx32  "x"
#	define PRIu64 "lld"
#	define PRIx64 "llx"
#endif
#endif


/* exported flags */
extern int udf_verbose;

#define UDF_MNT_RDONLY		   1
#define UDF_MNT_FORCE		   2

#define UDF_VERBLEV_NONE	   0
#define UDF_VERBLEV_ACTIONS	   1
#define UDF_VERBLEV_TABLES	   2
#define UDF_VERBLEV_MAX		   3

/* constants to identify what kind of identifier we are dealing with */
#define UDF_REGID_DOMAIN	   1
#define UDF_REGID_UDF		   2
#define UDF_REGID_IMPLEMENTATION   3
#define UDF_REGID_APPLICATION	   4
#define UDF_REGID_NAME		  99	/* 99? */

/* DON'T change these: they identify 13thmonkey's UDF toolkit */
#define APP_NAME		"*UDFtoolkit"
#define APP_VERSION_MAIN	0
#define APP_VERSION_SUB		8
#define IMPL_NAME		"*13thMonkey UDFtoolkit"

/* RW content hint for allocation and other purposes */
#define UDF_C_DSCR	0
#define UDF_C_USERDATA	1
#define UDF_C_FIDS	2
#define UDF_C_NODE	3


/* end of Configuration */


#if 1
#	define UDF_VERBOSE(op)			if (udf_verbose) { op; }
#	define UDF_VERBOSE_LEVEL(level, op)	if (udf_verbose >= (level)) { op; }
#	define UDF_VERBOSE_TABLES(op)		UDF_VERBOSE_LEVEL(UDF_VERBLEV_TABLES, op)
#	define UDF_VERBOSE_MAX(op)		UDF_VERBOSE_LEVEL(UDF_VERBLEV_MAX, op)
#else
#	define UDF_VERBOSE(op)
#	define UDF_VERBOSE_LEVEL(level, op)
#	define UDF_VERBOSE_TABLES(op)
#	define UDF_VERBOSE_MAX(op)
#endif

#endif /* _DEFS_H_ */

