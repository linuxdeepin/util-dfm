/* $NetBSD$ */

/*
 * File "udf_unix.c" is part of the UDFclient toolkit.
 * File $Id: udf_unix.c,v 1.17 2016/04/25 21:01:40 reinoud Exp $
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
#include "udf_unix.h"
#include "uio.h"
#include <pthread.h>


#ifndef MAX
#	define MAX(a,b) ((a)>(b)?(a):(b))
#	define MIN(a,b) ((a)<(b)?(a):(b))
#endif



/* #define DEBUG(a) { a; } */
#define DEBUG(a) if (0) { a; }


/******************************************************************************************
 *
 * Bufcache emulation
 *
 ******************************************************************************************/

/* shared bufcache structure */
struct udf_bufcache *udf_bufcache = NULL;


int udf_unix_init(void) {
	if (udf_bufcache) {
		fprintf(stderr, "reinit unix_init?\n");
		return 0;
	}

	udf_bufcache = calloc(1, sizeof(struct udf_bufcache));
	assert(udf_bufcache);

	UDF_MUTEX_INIT(&udf_bufcache->bufcache_lock);

	TAILQ_INIT(&udf_bufcache->lru_bufs_data);
	TAILQ_INIT(&udf_bufcache->lru_bufs_metadata);

	pthread_cond_init(&udf_bufcache->purgethread_signal, NULL);
	pthread_mutex_init(&udf_bufcache->purgethread_lock, NULL);

	pthread_cond_init(&udf_bufcache->processed_signal, NULL);
	pthread_mutex_init(&udf_bufcache->processed_lock, NULL);
	
	return 0;
}


/* delete the buf entry */
void udf_free_buf_entry(struct udf_buf *buf_entry) {
	assert(udf_bufcache);

	buf_entry->b_vp    = NULL;	/* detach, i.e. recycle */
	buf_entry->b_flags = 0;		/* just in case */

udf_bufcache->bcnt--;
	free(buf_entry->b_data);
	free(buf_entry);
}


/* XXX knowledge of LBSIZE, FILETYPE, INTERNAL NODE (?) ! XXX */
/* must be called with bufcache lock ! */
int udf_get_buf_entry(struct udf_node *udf_node, struct udf_buf **buf_entry_p) {
	struct udf_log_vol *log_vol;
	struct udf_buf     *buf_entry;
	uint32_t lb_size;

	assert(udf_node);
	assert(udf_bufcache);
	assert(buf_entry_p);

	log_vol = udf_node->udf_log_vol;
	lb_size = log_vol->lb_size;

	*buf_entry_p = NULL;
	buf_entry    = NULL;

	if (udf_node->udf_filetype != UDF_ICB_FILETYPE_RANDOMACCESS) {
		if (udf_bufcache->lru_len_metadata >= UDF_LRU_METADATA_MIN) {
			/* kick writeout of data; if past max wait for space to continue */
			UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);
				udf_purgethread_kick("Data buffer surplus");
				while (udf_bufcache->lru_len_metadata >= UDF_LRU_METADATA_MAX) {
					udf_purgethread_kick("Metadata buffer surplus");
					/* wait for processed signal */
					pthread_mutex_lock(&udf_bufcache->processed_lock);
					pthread_cond_wait(&udf_bufcache->processed_signal, &udf_bufcache->processed_lock);
					pthread_mutex_unlock(&udf_bufcache->processed_lock);
				}
			UDF_MUTEX_LOCK(&udf_bufcache->bufcache_lock);
		}
	} else {
		if (udf_bufcache->lru_len_data >= UDF_LRU_DATA_MIN) {
			/* kick writeout of data; if past max wait for space to continue */
			UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);
				udf_purgethread_kick("Data buffer surplus");
				while (udf_bufcache->lru_len_data >= UDF_LRU_DATA_MAX) {
					udf_purgethread_kick("Data buffer surplus");
					/* wait for processed signal */
					pthread_mutex_lock(&udf_bufcache->processed_lock);
					pthread_cond_wait(&udf_bufcache->processed_signal, &udf_bufcache->processed_lock);
					pthread_mutex_unlock(&udf_bufcache->processed_lock);
				}
			UDF_MUTEX_LOCK(&udf_bufcache->bufcache_lock);
		}
	}

	/* create new buf_entry */
	buf_entry = calloc(1, sizeof(struct udf_buf));
	if (!buf_entry) return ENOMEM;

	buf_entry->b_data = calloc(1, lb_size);
	if (!buf_entry->b_data) {
		*buf_entry_p = NULL;
		free(buf_entry);
		return ENOMEM;
	}
	*buf_entry_p = buf_entry;

	/* fill in details */
	buf_entry->b_bufsize = lb_size;
	buf_entry->b_bcount  = 0;
	buf_entry->b_resid   = lb_size;
	buf_entry->b_lblk    = 0;
	buf_entry->b_flags   = B_INVAL;
	buf_entry->b_vp      = udf_node;	/* not just NULL ? */

udf_bufcache->bcnt++;
	return 0;
}


/* really `out of the sky' hash formula */
uint32_t udf_calc_bufhash(struct udf_node *udf_node, uint32_t b_lblk) {
	return (udf_node->hashkey * 5 + b_lblk);
}


/* !!! needs to be called with bufcache and udf_node->buf_mutex lock !!! */
void udf_mark_buf_needing_allocate(struct udf_node *udf_node, struct udf_buf *buf_entry) {
	uint32_t lb_size;

	assert(udf_node);
	/* assert(udf_node->buf_mutex.locked && udf_bufcache->bufcache_lock.locked); */
	lb_size = udf_node->udf_log_vol->lb_size;

	/* if it isnt allready marked to eb allocated, allocate it and claim space */
	if (!(buf_entry->b_flags & B_NEEDALLOC)) {
		udf_node->udf_log_vol->await_alloc_space += lb_size;
		buf_entry->b_flags |= B_NEEDALLOC;
	}
}


/* !!! needs to be called with bufcache and udf_node->buf_mutex lock !!! */
void udf_mark_buf_allocated(struct udf_node *udf_node, struct udf_buf *buf_entry) {
	uint32_t lb_size;

	assert(udf_node);
	/* assert(udf_node->buf_mutex.locked && udf_bufcache->bufcache_lock.locked); */
	lb_size = udf_node->udf_log_vol->lb_size;

	/* if it needed allocation, clear the flag and release the space */
	if (buf_entry->b_flags & B_NEEDALLOC) {
		udf_node->udf_log_vol->await_alloc_space -= lb_size;
		buf_entry->b_flags &= ~B_NEEDALLOC;
	}
}


/* !!! needs to be called with bufcache and udf_node->buf_mutex lock !!! */
void udf_mark_buf_dirty(struct udf_node *udf_node, struct udf_buf *buf_entry) {
	assert(udf_node);
	assert(buf_entry);
	assert(udf_node->buf_mutex.locked);
	assert(udf_bufcache->bufcache_lock.locked);

	if (buf_entry->b_flags & B_DIRTY)
		return;

	if (udf_node->addr_type == UDF_ICB_INTERN_ALLOC) {
		udf_mark_buf_needing_allocate(udf_node, buf_entry);	/* signal it needs allocation */
	}

	if (udf_node->udf_filetype != UDF_ICB_FILETYPE_RANDOMACCESS) {
		udf_bufcache->lru_len_dirty_metadata++;
	} else {
		udf_bufcache->lru_len_dirty_data++;
	}
	buf_entry->b_flags |= B_DIRTY;
	udf_node->v_numoutput++;
}


/* !!! needs to be called with bufcache and udf_node->buf_mutex lock !!! */
void udf_mark_buf_clean(struct udf_node *udf_node, struct udf_buf *buf_entry) {
	assert(udf_node);
	assert(buf_entry);
	assert(udf_node->buf_mutex.locked);
	assert(udf_bufcache->bufcache_lock.locked);

	if ((buf_entry->b_flags & B_DIRTY) == 0)
		return;

	if (udf_node->udf_filetype != UDF_ICB_FILETYPE_RANDOMACCESS) {
		udf_bufcache->lru_len_dirty_metadata--;
	} else {
		udf_bufcache->lru_len_dirty_data--;
	}
	buf_entry->b_flags &= ~B_DIRTY;
	assert(udf_node->v_numoutput >= 1);
	udf_node->v_numoutput--;
}


/* !!! needs to be called with bufcache and udf_node->buf_mutex lock !!! */
int udf_attach_buf_to_node(struct udf_node *udf_node, struct udf_buf *buf_entry) {
	struct udf_buf_queue *lru_chain;
	struct udf_buf *buf, *lbuf;
	uint32_t hashkey, bucket;

	assert(udf_node);
	assert(buf_entry);
	assert(udf_node->buf_mutex.locked && udf_bufcache->bufcache_lock.locked);

	buf_entry->b_vp = udf_node;

if (0) {
	/*
	 * Insert ordered in list. In KERNEL: vnode->v_dirtyblkhd is a list
	 * that can be reverse-sorted. Ordering not used yet thus commented
	 * out.
	 */
	lbuf = TAILQ_LAST(&udf_node->vn_bufs, udf_buf_queue);
	if (lbuf) {
		if (buf_entry->b_lblk > lbuf->b_lblk) {
			TAILQ_INSERT_TAIL(&udf_node->vn_bufs, buf_entry, b_vnbufs);
		} else {
			buf = TAILQ_FIRST(&udf_node->vn_bufs);
			while (buf->b_lblk < buf_entry->b_lblk) {
				buf = TAILQ_NEXT(buf, b_vnbufs);
			}
			assert((buf->b_lblk != buf_entry->b_lblk) && (buf->b_vp == udf_node));
			TAILQ_INSERT_BEFORE(buf, buf_entry, b_vnbufs);
		}
	} else {
		TAILQ_INSERT_HEAD(&udf_node->vn_bufs, buf_entry, b_vnbufs);
	}
} else {
	TAILQ_INSERT_TAIL(&udf_node->vn_bufs, buf_entry, b_vnbufs);
}

	/* fill buf into the bufcache */
	hashkey = udf_calc_bufhash(udf_node, buf_entry->b_lblk);
	bucket  = hashkey & UDF_BUFCACHE_HASHMASK;

	DEBUG(
		struct udf_buf *buf;

		/* checking for doubles */
		LIST_FOREACH(buf, &udf_bufcache->udf_bufs[bucket], b_hash) {
			if ((buf->b_vp == udf_node) && (buf->b_lblk == buf_entry->b_lblk)) {
				printf("DOUBLE hashnode in UDF_BUFS!?\n");
				exit(1);
			}
		}
	);

	LIST_INSERT_HEAD(&udf_bufcache->udf_bufs[bucket], buf_entry, b_hash);

	/* queue it in the lru chain */
	if ((udf_node->udf_filetype != UDF_ICB_FILETYPE_RANDOMACCESS) && (udf_node->udf_filetype != UDF_ICB_FILETYPE_REALTIME)) {
		lru_chain = &udf_bufcache->lru_bufs_metadata;
		udf_bufcache->lru_len_metadata++;
	} else {
		lru_chain = &udf_bufcache->lru_bufs_data;
		udf_bufcache->lru_len_data++;
	}
	TAILQ_INSERT_TAIL(lru_chain, buf_entry, b_lru);

	return 0;
}


/* kind of brelse() ? */
/* !!! needs to be called with bufcache and udf_node->buf_mutex lock !!! */
int udf_detach_buf_from_node(struct udf_node *udf_node, struct udf_buf *buf_entry) {
	struct udf_buf_queue *lru_chain;

	assert(udf_node);
	assert(buf_entry);
	assert(udf_node->buf_mutex.locked && udf_bufcache->bufcache_lock.locked);

	/* remove from vnode admin */
	TAILQ_REMOVE(&udf_node->vn_bufs, buf_entry, b_vnbufs);

	/* please don't forget this one */
	if (buf_entry->b_flags & B_DIRTY) 
		udf_node->v_numoutput--;

	/* remove from buffer cache */
	LIST_REMOVE(buf_entry, b_hash);

	/* remove from lru lists */
	if ((udf_node->udf_filetype != UDF_ICB_FILETYPE_RANDOMACCESS) && (udf_node->udf_filetype != UDF_ICB_FILETYPE_REALTIME)) {
		lru_chain = &udf_bufcache->lru_bufs_metadata;
		TAILQ_REMOVE(lru_chain, buf_entry, b_lru);
		udf_bufcache->lru_len_metadata--;
	} else {
		lru_chain = &udf_bufcache->lru_bufs_data;
		TAILQ_REMOVE(lru_chain, buf_entry, b_lru);
		udf_bufcache->lru_len_data--;
	}

	return 0;
}


/* bufcache lock has to be held! */
int udf_lookup_node_buf(struct udf_node *udf_node, uint32_t lblk, struct udf_buf **buf_p) {
#ifdef UDF_METADATA_LRU
	struct udf_buf_queue *lru_chain;
#endif
	struct udf_buf *buf;
	uint32_t hashkey, bucket;

	assert(udf_node);
	assert(udf_bufcache->bufcache_lock.locked);

	*buf_p = NULL;

	hashkey = udf_calc_bufhash(udf_node, lblk);
	bucket  = hashkey & UDF_BUFCACHE_HASHMASK;
	LIST_FOREACH(buf, &udf_bufcache->udf_bufs[bucket], b_hash) {
		if ((buf->b_vp == udf_node) && (buf->b_lblk == lblk)) {
			*buf_p = buf;

#ifdef UDF_METADATA_LRU
			lru_chain = &udf_bufcache->lru_bufs_data;
			if (udf_node->udf_filetype != UDF_ICB_FILETYPE_RANDOMACCESS) lru_chain = &udf_bufcache->lru_bufs_metadata;

			TAILQ_REMOVE(lru_chain, buf, b_lru);
			TAILQ_INSERT_TAIL(lru_chain, buf, b_lru);
#endif

			break; /* for each */
		}
	}

	return 0;
}


void *udf_purger(void *arg) {
	struct timespec wakeup;
	struct udf_buf *buf_entry, *marker;
	struct udf_node *udf_node;

	arg = arg;	/* paramter not used */
	marker = calloc(1, sizeof(struct udf_buf));
	assert(marker);

	UDF_VERBOSE(printf("\tbufcache thread initialising\n"));
	while (1) {
		DEBUG(printf("UDF bufcache sync thread: waiting for lock\n"));
		/*
		 * If we are not asked to finish up our writing, block to
		 * wait for more data. Signal the reader to continue just in
		 * case it is still stuck.
		 */
		if (!udf_bufcache->finish_purgethread) {
			do {
				/* determine the time we want to wake up again * */
				clock_gettime(CLOCK_REALTIME, &wakeup);
				wakeup.tv_sec += UDF_BUFCACHE_IDLE_SECS;

				/* ask for more requests */
				pthread_cond_signal(&udf_bufcache->processed_signal);
				pthread_mutex_lock(&udf_bufcache->purgethread_lock);
				pthread_cond_timedwait(&udf_bufcache->purgethread_signal, &udf_bufcache->purgethread_lock, &wakeup);
				pthread_mutex_unlock(&udf_bufcache->purgethread_lock);

				if (!udf_bufcache->purgethread_kicked) {
					/* UDF_VERBOSE_MAX(printf("\nUDF purger woke up due to timeout\n")); */

					/* see if we would want to do something */
					if (udf_bufcache->flushall)	/* shouldn't happen */
						break;
					if (udf_bufcache->lru_len_data >= UDF_LRU_DATA_MIN)
						break;			/* we have something to do */
					if (udf_bufcache->lru_len_metadata >= UDF_LRU_METADATA_MIN)
						break;			/* we have something to do */
				} /* else : we have been explicitly asked to do something */
			} while (!udf_bufcache->purgethread_kicked && !udf_bufcache->finish_purgethread);
		}
		udf_bufcache->purgethread_kicked = 0;

		DEBUG(printf("UDF read/write thread: got activate\n"));

		UDF_MUTEX_LOCK(&udf_bufcache->bufcache_lock);
			/* writeout surplus of dirty buffers */
			/* PURGE surplus bufs if possible */
			if ((udf_bufcache->lru_len_data >= UDF_LRU_DATA_MIN) || udf_bufcache->flushall) {
				DEBUG(printf("UDF read/write thread: purging dirty buffers\n"));
				/* getting too many dirty data buffers */
				TAILQ_INSERT_HEAD(&udf_bufcache->lru_bufs_data, marker, b_lru);
				while ((buf_entry = TAILQ_NEXT(marker, b_lru))) {
					/* advance marker */
					TAILQ_REMOVE(&udf_bufcache->lru_bufs_data, marker, b_lru);
					TAILQ_INSERT_AFTER(&udf_bufcache->lru_bufs_data, buf_entry, marker, b_lru);

					/* process buf_entry */
					if ((buf_entry->b_flags & B_DIRTY) != 0) {
						if ((udf_bufcache->lru_len_dirty_data >= UDF_READWRITE_LINE_LENGTH*2) || udf_bufcache->flushall) {
							UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);
								/* signal there is time/space ahead and write out buffer */
								pthread_cond_signal(&udf_bufcache->processed_signal);
								udf_writeout_file_buffer(buf_entry->b_vp, "dirty data buf", UDF_C_USERDATA, buf_entry);
DEBUG(printf("."); fflush(stdout));
							UDF_MUTEX_LOCK(&udf_bufcache->bufcache_lock);
						}
					}

					/* if there are too many, drop least used */
					if (udf_bufcache->lru_len_data >= UDF_LRU_DATA_MIN) {
						TAILQ_FOREACH(buf_entry, &udf_bufcache->lru_bufs_data, b_lru) {
							/* process buf_entry */
							if ((buf_entry != marker) && ((buf_entry->b_flags & B_DIRTY) == 0)) {
								/* lock node bufs (locking protocol) */
								udf_node = buf_entry->b_vp;
								if (udf_node) {
									UDF_MUTEX_LOCK(&udf_node->buf_mutex);
										udf_detach_buf_from_node(udf_node, buf_entry);
									UDF_MUTEX_UNLOCK(&udf_node->buf_mutex);
								} else {
									printf("\n\nWARNING: got a NULL udf_node freeing dataspace\n\n");
								}
								udf_free_buf_entry(buf_entry);
		
								/* signal there is time/space ahead */
								UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);
									pthread_cond_signal(&udf_bufcache->processed_signal);
								UDF_MUTEX_LOCK(&udf_bufcache->bufcache_lock);

								break; /* mandatory leaving FOREACH */
							}
							if (udf_bufcache->lru_len_data < UDF_LRU_DATA_MIN)
								break;	/* foreach */
						}
					}
				}
				TAILQ_REMOVE(&udf_bufcache->lru_bufs_data, marker, b_lru);
			}
			if ((udf_bufcache->lru_len_metadata >= UDF_LRU_METADATA_MIN) || udf_bufcache->flushall) {
				/* getting too many dirty metadata buffers */
				TAILQ_INSERT_HEAD(&udf_bufcache->lru_bufs_metadata, marker, b_lru);
				while ((buf_entry = TAILQ_NEXT(marker, b_lru))) {
					/* advance marker */
					TAILQ_REMOVE(&udf_bufcache->lru_bufs_metadata, marker, b_lru);
					TAILQ_INSERT_AFTER(&udf_bufcache->lru_bufs_metadata, buf_entry, marker, b_lru);

					/* process buf_entry */
					if ((buf_entry->b_flags & B_DIRTY) != 0) {
						if ((udf_bufcache->lru_len_dirty_metadata >= UDF_READWRITE_LINE_LENGTH*2) || udf_bufcache->flushall) {
							UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);
								/* signal there is time/space ahead and writeout buffer */
								pthread_cond_signal(&udf_bufcache->processed_signal);
								udf_writeout_file_buffer(buf_entry->b_vp, "dirty metadata buf", UDF_C_FIDS, buf_entry);
DEBUG(printf("+"); fflush(stdout));
							UDF_MUTEX_LOCK(&udf_bufcache->bufcache_lock);
						}
					}

					/* if there are too many, drop least used */
					if (udf_bufcache->lru_len_metadata >= UDF_LRU_METADATA_MIN) {
						TAILQ_FOREACH(buf_entry, &udf_bufcache->lru_bufs_metadata, b_lru) {
							/* process buf_entry */
							if ((buf_entry != marker) && ((buf_entry->b_flags & B_DIRTY)) == 0) {
								/* lock node bufs (locking protocol); don't drop metadata from `held' nodes */
								udf_node = buf_entry->b_vp;
								if (udf_node && ((!udf_node->hold) || udf_bufcache->flushall)) {
									UDF_MUTEX_LOCK(&udf_node->buf_mutex);
										udf_detach_buf_from_node(udf_node, buf_entry);
									UDF_MUTEX_UNLOCK(&udf_node->buf_mutex);
									udf_free_buf_entry(buf_entry);
	
									/* signal there is time/space ahead */
									UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);
										pthread_cond_signal(&udf_bufcache->processed_signal);
									UDF_MUTEX_LOCK(&udf_bufcache->bufcache_lock);
		
									break; /* mandatory leaving FOREACH */
								} else {
									if (!udf_node) {
										printf("\n\nWARNING: got a NULL udf_node freeing METAspace\n\n");
									}
#ifdef UDF_METADATA_LRU
									TAILQ_REMOVE(&udf_bufcache->lru_bufs_metadata, buf_entry, b_lru);
									TAILQ_INSERT_TAIL(&udf_bufcache->lru_bufs_metadata, buf_entry, b_lru);
#endif
								}
							}
						}
					}
				}
				TAILQ_REMOVE(&udf_bufcache->lru_bufs_metadata, marker, b_lru);
			}
			/* can only be used once */
			udf_bufcache->flushall = 0;

		UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);
		/* PURGE nodes?	(or only on request?)	*/

		/* if asked to quit break out of the loop */
		if (udf_bufcache->finish_purgethread) break;
	}

	UDF_VERBOSE(printf("\tbufcache thread joining\n"));
	pthread_exit(0);	/* join */

	/* not reached */
	return NULL;
}


int udf_start_unix_thread(void) {
	/*
	 * start up bufcache purger thread and crudely kick it into
	 * existence.
	 */

	if (udf_bufcache->thread_active) {
		fprintf(stderr,"\tlogvol bufcache thread asked to start again; ignoring\n");
		return 0;
	}

	DEBUG(printf("\tstarting logvol bufcache thread\n"));

	udf_bufcache->thread_active = 1;
	pthread_create(&udf_bufcache->purgethread_id, NULL, udf_purger, NULL);
	sleep(1);

	DEBUG(printf("\n\n"));
	return 0;
}


int udf_stop_unix_thread(void) {
	/* stop all associated threads */
	UDF_VERBOSE(printf("\tstopping bufcache thread\n"));

	if (udf_bufcache->thread_active) {
		udf_bufcache->purgethread_kicked = 1;
		udf_bufcache->finish_purgethread = 1;
		pthread_cond_signal(&udf_bufcache->purgethread_signal);
		pthread_join(udf_bufcache->purgethread_id, NULL);		/* wait for join */
	}

	udf_bufcache->thread_active = 0;
	return 0;
}


int udf_purgethread_kick(char *why) {
	/*
	 * Kick the cache purger into existence in case its not active and wait
	 * for it to signal there is space left.
	 */

	DEBUG(printf("\npurgethread kick! because of %s\n", why));
	udf_bufcache->purgethread_kicked = 1;
	pthread_cond_signal(&udf_bufcache->purgethread_signal);

	return 0;
}

/* end of udf_unix.c */

