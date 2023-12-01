/* $NetBSD: dirhash.h,v 1.4 2008/10/30 23:02:12 rmind Exp $ */

/*
 * Copyright (c) 2008, 2011 Reinoud Zandijk
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

#ifndef	_SYS_DIRHASH_H_
#define	_SYS_DIRHASH_H_

#include <dirent.h>
#include "queue.h"

#ifndef DIRHASH_SIZE
#define	DIRHASH_SIZE	(1024*1024)
#endif

#define	DIRHASH_HASHBITS	5
#define	DIRHASH_HASHSIZE	(1 << DIRHASH_HASHBITS)
#define	DIRHASH_HASHMASK	(DIRHASH_HASHSIZE - 1)

#ifdef NO_DIRENT_NAMLEN
#	define DIRENT_NAMLEN(d) strlen((d)->d_name)
#else
#	define DIRENT_NAMLEN(d) (d)->d_namlen
#endif


#ifndef _DIRENT_SIZE
	/* guess */
#	define _DIRENT_SIZE(d) (32 + DIRENT_NAMLEN(d))
#endif


/* dirent's d_namlen is to avoid useless costly fid->dirent translations */
struct dirhash_entry {
	uint32_t		 hashvalue;
	uint64_t		 offset;
	uint32_t		 d_namlen;
	uint32_t		 entry_size;
	LIST_ENTRY(dirhash_entry) next;
};

struct dirhash {
	uint32_t		 flags;
	uint32_t		 size;			/* in bytes */
	uint32_t		 refcnt;
	LIST_HEAD(, dirhash_entry) entries[DIRHASH_HASHSIZE];
	LIST_HEAD(, dirhash_entry) free_entries;
	TAILQ_ENTRY(dirhash) next;
};

#define	DIRH_PURGED	0x0001	/* dirhash has been purged */
#define	DIRH_COMPLETE	0x0002	/* dirhash is complete */
#define	DIRH_BROKEN	0x0004	/* dirhash is broken on readin */
#define	DIRH_FLAGBITS	"\10\1DIRH_PURGED\2DIRH_COMPLETE\3DIRH_BROKEN"

void	dirhash_init(void);
/* void	dirhash_finish(void); */

void	dirhash_purge(struct dirhash **);
void	dirhash_purge_entries(struct dirhash *);
void	dirhash_get(struct dirhash **);
void	dirhash_put(struct dirhash *);
void	dirhash_enter(struct dirhash *, struct dirent *, uint64_t,
	    uint32_t, int);
void	dirhash_enter_freed(struct dirhash *, uint64_t, uint32_t);
void	dirhash_mark_freed(struct dirhash *, struct dirhash_entry *,
	    struct dirent *);
int	dirhash_lookup(struct dirhash *, const char *, int,
	    struct dirhash_entry **);
int	dirhash_lookup_freed(struct dirhash *, uint32_t,
	    struct dirhash_entry **);

#endif /* _SYS_DIRHASH_H_ */
