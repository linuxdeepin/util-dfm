/* $NetBSD$ */

/*
 * File "udf_readwrite.c" is part of the UDFclient toolkit.
 * File $Id: udf_readwrite.c,v 1.50 2016/04/25 21:01:40 reinoud Exp $ $Name:  $
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


/* #define DEBUG(a) { a; } */
#define DEBUG(a) if (0) { a; }


/* predefines */
#if 1
      extern void udf_dump_descriptor(union dscrptr *dscrpt);
#else
      void udf_dump_descriptor(union dscrptr *dscrptr) {}
#endif


int udf_writeout_session_cache(struct udf_session *udf_session);


/******************************************************************************************
 *
 * Session-cache init and syncing
 *
 ******************************************************************************************/

int udf_init_session_caches(struct udf_session *udf_session) {
	uint32_t sector_size;

	sector_size = udf_session->disc->sector_size;

	UDF_MUTEX_INIT(&udf_session->session_cache_lock);

	udf_session->cache_line_read  = malloc(UDF_READWRITE_LINE_LENGTH * sector_size);
	udf_session->cache_line_write = malloc(UDF_READWRITE_LINE_LENGTH * sector_size);
	assert(udf_session->cache_line_read);
	assert(udf_session->cache_line_write);

	bzero(udf_session->cache_write_callbacks, UDF_READWRITE_LINE_LENGTH * sizeof(struct udf_wrcallback));

	return 0;
}


void udf_sync_session_cache(struct udf_session *udf_session) {
	UDF_MUTEX_LOCK(&udf_session->session_cache_lock);
		/* hmm... have to write out current write-cache */
		udf_writeout_session_cache(udf_session);
	UDF_MUTEX_UNLOCK(&udf_session->session_cache_lock);
}


int udf_sync_caches(struct udf_log_vol *udf_log_vol) {
	struct udf_volumeset	 *udf_volumeset;
	struct udf_partition	 *udf_partition;
	struct udf_part_mapping  *udf_part_mapping;
	uint32_t		  part_num;

	/* XXX need to force writeout of session caches... XXX */
	/* process all `partions->sessions' */

	DEBUG(
		printf("SYNC statistics\n");
		printf("\tbufcache lru_len_data     %d\n", udf_bufcache->lru_len_data);
		printf("\tbufcache lru_len_metadata %d\n", udf_bufcache->lru_len_metadata);
		printf("\tbufcache claimed/released %d\n", udf_bufcache->bcnt);
	);

	udf_volumeset = udf_log_vol->primary->volumeset;
	SLIST_FOREACH(udf_part_mapping, &udf_log_vol->part_mappings, next_mapping) {
		part_num = udf_part_mapping->udf_virt_part_num;

		SLIST_FOREACH(udf_partition, &udf_volumeset->parts, next_partition) {
			if (udf_rw16(udf_partition->partition->part_num) == part_num) {
				/* sync session */
				DEBUG(printf("Syncing session cache for vpart %d, part %d\n", part_num, udf_partition->udf_session->session_num));
				udf_sync_session_cache(udf_partition->udf_session);
			}
		}
	}
	return 0;
}


/******************************************************************************************
 *
 * Session and logvol sector reading/writing (simple caching)
 *
 ******************************************************************************************/

int udf_read_session_sector(struct udf_session *udf_session, uint32_t sector, char *what, uint8_t *buffer, int prefetch_sectors, int rwflags) {
	uint32_t eff_sector, bit, sector_size;
	int32_t  cache_diff;
	int error;

	rwflags = rwflags;	/* unused here */

	/* maximise 'prefetch_sectors' to cache line length */
	prefetch_sectors = MIN(UDF_READWRITE_LINE_LENGTH, prefetch_sectors);
	sector_size = udf_session->disc->sector_size;

	/* XXX cache coherency ???? XXX */
	UDF_MUTEX_LOCK(&udf_session->session_cache_lock);

		eff_sector = udf_session->session_offset + sector;

		/* snoop write cache */
		cache_diff = eff_sector - udf_session->cache_line_w_start;

		if ((cache_diff >= 0) && (cache_diff < UDF_READWRITE_LINE_LENGTH)) {
			bit = (1 << cache_diff);
			if (udf_session->cache_line_w_present & bit) {
				/* return cached value */
				memcpy(buffer, udf_session->cache_line_write + cache_diff * sector_size, sector_size);
				UDF_MUTEX_UNLOCK(&udf_session->session_cache_lock);
				return 0;
			}
			/* not present */
		}

		/* check read cache */
		cache_diff = eff_sector - udf_session->cache_line_r_start;

		if ((cache_diff >= 0) && (cache_diff < UDF_READWRITE_LINE_LENGTH)) {
			bit = (1 << cache_diff);
			if (udf_session->cache_line_r_present & bit) {
				/* return cached value */
				memcpy(buffer, udf_session->cache_line_read + cache_diff * sector_size, sector_size);
				UDF_MUTEX_UNLOCK(&udf_session->session_cache_lock);
				return 0;
			}
			/* not present */
		}

		/* read in from this sector on for the prefetch length */
		/* XXX use `pending' and `unalloc'/`freed' allocentry queue to minimise read/write misses in streams ? XXX */
		/* XXX use 3 write streams ? XXX */
		error = udf_read_physical_sectors(udf_session->disc, eff_sector, prefetch_sectors, what, udf_session->cache_line_read);
		if (!error) {
			udf_session->cache_line_r_start = eff_sector;
			memcpy(buffer, udf_session->cache_line_read, sector_size);
			udf_session->cache_line_r_present = 0;
			for (cache_diff=0; cache_diff < prefetch_sectors; cache_diff++) {
				bit = (1 << cache_diff);
				udf_session->cache_line_r_present |= bit;
			}
			UDF_MUTEX_UNLOCK(&udf_session->session_cache_lock);
			return 0;
		}

		/* what now? */
		DEBUG(
			printf("ERROR! reading chunk\n");
		);

		udf_session->cache_line_r_present = 0;
		error = udf_read_physical_sectors(udf_session->disc, eff_sector, 1, what, buffer);
		if (!error) {
			udf_session->cache_line_r_start = eff_sector;
			udf_session->cache_line_r_present = 1;
		}
		DEBUG(
			if (error) printf("ERROR reading sector %d\n", eff_sector)
		);

	UDF_MUTEX_UNLOCK(&udf_session->session_cache_lock);

	return error;
}


int udf_writeout_session_cache(struct udf_session *udf_session) {
	struct udf_wrcallback *callback;
	uint32_t bit, error_bits, sector_size;
	uint32_t num_sectors;
	int32_t  cache_diff;
	uint32_t start_sector;
	uint8_t *from, *to;
	int error, report_error;

	if (udf_session->cache_line_w_dirty == 0) return 0;

	error_bits   = 0;
	sector_size  = udf_session->disc->sector_size;
	num_sectors  = UDF_READWRITE_LINE_LENGTH;
	start_sector = 0;
	error        = 0;
	report_error = 0;

	if (udf_session->disc->strict_overwrite) {
		/* Have to do our own Read-Modify-Write :( */
		assert((udf_session->cache_line_w_start % UDF_READWRITE_LINE_LENGTH) == 0);

		/* all present ? */
		if (udf_session->cache_line_w_dirty && (udf_session->cache_line_w_present != UDF_READWRITE_ALL_PRESENT)) {
			/* could snoop read buffer for missed sectors */
		}

		/* double check all present */
		if (udf_session->cache_line_w_dirty && (udf_session->cache_line_w_present != UDF_READWRITE_ALL_PRESENT)) {
			/* read in from media :-S */
			udf_session->cache_line_r_present = 0;
			error = udf_read_physical_sectors(udf_session->disc, udf_session->cache_line_w_start, UDF_READWRITE_LINE_LENGTH, "cache line", udf_session->cache_line_read);
			if (error) {
				/* TODO try to fix-up please */
				printf("Error reading physical sectors for cache for line_w_start %d ? : %s\n", udf_session->cache_line_w_start, strerror(error));
			}
			assert(!error);
			udf_session->cache_line_r_start   = udf_session->cache_line_w_start;
			udf_session->cache_line_r_present = UDF_READWRITE_ALL_PRESENT;

			for (cache_diff = 0; cache_diff < UDF_READWRITE_LINE_LENGTH; cache_diff++) {
				bit = (1 << cache_diff);
				if ((udf_session->cache_line_w_present & bit) == 0) {
					from = udf_session->cache_line_read  + cache_diff * sector_size;
					to   = udf_session->cache_line_write + cache_diff * sector_size;
					memcpy(to, from, sector_size);
				}
				udf_session->cache_line_w_present |= bit;
			}
		}
		assert(udf_session->cache_line_w_present == UDF_READWRITE_ALL_PRESENT);
	}
	assert(udf_session->cache_line_w_dirty);

	if (udf_session->cache_line_w_present != UDF_READWRITE_ALL_PRESENT) {
		/* count number of sectors present * (SEQUENTIAL?) */
		start_sector = 0;
		cache_diff   = 0;

		DEBUG(printf("Writing out non complete line\n"));
		DEBUG(printf("present     %032o\n", udf_session->cache_line_w_present));
		/* write out individual sectors */
		while (cache_diff < UDF_READWRITE_LINE_LENGTH) {
			bit = (1 << cache_diff);
			if (udf_session->cache_line_w_present & bit) {
				start_sector = cache_diff;
				num_sectors  = 1;

				/* calculate memory address and disc address */
				from = udf_session->cache_line_write + start_sector * sector_size;
				start_sector += udf_session->session_offset + udf_session->cache_line_w_start;

				/* write! */
				error = udf_write_physical_sectors(udf_session->disc, start_sector, num_sectors, "cache line (bits)", from);
				if (error) {
					error_bits |= bit;
					report_error = error;
				} else {
					udf_session->cache_line_w_dirty &= ~bit;
				}
			}
			cache_diff++;
		}
	} else {
		/* All present : calculate memory address and disc address */
		from = udf_session->cache_line_write + start_sector * sector_size;
		start_sector += udf_session->session_offset + udf_session->cache_line_w_start;

		/* write! */
		assert(num_sectors == UDF_READWRITE_LINE_LENGTH);
		error = udf_write_physical_sectors(udf_session->disc, start_sector, num_sectors, "cache line", from);
		if (error) {
			error_bits = UDF_READWRITE_ALL_PRESENT;
		} else {
			udf_session->cache_line_w_dirty = 0;
		}
		report_error = error;
	}

	if (error_bits) {
		/* ABORT/ROLLBACK */
		for (cache_diff = 0; cache_diff < UDF_READWRITE_LINE_LENGTH; cache_diff++) {
			bit = (1 << cache_diff);
			if (error_bits & bit) {
				from = udf_session->cache_line_write + cache_diff * sector_size;
				callback = &udf_session->cache_write_callbacks[cache_diff];

				udf_session->cache_line_w_present &= ~bit;

				if (callback->function) {
					callback->function(UDF_WRCALLBACK_REASON_ANULATE, callback, report_error, from);
				} else {
					fprintf(stderr, "WARNING: error encountered with NULL callback function\n");
				}
			}
		}
	}

	return error;
}


/* XXX called directly OR called by purging dirty buffers out trough VOP_STRATEGY or trough VOP_INACTIVE XXX */
int udf_write_session_sector(struct udf_session *udf_session, uint32_t sector, char *what, uint8_t *source, int rwflags, struct udf_wrcallback *wrcallback) {
	uint32_t eff_sector, bit, sector_size;
	int32_t  cache_diff;
	int error;

	rwflags = rwflags;	/* unused here */
	what    = what;		/* unused for now */

	assert(udf_session);
	assert(udf_session->cache_line_read);
	assert(udf_session->cache_line_write);
	sector_size  = udf_session->disc->sector_size;

	/* XXX cache coherency ???? XXX */
	error = 0;
	UDF_MUTEX_LOCK(&udf_session->session_cache_lock);
		eff_sector = udf_session->session_offset + sector;
		cache_diff = eff_sector - udf_session->cache_line_w_start;

		if (udf_session->cache_line_w_dirty && ((cache_diff < 0) || (cache_diff >= UDF_READWRITE_LINE_LENGTH))) {
			/* hmm... have to write out current write-cache */
			udf_writeout_session_cache(udf_session);
		}

		if (udf_session->cache_line_w_dirty == 0) {
			if (udf_session->disc->strict_overwrite) {
				udf_session->cache_line_w_start = eff_sector & ~(UDF_READWRITE_LINE_LENGTH-1);
			} else {
				udf_session->cache_line_w_start = eff_sector;
			}
			cache_diff = eff_sector - udf_session->cache_line_w_start;
			udf_session->cache_line_w_present = 0;
		}

		if ((cache_diff >= 0) && (cache_diff < UDF_READWRITE_LINE_LENGTH)) {
			/* its in the cache range: overwrite current value */
			bit = (1 << cache_diff);
			udf_session->cache_line_w_present |= bit;
			udf_session->cache_line_w_dirty   |= bit;
			memcpy(udf_session->cache_line_write + cache_diff * sector_size, source, sector_size);
			if (wrcallback) 
				memcpy(&udf_session->cache_write_callbacks[cache_diff], wrcallback, sizeof(struct udf_wrcallback));
			else
				bzero(&udf_session->cache_write_callbacks[cache_diff], sizeof(struct udf_wrcallback));
			;

			UDF_MUTEX_UNLOCK(&udf_session->session_cache_lock);
			return 0;
		}

	UDF_MUTEX_UNLOCK(&udf_session->session_cache_lock);

	return error;
}


/* reads in 'logvol->lb_size' logical sector size bytes */
int udf_read_logvol_sector(struct udf_log_vol *udf_log_vol, uint32_t vpart_num, uint32_t lb_num, char *what, uint8_t *buffer, uint32_t prefetch_sectors, int rwflags) {
	struct udf_partition	 *udf_partition;
	struct udf_part_mapping  *udf_part_mapping;
	struct udf_session	 *udf_session;
	uint64_t 		  ses_off, trans_valid_len;
	uint64_t 		  offset;
	uint32_t		  length, trans_length, trans_sectors, readahead;
	uint32_t		  lb_size, sector_size;
	uint32_t		  ses_sector, ses_offset;
	int			  error;

	lb_size     = udf_log_vol->lb_size;
	sector_size = udf_log_vol->sector_size;

	DEBUG(
		printf("Read logvol space for %s, from vpart %d, lb_num %d for logical sector size %d\n", what, vpart_num, (int)lb_num, (int) lb_size);
	);

	error = udf_logvol_vpart_to_partition(udf_log_vol, vpart_num, &udf_part_mapping, &udf_partition);
	if (error) return error;

	/* get the offset (in bytes) in the partition for translational purposes */
	offset = (uint64_t) lb_num * lb_size;
	length = lb_size;

	udf_session = udf_partition->udf_session;
	do {
		trans_length = length;
		ses_sector   = 0;

		/* TODO optimalisation: could use `trans_valid_len' and `prefetch_sectors' */
		/* determine the translated address and its translation validity length */
		error = udf_vpartoff_to_sessionoff(udf_log_vol, udf_part_mapping, udf_partition, offset, &ses_off, &trans_valid_len);
		if (error) break;

		ses_sector = ses_off / sector_size;
		ses_offset = ses_off % sector_size;	assert(ses_offset == 0);

		trans_length  = sector_size;
		trans_sectors = 1;

		/* estimate how much we could read-ahead given prefetch sectors and translation validation */
		readahead = MIN(trans_valid_len, prefetch_sectors * lb_size);
		readahead = (readahead + sector_size -1) / sector_size;

		/* XXX could use partition_sector defs XXX */
		error = udf_read_session_sector(udf_session, ses_sector, what, buffer + ses_offset, readahead, rwflags);
		if (error) break;

		/* advance to next block */
		offset	  += trans_length;
		length    -= trans_length;
		buffer    += trans_length;
		prefetch_sectors -= trans_sectors;

		if (length == 0) return error;
	} while (length && !error);

	return EFAULT;
}


/* internal function; sector is allready a partition sector */
void udf_fillin_fids_sector(uint8_t *buffer, uint32_t *fid_pos, uint32_t max_fidpos, uint32_t sector, uint32_t sector_size) {
	struct fileid_desc *fid;
	uint32_t rfid_pos;
	uint32_t fid_len;

	assert(fid_pos);
	assert(buffer);

	rfid_pos = (*fid_pos) % sector_size;
	while (rfid_pos + sizeof(struct desc_tag) <= sector_size) {
		if ((*fid_pos) + sizeof(struct desc_tag) > max_fidpos) {
			return;
		}

		fid = (struct fileid_desc *) (buffer + (*fid_pos));
		fid_len = udf_calc_tag_malloc_size((union dscrptr *) fid, sector_size);

		/* update sector number and recalculate checkum */
		fid->tag.tag_loc = udf_rw32(sector);
		udf_validate_tag_sum((union dscrptr *) fid);

		*fid_pos += fid_len;
		rfid_pos += fid_len;
	}
}


/* writes out 'logvol->lb_size' logical sector size bytes */
/* XXX it ASSUMES that the translation is allready known/filled in (!) (offcource) XXX */
int udf_write_logvol_sector(struct udf_log_vol *udf_log_vol, uint32_t vpart_num, uint32_t lb_num, char *what, uint8_t *buffer, int rwflags, struct udf_wrcallback *wrcallback) {
	struct udf_partition	 *udf_partition;
	struct udf_part_mapping  *udf_part_mapping;
	struct udf_session	 *udf_session;
	union dscrptr *dscrptr;
	uint64_t ses_off, trans_valid_len;
	uint64_t offset;
	uint64_t length, trans_length;
	uint32_t lb_size, sector_size;
	uint32_t ses_sector, ses_offset;
	uint32_t fid_pos, max_fid_pos;
	int	 error, has_fids, recalc_crc, file_type;

	lb_size     = udf_log_vol->lb_size;
	sector_size = udf_log_vol->sector_size;

	DEBUG(
		printf("Write logvol space for %s, rwflags = %d, from vpart %d, lb_num %d for logical sector size %d\n", what, rwflags, vpart_num, (int)lb_num, (int) lb_size);
	);

	error = udf_logvol_vpart_to_partition(udf_log_vol, vpart_num, &udf_part_mapping, &udf_partition);
	if (error) return error;

	/* get the offset (in bytes) in the partition for translational purposes */
	offset = (uint64_t) lb_num * lb_size;
	length = lb_size;

	fid_pos = max_fid_pos = 0;
	has_fids = recalc_crc = 0;
	dscrptr = (union dscrptr *) buffer;	/* doesn't have to be valid */

	if (rwflags == UDF_C_FIDS) {
		/* FIDs in this sector need to be updated, so search the first FID by using the resync function */
		DEBUG(printf("C_FIDS\n"));
		max_fid_pos = lb_size;
		udf_resync_fid_stream(buffer, &fid_pos, max_fid_pos, &has_fids);
		recalc_crc = 0;
	}
	if (rwflags == UDF_C_NODE) {
		DEBUG(printf("C_NODE\n"));
		/* if NODE with possibly an embedded FID stream -> have to patch up the FIDs (max one lbnum though) */
		file_type = 0;
		if (udf_rw16(dscrptr->tag.id) == TAGID_FENTRY) {
			if ((udf_rw16(dscrptr->fe.icbtag.flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK) == UDF_ICB_INTERN_ALLOC) {
				DEBUG(printf("\tINTERN FE\n"));
				fid_pos = (dscrptr->fe.data  - buffer) + udf_rw32(dscrptr->fe.l_ea);
				max_fid_pos = fid_pos + udf_rw64(dscrptr->fe.inf_len);
				has_fids = 1;
				recalc_crc = 1;
				file_type = dscrptr->fe.icbtag.file_type;	/* 8 bit */
			}
		} else {
			if ((udf_rw16(dscrptr->fe.icbtag.flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK) == UDF_ICB_INTERN_ALLOC) {
				DEBUG(printf("\tINTERN EFE\n"));
				fid_pos = (dscrptr->efe.data - buffer) + udf_rw32(dscrptr->efe.l_ea);
				max_fid_pos = fid_pos + udf_rw64(dscrptr->efe.inf_len);
				has_fids = 1;
				recalc_crc = 1;
				file_type = dscrptr->efe.icbtag.file_type;	/* 8 bit */
			}
		}
		if (!((file_type == UDF_ICB_FILETYPE_DIRECTORY) || (file_type == UDF_ICB_FILETYPE_STREAMDIR))) {
			has_fids = 0;
		}
	}
	DEBUG(
		if (rwflags == UDF_C_USERDATA) {
			printf("C_USERDATA\n");
		}
		printf("has_fids = %d, fid_pos = %d, max_fid_pos = %d\n", has_fids, fid_pos, max_fid_pos);
	);

	udf_session = udf_partition->udf_session;
	do {
		trans_length = length;
		ses_sector   = 0;

		/* determine the translated address and its translation validity length */
		error = udf_vpartoff_to_sessionoff(udf_log_vol, udf_part_mapping, udf_partition, offset, &ses_off, &trans_valid_len);
		if (error) break;

		ses_sector  = ses_off  / sector_size;
		ses_offset  = ses_off  % sector_size;	assert(ses_offset == 0);

		/* FIDs need to be updated to include the correct physical sector */
		if (has_fids) {
			udf_fillin_fids_sector(buffer, &fid_pos, max_fid_pos, lb_num, sector_size);
			if (recalc_crc) {
				udf_validate_tag_and_crc_sums(dscrptr);
				recalc_crc = 0;
			}
		}

		/* XXX optimalisation: could use more of `trans_valid_len' XXX */
		trans_length = sector_size;

		/* XXX could use partition_sector defs XXX */
		error = udf_write_session_sector(udf_session, ses_sector, what, buffer, rwflags, wrcallback);
		if (error) break;

		/* advance to next physical sector */
		offset	  += trans_length;
		length    -= trans_length;
		buffer    += trans_length;	/* really? */

		DEBUG(
			printf("write logvol sector loop: recalc_crc = %d, offset = %d, length = %d, buffer = %p\n", recalc_crc, (uint32_t) offset, (uint32_t) length, buffer);
		);

		if (length == 0) {
			return error;
		}
	} while (length && !error);

	return EFAULT;
}


/******************************************************************************************
 *
 * Descriptor readers and writers 
 *
 ******************************************************************************************/


/*
 * Read in an descriptor in either logvol space or in session space determined
 * by the specification of log_vol.
 *
 * In logvol space, lb_num specifies the logical block number in the logical
 * volume wich can be bigger than a sector.
 *
 * In session space, lb_num specifies a distinct sector.
 *
 * The function returns the read in descriptor blob and its length; it deals
 * with both short and long descriptors.
 */

int udf_read_descriptor(struct udf_log_vol *udf_log_vol, uint32_t vpart_num, struct udf_session *udf_session, uint32_t lb_num, char *what, uint32_t cache_flags, union dscrptr **dscr, uint32_t *length) {
	union dscrptr	*cur_dscr, *new_dscr;
	void		*sector0;
	uint32_t	 sector_size, num_sectors, sector;
	uint32_t	 cur_length, new_length;
	uint8_t		*pos;
	int		 error;

	assert(dscr);
	if (length) *length = 0;
	*dscr   = NULL;

	assert((udf_log_vol && !udf_session) || (!udf_log_vol && udf_session));
	sector_size = udf_log_vol ? udf_log_vol->lb_size : (uint32_t) udf_session->disc->sector_size;

	/* All discriptors have a mimimum size of one sector be it logical or physical */
	cur_length  = sector_size;
	num_sectors = 1;

	sector0    = malloc(cur_length);
	cur_dscr   = sector0;
	if (!sector0) {
		printf("\t\t\tOut of memory claiming memory for %s\n", what);
		return ENOMEM;
	}

	/* start reading in sector; read at offset 0 into the logic block */

	if (udf_log_vol) {
		/* could read more in advance? */
		error = udf_read_logvol_sector(udf_log_vol, vpart_num, lb_num, what, (uint8_t *) cur_dscr, num_sectors, cache_flags);
	} else {
		error = udf_read_session_sector(udf_session, lb_num, what,  (uint8_t *) cur_dscr, num_sectors, cache_flags);
	}

	if (!error) error = udf_check_tag(cur_dscr);
	if (!error) {
		new_length = udf_calc_tag_malloc_size(cur_dscr, sector_size);

		DEBUG(
			if (new_length < (uint32_t) udf_rw16(cur_dscr->tag.desc_crc_len) + UDF_DESC_TAG_LENGTH) {
				printf("UDF warning: reading in %s for %d bytes but descriptor crc len is %d bytes\n", what, new_length,
						udf_rw16(cur_dscr->tag.desc_crc_len) + UDF_DESC_TAG_LENGTH);
				udf_dump_descriptor(cur_dscr);
			}
		);

		if (new_length > cur_length) {
			/* extent the current descriptor; length is multiple of (logical or session) sector size */
			num_sectors = (new_length + sector_size -1) / sector_size;
			new_length  = num_sectors * sector_size;

			new_dscr   = malloc(new_length);
			if (new_dscr) {
				/* copy read-in stuff into the new allocated space */
				memcpy(new_dscr, sector0, cur_length);
				free(sector0);

				/* read in the additional sectors */
				cur_dscr   = new_dscr;
				cur_length = new_length;
				for (sector = 1; sector < num_sectors; sector++) {
					pos = ((uint8_t *) cur_dscr) + sector * sector_size;
					if (udf_log_vol) {
						/* could read more in advance? */
						error = udf_read_logvol_sector(udf_log_vol, vpart_num, lb_num + sector, what, pos, num_sectors - sector, cache_flags);
					} else {
						error = udf_read_session_sector(udf_session, lb_num + sector, what, pos, num_sectors - sector, cache_flags);
					}
				}
			} else {
				free(sector0);
			}
		}
	}
	if (!error) {
		*dscr   = cur_dscr;
		if (length) *length = cur_length;	/* if requested return length */
		error = udf_check_tag(*dscr);
		if (!error) error = udf_check_tag_payload(*dscr);
	}
	return error;
}


/* Reads descriptor as in currenly recorded on disc or as is in the cache */
int udf_read_session_descriptor(struct udf_session *udf_session, uint32_t lb_num, char *what, union dscrptr **dscr, uint32_t *length) {
	uint32_t cache_flags;

	cache_flags = UDF_C_DSCR;
	return udf_read_descriptor(NULL, 0, udf_session, lb_num, what, cache_flags, dscr, length);
}


/* Reads descriptor as in currenly recorded on disc or as is in the cache */
int udf_read_logvol_descriptor(struct udf_log_vol *udf_log_vol, uint32_t vpart_num, uint32_t lb_num, char *what, union dscrptr **dscr, uint32_t *length) {
	uint32_t cache_flags;

	cache_flags = UDF_C_DSCR;
	return udf_read_descriptor(udf_log_vol, vpart_num, NULL, lb_num, what, cache_flags, dscr, length);
}


static int udf_write_descriptor(struct udf_log_vol *udf_log_vol, uint32_t vpart_num, struct udf_session *udf_session, uint32_t lb_num, uint32_t dscr_lb_num, char *what, union dscrptr *dscr, struct udf_wrcallback *wrcallback) {
	uint32_t         dscr_length;
	uint32_t	 sector_size;
	uint32_t	 sector, num_sectors;
	uint8_t		*pos;
	int		 error, rwflags;

	assert(dscr);

	assert((udf_log_vol && !udf_session) || (!udf_log_vol && udf_session));
	sector_size = udf_log_vol ? udf_log_vol->lb_size : (uint32_t) udf_session->disc->sector_size;

	/* All discriptors have a mimimum size of one sector be it logical or physical */
	num_sectors = 1;

	dscr_length = udf_calc_tag_malloc_size(dscr, sector_size);

	/* extent the current descriptor; length is multiple of (logical or session) sector size */
	num_sectors = (dscr_length + sector_size -1) / sector_size;

	/* set the rwflags according to what kind of descriptor we are writing */
	wrcallback->flags |= UDF_WRCALLBACK_FLAG_DESCRIPTOR;	/* not needed? */
	rwflags = UDF_C_DSCR;
	if (udf_rw16(dscr->tag.id) == TAGID_FENTRY)
		rwflags = UDF_C_NODE;
	if (udf_rw16(dscr->tag.id) == TAGID_EXTFENTRY)
		rwflags = UDF_C_NODE;

	/* write out descriptor */
	error = 0;
	if (udf_log_vol) {
		/* prepare descriptor for writing */
		dscr->tag.tag_loc = udf_rw32(dscr_lb_num);
		udf_validate_tag_and_crc_sums(dscr);

		/* write sectors */
		for (sector = 0; sector < num_sectors; sector++) {
			pos = ((uint8_t *) dscr) + sector * sector_size;

			/* wrcallback->function is given */
#if 0
			wrcallback->udf_node = 
			wrcallback->lb_num    = lb_num + sector;
			wrcallback->length    = sector_size;
			wrcallback->vpart_num = vpart_num;
#endif
			DEBUG(printf("writing logical sector %8d for %s (sector offset %d)\n", lb_num + sector, what, sector));
			error = udf_write_logvol_sector(udf_log_vol, vpart_num, lb_num + sector, what, pos, rwflags, wrcallback);
			if (error) break;
		}
	} else {
		/* prepare descriptor for writing */
		dscr->tag.tag_loc = udf_rw32(dscr_lb_num);
		udf_validate_tag_and_crc_sums(dscr);

		/* write sectors */
		for (sector = 0; sector < num_sectors; sector++) {
			pos = ((uint8_t *) dscr) + sector * sector_size;
			/* wrcallback->function is given */
#if 0
			wrcallback->lb_num    = lb_num;
			wrcallback->length    = sector_size;
#endif

			DEBUG(printf("writing sector %8d for %s (sector offset %d)\n", lb_num + sector, what, sector));
			error = udf_write_session_sector(udf_session, lb_num + sector, what, pos, rwflags, wrcallback);
			if (error) break;
		}
	}

	return error;
}


/* Write descriptor trough cache if present */
int udf_write_session_descriptor(struct udf_session *udf_session, uint32_t lb_num, char *what, union dscrptr *dscr, struct udf_wrcallback *wrcallback) {
	return udf_write_descriptor(NULL, 0, udf_session, lb_num, lb_num, what, dscr, wrcallback);
}


int udf_write_partition_descriptor(struct udf_partition *udf_partition, uint32_t lb_num, char *what, union dscrptr *dscr, struct udf_wrcallback *wrcallback) {
	uint32_t dscr_lb_num;

	dscr_lb_num = udf_rw32(lb_num + udf_partition->partition->start_loc);
	return udf_write_descriptor(NULL, 0, udf_partition->udf_session, dscr_lb_num, lb_num, what, dscr, wrcallback);
}


/* Write descriptor trough cache if present */
int udf_write_logvol_descriptor(struct udf_log_vol *udf_log_vol, uint32_t vpart_num, uint32_t lb_num, char *what, union dscrptr *dscr, struct udf_wrcallback *wrcallback) {
	return udf_write_descriptor(udf_log_vol, vpart_num, NULL, lb_num, lb_num, what, dscr, wrcallback);
}


/* end of udf_readwrite.c */

