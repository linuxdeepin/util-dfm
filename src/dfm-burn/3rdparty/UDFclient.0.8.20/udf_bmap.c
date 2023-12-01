/* $NetBSD$ */

/*
 * File "udf_bmap.c" is part of the UDFclient toolkit.
 * File $Id: udf_bmap.c,v 1.28 2016/04/25 21:01:40 reinoud Exp $ $Name:  $
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


#ifndef MAX
#	define MAX(a,b) ((a)>(b)?(a):(b))
#	define MIN(a,b) ((a)<(b)?(a):(b))
#endif


//#define DEBUG(a) { a; }
#define DEBUG(a) if (0) { a; }


void udf_dump_allocentry_queue(char *msg, struct udf_alloc_entries *queue, uint32_t lb_size);



/******************************************************************************************
 *
 * Get and release free logical blocks from the free space tables
 *
 ******************************************************************************************/

/*
 * simple metadata distribution algorithm; first part of each block is
 * metadata followed by data; the offset this blocking can be tweaked by
 * part_offset.
 */
static int udf_allocate_lbs_on_rewritables(struct udf_partition *udf_partition, uint64_t part_offset, uint64_t metadata_granularity, uint32_t lb_size, uint64_t req_size, int content, uint32_t *res_start_lb, uint32_t *res_num_lbs) {
	struct udf_allocentry    *alloc_entry, *chosen;
	struct udf_alloc_entries *queue;
	uint64_t	 start;
	uint64_t	 chosen_offset;
	uint64_t	 size;
	int		 error, is_meta;

	/* allways unalloc_space_queue ? */
	queue = &udf_partition->unalloc_space_queue;

	/* TODO this is a simplification; no multiple queues yet XXX */
	is_meta = (content != UDF_C_USERDATA);

	/* start from the beginning of the unallocated space queue */
	alloc_entry = TAILQ_FIRST(queue);
	start = alloc_entry->lb_num;
	assert(start == 0);

	chosen = NULL;
	chosen_offset = 0;

	size   = 0;
	TAILQ_FOREACH(alloc_entry, queue, next_alloc) {
		/* we can only start on lb_size (extra sanity check) */
		assert(start % lb_size == 0);

		/* piecewise if nessisary */
		size = MIN(alloc_entry->len, req_size);
		size = (uint64_t) lb_size * (size / lb_size);
		if (size == 0) continue;

		/* we can only give out on lb_size boundaries */
		assert(size % lb_size == 0);
		if  (alloc_entry->flags == UDF_SPACE_FREE) {
			DEBUG(printf("got free entry from %"PRIu64" + %"PRIu64"\n", start / lb_size, (start + alloc_entry->len)/lb_size));
			if (is_meta) {
				if (start + req_size + part_offset <= metadata_granularity) {
					chosen = alloc_entry;
					chosen_offset = 0;
				}
			} else {
				chosen_offset = 0;
				if (start + part_offset < metadata_granularity) {
					chosen_offset = metadata_granularity - (start + part_offset);
				}
				if (chosen_offset + size <= alloc_entry->len) {
					chosen = alloc_entry;
				}
			}
			DEBUG(if (chosen && (size > 0)) printf("chosen\n"));
		}
		if (size <= 0) chosen = NULL;
		if (chosen) break;	/* foreach */

		start = start + alloc_entry->len;
	}	/* FOREACH */
	if (!chosen) return ENOSPC;

	assert(size > 0);
	assert(size % lb_size == 0);
	assert(chosen->len % lb_size == 0);
	assert(start + chosen_offset + size <= start + chosen->len);

	start += chosen_offset;
	*res_start_lb = start / lb_size;
	*res_num_lbs  = size  / lb_size;

	error = udf_mark_allocentry_queue(queue, lb_size, start, size, UDF_SPACE_ALLOCATED, NULL, NULL);

	return error;
}


/* get at most req_lbs from the partition and mark the area used */
static int udf_allocate_lbs_on_partition(struct udf_log_vol *udf_log_vol, uint16_t vpart_num, uint32_t req_lbs, int content, uint32_t *res_start_lb, uint32_t *res_num_lbs) {
	struct udf_partition     *udf_partition;
	struct udf_part_mapping  *udf_part_mapping;
	struct udf_alloc_entries *queue;
	uint64_t	 part_length;
	uint64_t	 metadata_granularity, metadata_blk_len;
	uint64_t	 part_offset;
	uint32_t	 lb_size, num_lbs;
	uint64_t	 req_size;
	uint32_t	 cnt;
	int		 error;

	assert(udf_log_vol);
	lb_size = udf_log_vol->lb_size;
	error = udf_logvol_vpart_to_partition(udf_log_vol, vpart_num, &udf_part_mapping, &udf_partition);
	if (error) return error;

	// part_start  = (uint64_t) lb_size * udf_rw32(udf_partition->partition->start_loc);
	part_length = (uint64_t) lb_size * udf_rw32(udf_partition->partition->part_len);

	req_size = (uint64_t) lb_size * req_lbs;

	/* lock against corruption with free */
	UDF_MUTEX_LOCK(&udf_partition->partition_space_mutex);
	switch (udf_part_mapping->udf_part_mapping_type) {
		case UDF_PART_MAPPING_PHYSICAL  :
		case UDF_PART_MAPPING_SPARABLE  :
			/* sparable is just like the physical mapping */

			/* allways unalloc_space_queue ? */
			queue = &udf_partition->unalloc_space_queue;
			udf_merge_allocentry_queue(queue, lb_size);
			DEBUG(udf_dump_allocentry_queue("Alloc ", queue, lb_size));

			metadata_granularity = part_length / 1024;
			metadata_blk_len     = metadata_granularity / 16;

			part_offset = 0;
			for (cnt = 0; cnt <= (metadata_granularity / metadata_blk_len); cnt++) {
				num_lbs = 0;
				error = udf_allocate_lbs_on_rewritables(udf_partition, part_offset, metadata_granularity, lb_size, req_size, content, res_start_lb, &num_lbs);
				if (error) {
					assert(error == ENOSPC);
					/* disc is most likely getting full with (meta) data; shift window */
					part_offset += metadata_blk_len;
					num_lbs = 0;
				} else {
					assert(num_lbs >= 1);
					udf_partition->free_unalloc_space -= num_lbs * lb_size;
					udf_log_vol->free_space -= num_lbs * lb_size;
					break;	/* for */
				}
			}
			*res_num_lbs = num_lbs;
			break;
		case UDF_PART_MAPPING_VIRTUAL   :
			/* strict increasing virtual adress giveout */
			printf("UDF: get lbs from virtual partition mapping not implemented yet\n");
			return EBADF;
		case UDF_PART_MAPPING_META      :
			/* select blobs from the metadata file */
			if (udf_part_mapping->meta_bitmap_file == NULL)
				return EROFS;
			printf("UDF: get lbs from metadata partition mapping not implemented yet\n");
			break;
		case UDF_PART_MAPPING_PSEUDO_RW :
			/* strict increasing adress giveout from the pseudo over. track */
			printf("UDF: get lbs from pseudo overwritable partition not implemented yet\n");
			break;
	}

	/* sanity */
	if (*res_num_lbs == 0)
		*res_start_lb = 0;

	/* release partition unalloced and freed space again */
	UDF_MUTEX_UNLOCK(&udf_partition->partition_space_mutex);
	return error;
}


/******************************************************************************************
 *
 * Upper level free space allocation and releasing.
 *
 ******************************************************************************************/

int udf_allocate_lbs(struct udf_log_vol *udf_log_vol, int content, uint32_t req_lbs, char *what, uint16_t *res_vpart_num, uint32_t *res_start_lb, uint32_t *res_num_lbs) {
	struct udf_partition *udf_partition;
	struct udf_part_mapping *udf_part_mapping;
	uint32_t	 num_lbs;
	uint16_t	 vpart_num;
	int		 is_meta, ok;
	int		 error;

	assert(udf_log_vol);
	num_lbs  = 0;		/* shutup gcc */

	/* select udf partition to write to depending on the contents */
	is_meta = ((content == UDF_C_FIDS) || (content == UDF_C_NODE));
	vpart_num = is_meta ? udf_log_vol->metadata_vpart : udf_log_vol->data_vpart;

	/* TODO what about when space is freed in an earlier partition ? reset the vpart's ? */
	DEBUG(printf("UDF allocate %d lbs for node `%s`\n", req_lbs, what));
	do {
		DEBUG(printf("do: vpart_num = %d\n", vpart_num));
		error = udf_logvol_vpart_to_partition(udf_log_vol, vpart_num, &udf_part_mapping, &udf_partition);
		ok = (udf_partition != NULL) && (udf_part_mapping != NULL);
		if (ok && !is_meta) ok = udf_part_mapping->data_writable;
		if (ok &&  is_meta) ok = udf_part_mapping->metadata_writable;
		if (ok) {
			/* try to snoop from this vpart */
			error = udf_allocate_lbs_on_partition(udf_log_vol, vpart_num, req_lbs, content, res_start_lb, &num_lbs);
			if (!error) {
				*res_vpart_num = vpart_num;
				if (res_num_lbs) *res_num_lbs = num_lbs;
			}
			ok = !error;
		}
		if (!ok) {
			DEBUG(printf("advance vpart\n"));
			vpart_num = is_meta ? ++(udf_log_vol->metadata_vpart) : ++(udf_log_vol->data_vpart);
			if (vpart_num >= udf_log_vol->num_part_mappings) {
				udf_log_vol->metadata_vpart = udf_log_vol->data_vpart = 0;
				printf("UDF: logvol discs full ?\n");
				return ENOSPC;
			}
		}
	} while (!ok);
	DEBUG(printf("udf_allocate_lbs: got space on vpart_num = %d; req %d, got %d\n", vpart_num, req_lbs, num_lbs));

	assert((*res_start_lb != 0) && (num_lbs != 0));
	return 0;
}


int udf_node_allocate_lbs(struct udf_node *udf_node, int req_lbs, uint16_t *res_vpart_num, uint32_t *res_start_lb, uint32_t *res_num_lbs) {
	char *what;
	int   content;

	/* content can be USERDATA or FID stream here; checking on udf filetype */
	switch (udf_node->udf_filetype) {
		case UDF_ICB_FILETYPE_DIRECTORY :
		case UDF_ICB_FILETYPE_STREAMDIR :
			content = UDF_C_FIDS;
			what = "FID stream";
			break;
		default :
			content = UDF_C_USERDATA;
			what = "file content";
			break;
	}

	return udf_allocate_lbs(udf_node->udf_log_vol, content, req_lbs, what, res_vpart_num, res_start_lb, res_num_lbs);
}


/* returns non zero if space is available */
int udf_confirm_freespace(struct udf_log_vol *udf_log_vol, int content, uint64_t size) {
	content = content;	/* not used now */

	/* generic check for now */
	if (udf_log_vol->free_space >= udf_log_vol->await_alloc_space + size + UDF_MINFREE_LOGVOL) {
		return 1;
	}

	return 0;
}


int udf_release_lbs(struct udf_log_vol *udf_log_vol, uint16_t vpart_num, uint32_t lb_num, uint64_t size) {
	struct udf_partition     *udf_partition;
	struct udf_part_mapping  *udf_part_mapping;
	struct udf_alloc_entries *queue;
	uint32_t	 lb_size;
	int		 error;

	if (!udf_log_vol) return 0;

	lb_size = udf_log_vol->lb_size;
	error = udf_logvol_vpart_to_partition(udf_log_vol, vpart_num, &udf_part_mapping, &udf_partition);
	if (error) return error;

	/* space can only be freed in lb_size chuncks by definition */
	size = (uint64_t) lb_size * ((size + lb_size -1) / lb_size);
	switch (udf_part_mapping->udf_part_mapping_type) {
		case UDF_PART_MAPPING_PHYSICAL :
		case UDF_PART_MAPPING_SPARABLE :
			/* sparable is just like the physical mapping */
			queue = &udf_partition->unalloc_space_queue;	/* TODO freed- for non sequential MO media */
			UDF_MUTEX_LOCK(&udf_partition->partition_space_mutex);
				error = udf_mark_allocentry_queue(queue, lb_size, (uint64_t) lb_num * lb_size, size, UDF_SPACE_FREE, NULL, NULL);
				udf_partition->free_unalloc_space += size;
				udf_log_vol->free_space += size;
			UDF_MUTEX_UNLOCK(&udf_partition->partition_space_mutex);
			return error;
		case UDF_PART_MAPPING_VIRTUAL  :
			/* freeing is not applicable */
			return 0;
		case UDF_PART_MAPPING_META     :
			/* free blobs in the metadata file */
			printf("UDF: freeing lbs from metadata partition mapping not implemented yet\n");
			break;
		case UDF_PART_MAPPING_PSEUDO_RW :
			/* do we have to keep a space bitmap? */
			printf("UDF: freeing lbs from pseudo rewritable partition mapping not implemented yet\n");
			break;
	}
	return 0;

}


/* !!! needs to be called with alloc entry mutex held !!! */
int udf_node_release_extent(struct udf_node *udf_node, uint64_t from, uint64_t to) {
	struct udf_allocentry *from_ae, *to_ae, *alloc_entry, *last_alloc_entry;
	uint32_t lb_size, lbnum, len;
	uint16_t vpart;
	int error, flags;

	assert(udf_node->alloc_mutex.locked);
	assert(udf_node->udf_log_vol);
	lb_size = udf_node->udf_log_vol->lb_size;
	error = udf_splitup_allocentry_queue(&udf_node->alloc_entries, lb_size, from, to-from, &from_ae, &to_ae);
	if (!error) {
		alloc_entry = from_ae;
		last_alloc_entry = TAILQ_NEXT(to_ae, next_alloc);
		while (alloc_entry != last_alloc_entry) {
			vpart = alloc_entry->vpart_num;
			lbnum = alloc_entry->lb_num;
			flags = alloc_entry->flags;
			len   = alloc_entry->len;

			if (flags == UDF_SPACE_ALLOCATED) {
				error = udf_release_lbs(udf_node->udf_log_vol, vpart, lbnum, len);
				assert(!error);
				alloc_entry->flags = UDF_SPACE_FREE;		/* WORM: or freed? */
			} else {
				DEBUG(printf("udf_filepart_free_extent :freeing a non allocated piece : flags = %d\n", flags));
			}
			alloc_entry = TAILQ_NEXT(alloc_entry, next_alloc);
		}
	} else {
		fprintf(stderr, "udf_filepart_free_extent: splitup failed\n");
	}
	return 0;
}


/* end of udf_bmap.c */

