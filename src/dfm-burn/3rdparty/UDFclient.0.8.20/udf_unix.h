/* $NetBSD$ */

/*
 * File "udf.h" is part of the UDFclient toolkit.
 * File $Id: udf_unix.h,v 1.5 2011/02/01 20:43:41 reinoud Exp $ $Name:  $
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


#ifndef _UDF_UNIX_H_
#define _UDF_UNIX_H_


#include "udf.h"


/* Predefines */
struct udf_pri_vol;
struct udf_node;


/*
 * Provides :: system buffer structure for file/metadata caching
 *
 * ... free after struct buf ...
 */

struct udf_buf {
	UDF_MUTEX(b_interlock);
	uint32_t		 b_lblk;		/* logical block number for vnode	*/

	struct udf_node		*b_vp;			/* points to its parent udf_node	*/
	uint32_t		 b_flags;		/* B_* flags				*/

	uint8_t			*b_data;		/* buffer itself			*/
	uint8_t			*b_private;		/* private pointer for FS		*/
	uint32_t		 b_bufsize;		/* allocated size			*/
	uint32_t		 b_bcount;		/* size used				*/
	uint32_t		 b_resid;		/* size remaining			*/

	LIST_ENTRY(udf_buf)  b_hash;
	TAILQ_ENTRY(udf_buf) b_vnbufs;			/* used for vnode bufs			*/
	TAILQ_ENTRY(udf_buf) b_lru;			/* lru queue (reuse)			*/
};


TAILQ_HEAD(udf_buf_queue, udf_buf);


/*      
 * These flags are kept in b_flags. Copied from NetBSD's <sys/buf.h>
 */     
#define B_AGE           0x00000001      /* Move to age queue when I/O done. */
#define B_ASYNC         0x00000004      /* Start I/O, do not wait. */
#define B_BAD           0x00000008      /* Bad block revectoring in progress. */
#define B_BUSY          0x00000010      /* I/O in progress. */ 
#define B_SCANNED       0x00000020      /* Block already pushed during sync */
#define B_CALL          0x00000040      /* Call b_iodone from biodone. */
#define B_DELWRI        0x00000080      /* Delay I/O until buffer reused. */ 
#define B_DIRTY         0x00000100      /* Dirty page to be pushed out async. */
#define B_DONE          0x00000200      /* I/O completed. */
#define B_EINTR         0x00000400      /* I/O was interrupted */
#define B_ERROR         0x00000800      /* I/O error occurred. */
#define B_GATHERED      0x00001000      /* LFS: already in a segment. */
#define B_INVAL         0x00002000      /* Does not contain valid info. */
#define B_LOCKED        0x00004000      /* Locked in core (not reusable). */
#define B_NOCACHE       0x00008000      /* Do not cache block after use. */
#define B_CACHE         0x00020000      /* Bread found us in the cache. */
#define B_PHYS          0x00040000      /* I/O to user memory. */
#define B_RAW           0x00080000      /* Set by physio for raw transfers. */
#define B_READ          0x00100000      /* Read buffer. */  
#define B_TAPE          0x00200000      /* Magnetic tape I/O. */
#define B_WANTED        0x00800000      /* Process wants this buffer. */
#define B_WRITE         0x00000000      /* Write buffer (pseudo flag). */
#define B_XXX           0x02000000      /* Debugging flag. */ 
#define B_VFLUSH        0x04000000      /* Buffer is being synced. */
#define B_NEEDALLOC	0x08000000	/* TEMP */

#define BUF_FLAGBITS \
    "\20\1AGE\3ASYNC\4BAD\5BUSY\6SCANNED\7CALL\10DELWRI" \
    "\11DIRTY\12DONE\13EINTR\14ERROR\15GATHERED\16INVAL\17LOCKED\20NOCACHE" \
    "\22CACHE\23PHYS\24RAW\25READ\26TAPE\30WANTED\32XXX\33VFLUSH"
         




struct udf_bufcache {
	/* udf_node buf's multiplexed in one hashtable on (inode, b_lblk) */
	LIST_HEAD(bufcache, udf_buf) udf_bufs[UDF_BUFCACHE_HASHSIZE];

	UDF_MUTEX(bufcache_lock);

	int32_t bcnt;				/* temp var counting alloc/free			*/

	uint32_t lru_len_data, lru_len_metadata;
	uint32_t lru_len_dirty_data, lru_len_dirty_metadata;
	struct udf_buf_queue lru_bufs_data;
	struct udf_buf_queue lru_bufs_metadata;

	/* thread support for bufs & nodes purge (bufcache emulation) */
	pthread_t	 purgethread_id;
	pthread_mutex_t  purgethread_lock;	/* lock the thread code out			*/
	pthread_cond_t	 purgethread_signal;	/* signal there is work to be done		*/
	int		 purgethread_kicked;	/* sanity for spurious wakeups			*/

	pthread_mutex_t	 processed_lock;	/* lock for main threads to wait on		*/
	pthread_cond_t	 processed_signal;	/* signals an action has been done		*/

	int		 thread_active;
	int		 finish_purgethread;	/* ask thread to stop what its doing and exit	*/
	int		 flushall;		/* flusha all dirty buffers			*/
};


extern struct udf_bufcache *udf_bufcache;


/* bufcache emulation */
extern int  udf_get_buf_entry(struct udf_node *udf_node, struct udf_buf **buf_entry_p);
extern void udf_free_buf_entry(struct udf_buf *buf_entry);
extern int  udf_attach_buf_to_node(struct udf_node *udf_node, struct udf_buf *buf_entry);
extern int  udf_detach_buf_from_node(struct udf_node *udf_node, struct udf_buf *buf_entry);
extern int  udf_build_udf_node(struct udf_node *dir_node, struct fileid_desc *fid, struct udf_node **res_sub_node);
extern int  udf_lookup_node_buf(struct udf_node *udf_node, uint32_t lblk, struct udf_buf **buf_p);

extern void udf_mark_buf_clean(struct udf_node *udf_node, struct udf_buf *buf_entry);
extern void udf_mark_buf_dirty(struct udf_node *udf_node, struct udf_buf *buf_entry);

/* maybe not strictly udf_unix */
extern void udf_mark_buf_needing_allocate(struct udf_node *udf_node, struct udf_buf *buf_entry);
extern void udf_mark_buf_allocated(struct udf_node *udf_node, struct udf_buf *buf_entry);

extern int  udf_unix_init(void);
extern int  udf_start_unix_thread(void);
extern int  udf_stop_unix_thread(void);		/* HELP ! need good clear closedown */

/* TEMP */
extern int  udf_purgethread_kick(char *why);


#endif	/* _UDF_UNIX_H_ */

