/* $NetBSD$ */

/*
 * File "udf_allocentries.c" is part of the UDFclient toolkit.
 * File $Id: udf_allocentries.c,v 1.13 2016/04/25 21:01:40 reinoud Exp $ $Name:  $
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


/* XXX strip list to bare minimum XXX */
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <time.h>

#include "uscsilib.h"


/* for locals */
#include "udf.h"
#include "udf_bswap.h"
#include "udf_discop.h"
#include "uio.h"
#include <pthread.h>


/* for scsilib */
extern const char *dvname;


#ifndef MAX
#	define MAX(a,b) ((a)>(b)?(a):(b))
#	define MIN(a,b) ((a)<(b)?(a):(b))
#endif


/* #define DEBUG(a) { a; } */
#define DEBUG(a) if (0) { a; }


/* XXX TODO support non lb_size splits ?? TODO XXX */

/******************************************************************************************
 *
 * Basic operations on udf_alloc_entries queues
 *
 ******************************************************************************************/


void udf_merge_allocentry_queue(struct udf_alloc_entries *queue, uint32_t lb_size) {
	struct udf_allocentry *alloc_entry, *next_alloc;
	uint64_t this_end, next_start;
	int merge;

	TAILQ_FOREACH(alloc_entry, queue, next_alloc) {
		do {
			merge = 0;

			/* only non busy ; prolly old cruft? */
			if (alloc_entry->flags == UDF_SPACE_FREED) break;

			next_alloc = TAILQ_NEXT(alloc_entry, next_alloc);
			if (next_alloc) {
				if (next_alloc->flags != alloc_entry->flags) break;

				if (alloc_entry->flags == UDF_SPACE_ALLOCATED) {
					/* merge on virtual/physical lb_num base; they are automatically adjacent on offset */
					if (next_alloc->vpart_num != alloc_entry->vpart_num) break;
					this_end   = alloc_entry->lb_num * lb_size + alloc_entry->len;
					next_start = next_alloc->lb_num * lb_size;
	
					if (this_end != next_start) break;
				}

				/* Only merge if merge would result in a legal UDF allocation size */
				if (((uint64_t) alloc_entry->len + (uint64_t) next_alloc->len) > ((uint64_t) 1<<30)-1) break;

				/* merge! */
				alloc_entry->len = alloc_entry->len + next_alloc->len;
				TAILQ_REMOVE(queue, next_alloc, next_alloc);
				free(next_alloc);
				merge = 1;
			}
		} while (merge);
	} /* foreach */
}


/* Splits up allocated pieces so that there is a break on `offset' */
int udf_cut_allocentry_queue(struct udf_alloc_entries *queue, uint32_t lb_size, uint64_t offset) {
	struct udf_allocentry *entry, *new_entry;
	uint64_t cur_offset;
	uint64_t total_length, extra_length, max_slot, new_length;
	uint64_t entry_offset;

	total_length = 0;
	TAILQ_FOREACH(entry, queue, next_alloc) {
		total_length += entry->len;
	}

	/* printf("Cutting up at offset %ld (lb %ld)\n", offset, offset / lb_size); */
	if (offset < total_length) {
		/* split */
		cur_offset = 0;
		TAILQ_FOREACH(entry, queue, next_alloc) {
			if ((offset >= cur_offset) && (offset < cur_offset + entry->len)) {
				/* overlap */
				entry_offset = offset - cur_offset;
				entry_offset = (entry_offset / lb_size) * lb_size;
				assert(entry_offset % lb_size == 0);
				if (entry_offset == 0) return 0;

				/* clone the current space */
				new_entry = calloc(1, sizeof(struct udf_allocentry));
				if (!new_entry) return ENOMEM;
				memcpy(new_entry, entry, sizeof(struct udf_allocentry));

				/* split! */
				/* printf("split up lb %d + lb %d due to lb offset = %ld\n", entry->lb_num, entry->len / lb_size, entry_offset); */

				entry->len           = entry_offset;
				new_entry->len      -= entry_offset;
				new_entry->lb_num   += entry_offset / lb_size;
				TAILQ_INSERT_AFTER(queue, entry, new_entry, next_alloc);
				DEBUG(printf("split up due to lb offset = %d\n", (int) entry_offset));
				return 0;
			}
			cur_offset += entry->len;
		}
		printf("Sanity check: i can't be here\n");
		exit(1);
	}

	/* no use to do more if we're there */
	if (offset == total_length) return 0;

	/*
	 * Glue extra piece on the queue (auto-extending)
	 * see if we reached our `goal'; see if we can just extent the last
	 * allocation entry but NEVER more or equal to one block size for that
	 * would alter semantics.
	 */
	entry = TAILQ_LAST(queue, udf_alloc_entries);
	if (!TAILQ_EMPTY(queue)) {
		extra_length = (uint64_t) lb_size*(((uint64_t) entry->len + lb_size -1) / lb_size) - entry->len;
		extra_length = MIN(extra_length, (offset - total_length));
		/* keep semantics: only meant for extending upto blocksize */
		if (extra_length < lb_size) {
			entry->len   += extra_length;
			total_length += extra_length;
		}
	}

	max_slot = ((((uint64_t) 1<<30)-1) / lb_size) * lb_size;
	while (offset > total_length) {
		/* grow queue by adding difference as a zero unallocated space */
		new_length  = offset - total_length;
		new_length  = MIN(max_slot, new_length);

		new_entry = calloc(1, sizeof(struct udf_allocentry));
		if (!new_entry) return ENOMEM;

		new_entry->len   = new_length;
		new_entry->flags = UDF_SPACE_FREE;
		TAILQ_INSERT_TAIL(queue, new_entry, next_alloc);

		total_length += new_entry->len;
	} /* while */

	return 0;
}


/* Splits up allocated pieces so that there is a break on `data_offset' and on `data_offset + data_length' */
int udf_splitup_allocentry_queue(struct udf_alloc_entries *queue, uint32_t lb_size, uint64_t data_offset, uint64_t data_length, struct udf_allocentry **res_firstae, struct udf_allocentry **res_lastae) {
	struct udf_allocentry *entry, *prev_entry;
	uint64_t cur_offset, len;

	entry = prev_entry = NULL;
#if 0
	printf("Split %ld + %ld : \n", data_offset, data_length);
	printf("PRE SPLIT block = lb %d + lb %d (%ld bytes)\n", (int) (data_offset / lb_size), (int) (data_length / lb_size), data_length);
	TAILQ_FOREACH(entry, queue, next_alloc) {
		printf("\t(lb %08d + lb %08d[+ %d]) flag %d\n", entry->lb_num, entry->len/lb_size, entry->len % lb_size, entry->flags);
	}
	printf("END PRE\n");
#endif

	/* cut the string at the specified places */
	(void) udf_cut_allocentry_queue(queue, lb_size, data_offset);
	(void) udf_cut_allocentry_queue(queue, lb_size, data_offset + data_length);

#if 0
	printf("POST SPLIT\n");
	TAILQ_FOREACH(entry, queue, next_alloc) {
		printf("\t(lb %08d + lb %08d[+ %d]) flag %d\n", entry->lb_num, entry->len/lb_size, entry->len % lb_size, entry->flags);
	}
	printf("END POST\n\n");
#endif

	if ((res_firstae == NULL) && (res_lastae == NULL)) return 0;

	if (res_firstae) *res_firstae = NULL;
	if (res_lastae)  *res_lastae  = NULL;

	DEBUG(printf("SEARCH SPLIT block = %"PRIu64" + %"PRIu64"\n", (data_offset / lb_size), (data_length / lb_size)));
	/* search the element-range this splitting induced */
	cur_offset = 0;
	entry = TAILQ_FIRST(queue);
	while (entry) {
		len = entry->len;

		DEBUG(printf("\t(%d + %d) flag %d\n", (int) entry->lb_num, (int) entry->len, (int) entry->flags));
		if (cur_offset + len > data_offset) {
			if (res_firstae) *res_firstae = entry;
			DEBUG(printf("\t\tReturned as first\n"));
			break;
		}
		/* advance */
		cur_offset += len;
		entry = TAILQ_NEXT(entry, next_alloc);
	}
	prev_entry = entry;
	while (entry) {
		len = entry->len;

		if (cur_offset + len > data_offset + data_length) {
			break;
		}
		DEBUG(printf("\t(%d + %d) flag %d\n", (int) entry->lb_num, (int) entry->len, (int) entry->flags));

		/* advance */
		cur_offset += len;
		prev_entry  = entry;
		entry = TAILQ_NEXT(entry, next_alloc);
	}
	if (res_lastae)  *res_lastae = prev_entry;
	DEBUG(printf("\t\tReturned as last\n\t<skip>\n"));

	DEBUG(printf("END POST\n\n"));

	if (res_firstae) assert(*res_firstae);
	if (res_lastae)  assert(*res_lastae);
	
	return 0;
}


/* mark a piece with the specified `mark' with no side-effects */
/* tested OK */
int udf_mark_allocentry_queue(struct udf_alloc_entries *queue, uint32_t lb_size, uint64_t data_offset, uint64_t data_length, int mark, struct udf_allocentry **res_firstae, struct udf_allocentry **res_lastae) {
	struct udf_allocentry *alloc_entry, *first_alloc_entry, *last_alloc_entry;
	int error;

	DEBUG(printf("mark %d\n", mark));
	/* first split up so we don't have to worry about boundaries */
	error = udf_splitup_allocentry_queue(queue, lb_size, data_offset, data_length, &first_alloc_entry, &last_alloc_entry);
	assert(error == 0);

	alloc_entry = first_alloc_entry;
	/* inclusive last_alloc_entry */
	last_alloc_entry = TAILQ_NEXT(last_alloc_entry, next_alloc);
	while (alloc_entry != last_alloc_entry) {
		DEBUG(printf("marking %d + %d into type %d\n", alloc_entry->lb_num, alloc_entry->len/lb_size, mark);)
		alloc_entry->flags = mark;
		alloc_entry = TAILQ_NEXT(alloc_entry, next_alloc);
	}

	if (res_firstae) *res_firstae = first_alloc_entry;
	if (res_lastae)  *res_lastae  = last_alloc_entry;

	return 0;
} 


int udf_extent_properties(struct udf_alloc_entries *queue, uint32_t lb_size, uint64_t from, uint64_t to, int *res_all_allocated) {
	struct udf_allocentry *alloc_entry, *first_alloc_entry, *last_alloc_entry;
	int all_allocated, error;

	/* first split up so we don't have to worry about boundaries */
	error = udf_splitup_allocentry_queue(queue, lb_size, from, to-from, &first_alloc_entry, &last_alloc_entry);
	assert(error == 0);

	/* inclusive last_alloc_entry */
	alloc_entry = first_alloc_entry;
	last_alloc_entry = TAILQ_NEXT(last_alloc_entry, next_alloc);
	all_allocated = 1;
	while (alloc_entry != last_alloc_entry) {
		all_allocated = all_allocated && ((alloc_entry->flags == UDF_SPACE_ALLOCATED) || (alloc_entry->flags == UDF_SPACE_ALLOCATED_BUT_NOT_USED));
		alloc_entry = TAILQ_NEXT(alloc_entry, next_alloc);
	}
	if (res_all_allocated) *res_all_allocated = all_allocated;

	return 0;
}


void udf_dump_allocentry_queue(char *msg, struct udf_alloc_entries *queue, uint32_t lb_size) {
	struct udf_allocentry *entry;
	uint64_t offset;

	printf("\n%s :", msg);
	offset = 0;
	TAILQ_FOREACH(entry, queue, next_alloc) {
		printf(" [%d : lb %08d till lb %08d] mapped on (lb %d + %d bytes)  ",
				entry->flags, (uint32_t) (offset/lb_size), (uint32_t) (offset + entry->len)/lb_size-1,
				(uint32_t) (entry->lb_num/lb_size), (uint32_t) entry->len);
		offset += entry->len;
	}
	printf("\n");
}


/* end of udf_allocentries.c */

