/* $NetBSD$ */

/*
 * File "udf.h" is part of the UDFclient toolkit.
 * File $Id: udf.h,v 1.150 2020/06/10 09:41:30 reinoud Exp $ $Name:  $
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


#ifndef _UDF_H_
#define _UDF_H_

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <inttypes.h>
#include "dirhash.h"

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
#define UDF_MNT_BSWAP		   4

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

/* RW content hint for allocation and other purposes */
#define UDF_C_DSCR	0
#define UDF_C_USERDATA	1
#define UDF_C_FIDS	2
#define UDF_C_NODE	3



/* Configuration */

#define UDF_MINFREE_LOGVOL	(128*1024)	/* max sector is 64kb, give 2 (sorry) slack; use with care */
#define UDF_LOOKUP_READAHEAD	64		/* rewrite lookahead; set lookahead 0 to disable */

/* misc. configuration; don't change unless you know what you're doing */
#define UDF_READWRITE_LINE_LENGTH	32	/* DONT change this! 32 sectors for CD-RW/DVD-RW fixed packets */
#define UDF_READWRITE_ALL_PRESENT	0xffffffff

#define UDF_INODE_HASHBITS 	10
#define UDF_INODE_HASHSIZE	(1<<UDF_INODE_HASHBITS)
#define UDF_INODE_HASHMASK	(UDF_INODE_HASHSIZE - 1)

#define UDF_BUFCACHE_HASHBITS	13
#define UDF_BUFCACHE_HASHSIZE	(1<<UDF_BUFCACHE_HASHBITS)
#define UDF_BUFCACHE_HASHMASK	(UDF_BUFCACHE_HASHSIZE - 1)

#define UDF_BUFCACHE_IDLE_SECS	15	/* chosen... but preferably before the device spins down (!) */
#define UDF_LRU_METADATA_MIN 	(100 * UDF_READWRITE_LINE_LENGTH)
#define UDF_LRU_METADATA_MAX	(150 * UDF_READWRITE_LINE_LENGTH)
#define UDF_LRU_DATA_MIN	(100 * UDF_READWRITE_LINE_LENGTH)
#define UDF_LRU_DATA_MAX	(300 * UDF_READWRITE_LINE_LENGTH)

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


#define UDF_MUTEX(name) struct { \
		pthread_mutex_t mutex;\
		int locked;\
		char *status;\
		char *file;\
		int line;\
	} name

#define UDF_MUTEX_INIT(name) { \
		pthread_mutex_init(&(name)->mutex, 0); \
		(name)->locked = 0; \
		(name)->status = "initialised as " #name;\
		(name)->file   = __FILE__;\
		(name)->line   = __LINE__;\
	}

#define UDF_MUTEX_LOCK(name) { \
		if (0 && (name)->locked) printf("Waiting for lock " #name " at line %d of file %s marked %s in %s at line %d\n", __LINE__, __FILE__, (name)->status, (name)->file, (name)->line);\
		pthread_mutex_lock(&(name)->mutex);\
		(name)->locked = 1; \
		(name)->status = "locked as " #name;\
		(name)->file   = __FILE__;\
		(name)->line   = __LINE__;\
	}

#define UDF_MUTEX_TRYLOCK(name) { \
		if (0) printf("Trying lock " #name " at line %d of file %s marked %s in %s at line %d\n", __LINE__, __FILE__, (name)->status, (name)->file, (name)->line);\
		if (pthread_mutex_trylock(&(name)->mutex) != EBUSY) {\
			(name)->locked = 1;\
			(name)->status = "locked as " #name;\
			(name)->file   = __FILE__;\
			(name)->line   = __LINE__;\
		};\
	}

#define UDF_MUTEX_UNLOCK(name) { \
		if (0 && !(name)->locked) printf("Unlocking lock " #name " at line %d of file %s marked %s in %s at line %d\n", __LINE__, __FILE__, (name)->status, (name)->file, (name)->line);\
		(name)->locked = 0; \
		(name)->status = "unlocked as " #name;\
		(name)->file   = __FILE__;\
		(name)->line   = __LINE__;\
		pthread_mutex_unlock(&(name)->mutex);\
	}


/* Predefines */
struct udf_node;
struct udf_mountpoint;

#include "osta.h"
#include "ecma167-udf.h"
#include "queue.h"

#include "udf_discop.h"
#include "udf_unix.h"
#include "uio.h"
#include <pthread.h>


/*---------------------------------------------------------------------*/

/*
 * Provides :: write action -> signal written OK or signal write error
 * NOTE: not used anymore but kept for convenience when write error resolution
 * is build.
 */
struct udf_wrcallback;
typedef void (udf_wrcallback_func)(int reason, struct udf_wrcallback *wrcallback, int error, uint8_t *sectordata);

struct udf_wrcallback {
	udf_wrcallback_func	*function;
	struct udf_buf 		*udf_buf;		/* associated buffer */
	struct udf_node		*udf_node;		/* associated vnode  */
	uint32_t		 flags;			/* some reserved flags but other bits are OK			*/
};
#define UDF_WRCALLBACK_REASON_PENDING	0
#define UDF_WRCALLBACK_REASON_ANULATE	1
#define UDF_WRCALLBACK_REASON_WRITTEN	2
#define UDF_WRCALLBACK_FLAG_DESCRIPTOR	(1<<0)
#define UDF_WRCALLBACK_FLAG_BUFFER	(1<<1)


/*
 * Provides :: ``allocentry'' an ordered list of allocation extents
 *
 */

struct udf_allocentry {
	uint32_t		 len;
	uint32_t		 lb_num;
	uint16_t		 vpart_num;
	uint8_t			 flags;

	TAILQ_ENTRY(udf_allocentry) next_alloc;
};
#define UDF_SPACE_ALLOCATED              0
#define UDF_SPACE_FREED                  1
#define UDF_SPACE_ALLOCATED_BUT_NOT_USED 1
#define UDF_SPACE_FREE                   2
#define UDF_SPACE_REDIRECT               3


TAILQ_HEAD(udf_alloc_entries, udf_allocentry);


/*
 * Provides :: ``inode'' or rather filing system dependent v_data node linked under vnode structure
 *
 */

struct udf_node {
	struct udf_mountpoint	*mountpoint;		/* foreign; provides logvol and fileset			*/
	struct udf_log_vol	*udf_log_vol;		/* foreign; backup if its not associated to mountpoint	*/

	int			 dirty;			/* flags if (ext)fentry needs to be synced (safeguard)	*/
	int			 hold;			/* node is on hold i.e. don't recycle nor its buffers	*/

	ino_t			 hashkey;		/* for hashing to lookup inode by vpart & lbnum		*/
	struct stat		 stat;			/* holds unix file attributes/filestats			*/

	struct udf_alloc_entries dscr_allocs;		/* where my complete descriptor space is located	*/

	uint8_t			 udf_filetype;		/* filetype of this node; raw, block, char, softlink... */
	uint8_t			 udf_filechar;		/* udf file characteristics (vis, meta, ...		*/
	uint16_t		 serial_num;		/* serial number of this descriptor on disc		*/
	uint16_t		 file_version_num;	/* file version number of this descriptor on disc	*/

	uint16_t		 udf_icbtag_flags;	/* serial, setuid, setguid, stickybit etc		*/
	uint16_t		 link_cnt;		/* how many FID's are linked to this (ext)fentry	*/
	uint64_t		 unique_id;		/* unique file ID					*/

	/* extended attributes and subfiles */
	struct udf_alloc_entry	*extattrfile_icb;	/* associated extended attribute file			*/
	struct udf_alloc_entry	*streamdir_icb;		/* associated streamdir					*/

	/* internal in-node storage of extended attributes copy. */
	uint8_t			*extended_attr;
	uint32_t		 extended_attr_len;

	/* internal in-node storage of data copy */
	uint8_t		 	*intern_data;		/* descriptor internal data				*/
	uint32_t		 intern_len;		/* length of descriptor internal data			*/
	uint32_t		 intern_free;		/* free space amount in the descriptor besides extattr.	*/

	/* extents of discspace that make up this file/directory */
	uint32_t		 addr_type;		/* storage type of allocation descriptors		*/
	uint32_t		 icb_len;		/* length of an icb descriptor				*/
	UDF_MUTEX(alloc_mutex);
	struct udf_alloc_entries alloc_entries;

	/* all associated vn_bufs for this node */
	UDF_MUTEX(buf_mutex);
	struct udf_buf_queue	 vn_bufs;

	uint32_t		 v_numoutput;

	/* dir hasing */
	struct dirhash		*dir_hash;

	/* lists */
	TAILQ_ENTRY(udf_node)	 next_dirty;		/* next in dirty node list */
	LIST_ENTRY(udf_node)	 next_node;		/* next in hash node list */
};

TAILQ_HEAD(udf_node_list, udf_node);


/*---------------------------------------------------------------------*/


/*
 * Provides :: mountpoint -> fileset descriptor and logical volume
 *
 */
struct udf_mountpoint {
	char			*mount_name;		/* identifier	*/
	struct udf_log_vol	*udf_log_vol;		/* foreign	*/
	struct fileset_desc	*fileset_desc;		/* fileset belonging to this mountpoint	*/

	struct udf_node		*rootdir_node;
	struct udf_node		*streamdir_node;

	int			 writable;		/* flags if its a writable fileset	*/

	SLIST_ENTRY(udf_mountpoint) all_next;		/* for overall mountpoint list		*/
	SLIST_ENTRY(udf_mountpoint) logvol_next;	/* for list of mountpoints in a logvol	*/
};


/*
 * Provides :: logvol_partition -> volumeset physical partition
 */
struct udf_part_mapping {
	uint32_t		  udf_part_mapping_type;
	uint32_t		  vol_seq_num;
	uint32_t		  udf_virt_part_num;
	uint32_t		  udf_phys_part_num;
	union  udf_pmap		 *udf_pmap;		/* foreign	*/

	int			  data_writable;	/* flags if its suited for data */
	int			  metadata_writable;	/* flags if its for meta-data	*/

	/* supporting tables */
	struct udf_sparing_table *sparing_table;

	/* virtual space */
	struct udf_node		 *vat_udf_node;
	struct udf_vat		 *vat;
	uint8_t			 *vat_translation;
	uint32_t		  vat_entries;
	uint32_t		  vat_length;

	/* needs to be updated; metadata partition disabled for now */
	struct udf_node		 *meta_file;
	struct udf_node		 *meta_mirror_file;
	struct udf_node		 *meta_bitmap_file;

	SLIST_ENTRY(udf_part_mapping) next_mapping;	/* for list of partition mappings */
};
#define UDF_PART_MAPPING_ERROR     0
#define UDF_PART_MAPPING_PHYSICAL  1
#define UDF_PART_MAPPING_VIRTUAL   2
#define UDF_PART_MAPPING_SPARABLE  3
#define UDF_PART_MAPPING_META      4
#define UDF_PART_MAPPING_PSEUDO_RW 5


/*
 * Provides :: log_vol -> ...
 *          :: MOUNTPOINTS :)
 */
struct udf_log_vol {
	int			 broken;

	/* primary volume this logical volume is recorded on */
	struct udf_pri_vol 	*primary;		/* foreign */

	/* logical volume info */
	struct logvol_desc	*log_vol;
	uint32_t		 lb_size;		/* constant over logvol in Ecma 167 */
	uint32_t		 sector_size;		/* constant over logvol in Ecma 167 */

	/* logical volume integrity/VAT information */
	uint32_t		 logvol_state;		/* maintained */
	uint16_t		 integrity_serial;
	uint32_t		 min_udf_readver;
	uint32_t		 min_udf_writever;
	uint32_t		 max_udf_writever;
	uint32_t		 num_files;		/* maintained */
	uint32_t		 num_directories;	/* maintained */
	uint64_t		 next_unique_id;	/* maintained */

	int			 writable;		/* flags if its writable */

	/* dirty nodes administration */
	UDF_MUTEX(dirty_nodes_mutex);
	struct udf_node_list	dirty_nodes;

	/* hash table to lookup ino_t -> udf_node */
	LIST_HEAD(inodes, udf_node) udf_nodes[UDF_INODE_HASHSIZE];

	/* estimated free space summation; from logvol integrity */
	uint64_t		 total_space;
	uint64_t		 free_space;
	uint64_t		 await_alloc_space;

	/* consisting of */
	uint32_t		 data_vpart, metadata_vpart;

	uint32_t		 num_mountpoints;	/* display only */
	SLIST_HEAD(mountpoints_list, udf_mountpoint) mountpoints;		/* list of mountables in logvol */

	uint32_t		 num_part_mappings;	/* display only */
	SLIST_HEAD(part_mappings_list, udf_part_mapping) part_mappings;		/* list of partition mappings   */

	/* next in list */
	SLIST_ENTRY(udf_log_vol) next_logvol;		/* for list of logical volumes in a primary volume */
};


/*
 * Provides :: pri_vol -> [log_vol], [part],[ ...]
 *          :: { volumeset -> [pri_vols] }
 */
struct udf_pri_vol {
	struct pri_vol_desc	*pri_vol;
	struct udf_session 	*udf_session;

	struct impvol_desc	*implemation;		/* most likely reduntant */
	struct udf_volumeset	*volumeset;		/* foreign ; nesissary? */
	struct unalloc_sp_desc	*unallocated;

	/* associated logical volumes */
	SLIST_HEAD(logvols, udf_log_vol) log_vols;	/* list of associated logical volumes */

	STAILQ_ENTRY(udf_pri_vol) next_primary;		/* for primary list in volumeset */
};


/*
 * Provides :: partion -> [partition info, session]
 */
struct udf_partition {
	struct part_desc	 *partition;
	struct udf_session	 *udf_session;		/* foreign */

	uint64_t		  part_offset;
	uint64_t		  part_length;

	UDF_MUTEX(partition_space_mutex);		/* MUTEX for unalloc and freed space */
	uint64_t		  free_unalloc_space;
	struct udf_alloc_entries  unalloc_space_queue;	/* authorative */
	struct space_bitmap_desc *unalloc_space_bitmap;	/* placeholder! does NOT have to be up-to-date */

	uint64_t		  free_freed_space;
	struct udf_alloc_entries  freed_space_queue;	/* authorative */
	struct space_bitmap_desc *freed_space_bitmap;	/* placeholder! does NOT have to be up-to-date */

	SLIST_ENTRY(udf_partition) next_partition;	/* for partition list in volumeset */
};


/*
 * Provides :: volumeset -> [pri_vol]
 *          :: [volumeset]
 */
struct udf_volumeset {
	int			 obsolete;
	uint32_t		 max_partnum;

	STAILQ_HEAD(primaries, udf_pri_vol) primaries;	/* linked list of primary volumes associated	*/
	SLIST_HEAD(parts, udf_partition)    parts;	/* linked list of partitions descriptors	*/

	SLIST_ENTRY(udf_volumeset) next_volumeset;	/* for volumeset list */
};


/*
 * Provides udf_session :: -> [(disc, anchor, tracknum)]
 */
struct udf_session {
	struct udf_discinfo	*disc;
	struct anchor_vdp	 anchor;

	uint16_t		 session_num;
	uint32_t		 session_offset;
	uint32_t		 session_length;

	int			 writable;

	/* physical layer read/write cache */
	UDF_MUTEX(session_cache_lock);

	/* SIMPLE cache */
	uint32_t		 cache_line_r_start;
	uint32_t		 cache_line_r_present;
	uint8_t			*cache_line_read;

	uint32_t		 cache_line_w_start;
	uint32_t		 cache_line_w_present;
	uint32_t		 cache_line_w_dirty;
	uint8_t			*cache_line_write;

	struct udf_wrcallback	cache_write_callbacks[UDF_READWRITE_LINE_LENGTH+1];

	STAILQ_ENTRY(udf_session) next_session;		/* sessions are added at tail to preserve order */
};
/*---------------------------------------------------------------------*/


/* exported functions */
extern int  udf_check_tag(union dscrptr *dscr);
extern int  udf_check_tag_payload(union dscrptr *dscr);
extern int  udf_check_tag_presence(union dscrptr *dscr, int TAG);

extern int  udf_check_session_range(char *range);


/* XXX new kernel like interface XXX */
extern int  udf_read_file_part_uio(struct udf_node *udf_node, char *what, int cachehints, struct uio *data_uio);
extern int  udf_write_file_part_uio(struct udf_node *udf_node, char *what, int cachehints, struct uio *data_uio);
extern void udf_dispose_udf_node(struct udf_node *udf_node);
extern int  udf_getattr(struct udf_node *udf_node, struct stat *stat);
extern int  udf_readdir(struct udf_node *udf_node, struct uio *result_uio, int *eof_res /* int *cookies, int ncookies */);
//extern int  udf_lookup_name_in_dir(struct udf_node *dir_node, struct udf_node **vnode, char *name);		/* not fully VOP_LOOKUP yet */
extern int  udf_lookup_name_in_dir(struct udf_node *dir_node, char *name, int namelen, struct long_ad *icb_loc, struct fileid_desc *fid, int *found);
extern int  udf_readin_udf_node(struct udf_node *dir_node, struct long_ad *udf_icbptr, struct fileid_desc *fid, struct udf_node **res_sub_node);
extern int  udf_sync_udf_node(struct udf_node *udf_node, char *why);			/* writeout node */
extern int  udf_truncate_node(struct udf_node *udf_node, uint64_t length /* ,ioflags */);
extern int  udf_remove_file(struct udf_node *parent_node, struct udf_node *udf_node, char *componentname);
extern int  udf_remove_directory(struct udf_node *dir_node, struct udf_node *udf_node, char *componentname);
extern int  udf_create_file(struct udf_node *dir_node, char *componentname, struct stat *stat, struct udf_node **new_node);
extern int  udf_create_directory(struct udf_node *dir_node, char *componentname, struct stat *stat, struct udf_node **new_node);
extern int  udf_rename(struct udf_node *old_parent, struct udf_node *rename_me, char *old_name, struct udf_node *new_parent, struct udf_node *present, char *new_name);
extern int  udf_unlink_node(struct udf_node *udf_node);

extern int  udf_read_session_sector(struct udf_session *udf_session, uint32_t sector, char *what, uint8_t *buffer, int prefetch_sectors, int rwflags);
extern int  udf_write_session_sector(struct udf_session *udf_session, uint32_t sector, char *what, uint8_t *source, int rwflags, struct udf_wrcallback *wrcallback);
extern int  udf_read_logvol_sector(struct udf_log_vol *udf_log_vol, uint32_t vpart_num, uint32_t lb_num, char *what, uint8_t *buffer, uint32_t prefetch_sectors, int rwflags);
extern int  udf_write_logvol_sector(struct udf_log_vol *udf_log_vol, uint32_t vpart_num, uint32_t lb_num, char *what, uint8_t *buffer, int rwflags, struct udf_wrcallback *wrcallback);

/* call back */
extern int  udf_writeout_file_buffer(struct udf_node *udf_node, char *what, int rwflags, struct udf_buf *buf_entry);

/* special cases like VRS */
extern int  udf_write_session_cache_sector(struct udf_session *udf_session, uint32_t sector, char *what, uint8_t *source, int flags, struct udf_wrcallback *wrcallback);

/* device/disc opener, closer and read/write operations */
extern void udf_init(void);	/* call me first! */
extern int  udf_mount_disc(char *devname, char *range, uint32_t sector_size, int mnt_flags, struct udf_discinfo **disc);
extern int  udf_dismount_disc(struct udf_discinfo *disc);
extern int  udf_open_disc(char *devname, int discop_flags, struct udf_discinfo **disc);
extern int  udf_close_disc(struct udf_discinfo *disc);
extern int  udf_sync_disc(struct udf_discinfo *disc);
extern int  udf_sync_logvol(struct udf_log_vol *udf_log_vol);
extern int  udf_sync_caches(struct udf_log_vol *udf_log_vol);
extern int  udf_open_logvol(struct udf_log_vol *udf_log_vol);
extern int  udf_close_logvol(struct udf_log_vol *udf_log_vol);
extern int  udf_sync_logvol(struct udf_log_vol *udf_log_vol);


/* readers/writers helper functions */
extern int  udf_init_session_caches(struct udf_session *udf_session);
extern int  udf_writeout_udf_node(struct udf_node *udf_node, char *why);
extern int  udf_sync_space_tables(struct udf_log_vol *udf_log_vol);		/* read comment on definition */

extern int  udf_logvol_vpart_to_partition(struct udf_log_vol *udf_log_vol, uint32_t vpart_num, struct udf_part_mapping **udf_part_mapping_ptr, struct udf_partition **udf_partition_ptr);
extern int  udf_vpartoff_to_sessionoff(struct udf_log_vol *udf_log_vol, struct udf_part_mapping *udf_part_mapping, struct udf_partition *udf_partition, uint64_t offset, uint64_t *ses_off, uint64_t *trans_valid_len);

extern int  udf_read_session_descriptor(struct udf_session *udf_session, uint32_t lb_num, char *what, union dscrptr **dscr, uint32_t *length);
extern int  udf_read_logvol_descriptor(struct udf_log_vol *udf_log_vol, uint32_t vpart_num, uint32_t lb_num, char *what, union dscrptr **dscr, uint32_t *length);

extern int  udf_write_session_descriptor(struct udf_session *udf_session, uint32_t lb_num, char *what, union dscrptr *dscr, struct udf_wrcallback *wrcallback);
extern int  udf_write_partition_descriptor(struct udf_partition *udf_partition, uint32_t lb_num, char *what, union dscrptr *dscr, struct udf_wrcallback *wrcallback);
extern int  udf_write_logvol_descriptor(struct udf_log_vol *udf_log_vol, uint32_t vpart_num, uint32_t lb_num, char *what, union dscrptr *dscr, struct udf_wrcallback *wrcallback);



/* exported text-dump functions */
extern void udf_dump_volume_name(char *prefix, struct udf_log_vol *udf_log_vol);
extern void udf_dump_long_ad(char *prefix, struct long_ad *adr);
extern void udf_dump_id(char *prefix, int len, char *id, struct charspec *chsp);
extern void udf_to_unix_name(char *result, char *id, int len, struct charspec *chsp);


/* exported descriptor creators */
extern int  udf_validate_tag_sum(union dscrptr *dscr);
extern int  udf_validate_tag_and_crc_sums(union dscrptr *dscr);
extern uint64_t udf_calc_tag_malloc_size(union dscrptr *dscr, uint32_t udf_sector_size);
extern int udf_read_fid_stream(struct udf_node *dir_node, uint64_t *offset, struct fileid_desc *fid, struct dirent *dirent);
extern void udf_resync_fid_stream(uint8_t *buffer, uint32_t *fid_pos, uint32_t max_fid_pos, int *fid_found);
extern int  udf_create_empty_anchor_volume_descriptor(uint32_t sector_size, uint16_t dscr_ver, uint32_t main_vds_loc, uint32_t reserve_vds_loc, uint32_t length, struct anchor_vdp **vdp);
extern int  udf_create_empty_primary_volume_descriptor(uint32_t sector_size, uint16_t dscr_ver, uint16_t serial, char *volset_id, char *privol_name, int vds_num, int max_vol_seq, struct pri_vol_desc **dscrptr);
extern int  udf_create_empty_partition_descriptor(uint32_t sector_size, uint16_t dscr_ver, uint16_t serial, uint16_t part_num, uint32_t access_type, uint32_t start_loc, uint32_t part_len, uint32_t space_bitmap_size, uint32_t unalloc_space_bitmap, struct part_desc **dscrptr);
extern int  udf_create_empty_unallocated_space_descriptor(uint32_t sector_size, uint16_t dscr_ver, uint16_t serial, struct unalloc_sp_desc **dscrptr);
extern int  udf_create_empty_implementation_use_volume_descriptor(uint32_t sector_size, uint16_t dscr_ver, uint16_t serial, char *logvol_name, struct impvol_desc **dscrptr);
extern int  udf_create_empty_logical_volume_descriptor(uint32_t sector_size, uint16_t dscr_ver, uint16_t serial, char *logvol_name, uint32_t lb_size, uint32_t integrity_start, uint32_t integrity_length, struct logvol_desc **dscrptr);
extern int  udf_create_empty_space_bitmap(uint32_t sector_size, uint16_t dscr_ver, uint32_t num_lbs, struct space_bitmap_desc **dscrptr);
extern int  udf_create_empty_terminator_descriptor(uint32_t sector_size, uint16_t dscr_ver, struct desc_tag **tag);
extern int  udf_create_empty_fileset_desc(uint32_t sector_size, uint16_t dscr_ver, uint32_t fileset_num, char *logvol_name, char *fileset_name, struct fileset_desc **dscrptr);
extern void udf_add_physical_to_logvol(struct logvol_desc *logvol, uint16_t vol_seq_num, uint16_t phys_part_num);
extern void udf_derive_new_logvol_integrity(struct udf_log_vol *udf_log_vol);

/* time related creator functions */
extern void udf_set_timespec_now(struct timespec *timespec);
extern void udf_set_timestamp_now(struct timestamp *timestamp);

/* exported processing functions */
extern int  udf_proc_pri_vol(struct udf_session *udf_session, struct udf_pri_vol **current, struct pri_vol_desc *incomming);
extern int  udf_proc_part(struct udf_pri_vol *primary, struct udf_partition **current, struct part_desc *incomming);
extern int  udf_proc_log_vol(struct udf_pri_vol *primary, struct udf_log_vol ** current, struct logvol_desc *incomming);
extern int  udf_proc_filesetdesc(struct udf_log_vol *udf_log_vol, struct fileset_desc *incomming);
extern int  udf_sync_space_bitmap(struct udf_alloc_entries *queue, struct space_bitmap_desc *sbd, uint32_t lb_size);

/* exported builders */
extern int  udf_init_udf_node(struct udf_mountpoint *mountpoint, struct udf_log_vol *udf_log_vol, char *what, struct udf_node **udf_nodeptr);
extern void udf_insert_node_in_hash(struct udf_node *udf_node);
extern int  udf_allocate_udf_node_on_disc(struct udf_node *udf_node);
extern void udf_node_mark_dirty(struct udf_node *udf_node);

extern int  udf_allocate_lbs(struct udf_log_vol *udf_log_vol, int content, uint32_t req_lbs, char *what, uint16_t *res_vpart_num, uint32_t *res_start_lb, uint32_t *res_num_lbs);
extern int  udf_node_allocate_lbs(struct udf_node *udf_node, int req_lbs, uint16_t *res_vpart_num, uint32_t *res_start_lb, uint32_t *res_num_lbs);

extern int  udf_release_lbs(struct udf_log_vol *udf_log_vol, uint16_t vpart_num, uint32_t lb_num, uint64_t size);
extern int  udf_node_release_extent(struct udf_node *udf_node, uint64_t from, uint64_t to);
extern int  udf_confirm_freespace(struct udf_log_vol *udf_log_vol, int content, uint64_t size);


extern int  udf_create_directory_entry(struct udf_node *dir_node, char *componentname, int filetype, int filechar, struct udf_node *refering, struct stat *stat, struct udf_node **new_node);
extern int  udf_unlink_node(struct udf_node *udf_node);
extern uint64_t udf_increment_unique_id(struct udf_log_vol *udf_log_vol);

/* exported (temp) allocentries */
extern void udf_merge_allocentry_queue(struct udf_alloc_entries *queue, uint32_t lb_size);
extern int  udf_cut_allocentry_queue(struct udf_alloc_entries *queue, uint32_t lb_size, uint64_t offset);
extern void udf_dump_allocentry_queue(char *msg, struct udf_alloc_entries *queue, uint32_t lb_size);
extern int  udf_filepart_mark_extent(struct udf_node *udf_node, uint64_t data_offset, uint64_t data_length, int mark);
extern int  udf_splitup_allocentry_queue(struct udf_alloc_entries *queue, uint32_t lb_size, uint64_t data_offset, uint64_t data_length, struct udf_allocentry **res_firstae, struct udf_allocentry **res_lastae);
extern int  udf_mark_allocentry_queue(struct udf_alloc_entries *queue, uint32_t lb_size, uint64_t data_offset, uint64_t data_length, int mark, struct udf_allocentry **res_firstae, struct udf_allocentry **res_lastae);
extern int  udf_extent_properties(struct udf_alloc_entries *queue, uint32_t lb_size, uint64_t from, uint64_t to, int *res_all_allocated);

/* define static list types and structures */
SLIST_HEAD(discslist,       udf_discinfo);
SLIST_HEAD(volumeset_list,  udf_volumeset);
SLIST_HEAD(mountables_list, udf_mountpoint);

extern struct discslist       udf_discs_list;
extern struct volumeset_list  udf_volumeset_list;
extern struct mountables_list udf_mountables;


#endif	/* _UDF_H_ */

