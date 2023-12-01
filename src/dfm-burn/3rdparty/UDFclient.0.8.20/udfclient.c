/* $NetBSD$ */

/*
 * File "udfclient.c" is part of the UDFclient toolkit.
 * File $Id: udfclient.c,v 1.104 2016/04/25 21:28:00 reinoud Exp $ $Name:  $
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

/*
 * ChangeLog
 * - hidden `main` and `usage`
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include "udf.h"
#include "udf_bswap.h"

/* switches */

/* #define DEBUG(a) (a) */
#define DEBUG(a) if (0) { a; }


#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif


/* include timeval to timespec conversion macro's for systems that don't provide them */
#ifndef TIMEVAL_TO_TIMESPEC
#	define TIMEVAL_TO_TIMESPEC(tv, ts) do {	\
		(ts)->tv_sec = (tv)->tv_sec;		\
		(ts)->tv_nsec = (tv)->tv_usec * 1000;	\
	} while (/*CONSTCOND*/0)
#endif
#ifndef TIMESPEC_TO_TIMEVAL
#	define TIMESPEC_TO_TIMEVAL(tv, ts) do {	\
		(tv)->tv_sec = (ts)->tv_sec;		\
		(tv)->tv_usec = (ts)->tv_nsec / 1000;	\
	} while (/*CONSTCOND*/0)
#endif


/* include the dump parts ... in order to get a more sane splitting */
extern void udf_dump_alive_sets(void);


/* globals */
extern int udf_verbose;
extern int uscsilib_verbose;
int read_only;


#define MAX_ARGS 100


struct curdir {
	char			*name;
	struct udf_mountpoint	*mountpoint;			/* foreign */
	struct udf_node		*udf_node;			/* foreign */
	struct hash_entry	*udf_mountpoint_entry;		/* `current' mountpoint entry */
} curdir;



/*
 * XXX
 * FTP client; de volumes vooraan zetten in de VFS ; evt. in meerdere subdirs.
 * general file name format 
 *
 * /volset:pri:log/udfpath
 *     of
 * /volset/pri/log/udfpath/
 * 
 * XXX
 */

int udfclient_readdir(struct udf_node *udf_node, struct uio *result_uio, int *eof_res) {
	struct dirent          entry;
	struct udf_mountpoint *mountable;

	assert(result_uio);
	if (!udf_node) {
		/* mountables */
		/* XXX result_uio needs to be long enough to hold all mountables!!!! XXX */
		SLIST_FOREACH(mountable, &udf_mountables, all_next) {
			strcpy(entry.d_name, mountable->mount_name);
			entry.d_type = DT_DIR;
			uiomove(&entry, sizeof(struct dirent), result_uio);
		}
		if (eof_res) *eof_res = 1;
		return 0;
	}

	/* intree nodes : pass on to udf_readdir */
	return udf_readdir(udf_node, result_uio, eof_res);
}


/* VOP_LOOKUP a-like */
int udfclient_lookup(struct udf_node *dir_node, struct udf_node **resnode, char *name) {
	struct udf_mountpoint *mountable;
	struct fileid_desc    *fid;
	struct long_ad         udf_icbptr;
	int lb_size, found, error;

	assert(resnode);
	assert(name);
	*resnode = NULL;
	if (!dir_node) {
		/* mountables */
		SLIST_FOREACH(mountable, &udf_mountables, all_next) {
			if (strcmp(mountable->mount_name, name) == 0) {
				/* found `root' of a mountable */
				*resnode = mountable->rootdir_node;
				return 0;
			}
		}
		return ENOENT;
	}

	/* intree nodes : pass on to udf_lookup_name_in_dir */
	lb_size = dir_node->udf_log_vol->lb_size;
	fid = malloc(lb_size);
	assert(fid);

	error = udf_lookup_name_in_dir(dir_node, name, strlen(name), &udf_icbptr, fid, &found);
	if (!error) {
		error = ENOENT;
		if (found) 
			error = udf_readin_udf_node(dir_node, &udf_icbptr, fid, resnode);
	}

	free(fid);
	return error;
}


int udfclient_getattr(struct udf_node *udf_node, struct stat *stat) {
	int error;

	error = 0;
	if (udf_node) {
		error = udf_getattr(udf_node, stat);
		/* print? */
		if (error) fprintf(stderr, "Can't stat file\n");
	} else {
		/* dummy entry for `root' in VFS */
		stat->st_mode = 0744 | S_IFDIR;
		stat->st_size = 0;
		stat->st_uid  = 0;
		stat->st_gid  = 0;
	}

	return error;
}


/* higher level of lookup; walk tree */
/* results in a `held'/locked node upto `root' */
int udfclient_lookup_pathname(struct udf_node *cur_node, struct udf_node **res_node, char *restpath_given) {
	struct udf_node *sub_node;
	char     *restpath, *next_element, *slashpos, *pathpos;
	int       error;
	
	assert(restpath_given);
	restpath = strdup(restpath_given);

	/* start at root */
	*res_node = NULL;
	pathpos   = restpath;
	assert(*pathpos == '/');
	pathpos++;		/* strip leading '/' */

	next_element = pathpos;
	while (next_element && (strlen(next_element) > 0)) {
		/* determine next slash position */
		slashpos = strchr(next_element, '/');
		if (slashpos) *slashpos++ = 0;

		error = udfclient_lookup(cur_node, &sub_node, next_element);
		if (error) {
			free(restpath);
			return error;
		}

		/* advance */
		cur_node = sub_node;
		next_element = slashpos;
	} 
	/* we are at the end; return result */
	*res_node = cur_node;
	free(restpath);
	return 0;
}


char *udfclient_realpath(char *cur_path, char *relpath, char **leaf) {
	char *resultpath, *here, *pos;

	resultpath = calloc(1, sizeof(cur_path) + sizeof(relpath) +1024);
	assert(resultpath);

	strcpy(resultpath, "/");
	strcat(resultpath, cur_path);
	strcat(resultpath, "/");

	/* check if we are going back to `root' */
	if (relpath && *relpath == '/') {
		strcpy(resultpath, "");
	}
	strcat(resultpath, relpath);
	/* now clean up the resulting string by removing double slashes */
	here = resultpath;
	while (*here) {
		pos = here; while (strncmp(pos, "//", 2) == 0) pos++;
		if (pos != here) strcpy(here, pos);
		here++;
	}

	/* remove '.' and '..' sequences */
	here = resultpath;
	while (*here) {
		/* printf("transformed to %s\n", resultpath); */
		/* check for internal /./ and trailing /. */
		if (strncmp(here, "/./", 3) == 0) {
			strcpy(here+1, here + 3);
			continue;
		}
		if (strcmp(here, "/.")==0) {
			strcpy(here+1, here + 2);
			continue;
		}
		if (strncmp(here, "/../", 4) == 0) {
			strcpy(here+1, here + 4);
			/* go for the parent */
			pos = here-1; while (*pos && *pos != '/') pos--; pos++;
			strcpy(pos, here+1);
			here = pos;
			continue;
		}
		if (strcmp(here, "/..")==0) {
			strcpy(here+1, here + 3);
			/* go for the parent */
			pos = here-1; while (*pos && *pos != '/') pos--; pos++;
			strcpy(pos, here+1);
			here = pos;
			continue;
		}
		here++;
	}
	if (leaf) {
		/* find the leaf name */
		here = resultpath;
		while (*here) {
			if (strncmp(here, "/", 1) == 0)
				*leaf = here+1;
			here++;
		}
	}

	return resultpath;
}


static void print_dir_entry(struct udf_node *udf_node, char *name) {
	struct stat     stat;
	uint64_t        size;
	int   mode, this_mode, uid, gid;
	int   error;

	error = udfclient_getattr(udf_node, &stat);
	if (error) return;

	size = stat.st_size;
	mode = stat.st_mode;
	uid  = stat.st_uid;
	gid  = stat.st_gid;

	if (mode & S_IFDIR) printf("d"); else printf("-");
	mode = mode & 511;

	this_mode = (mode >> 6) & 7;
	printf("%c%c%c", "----rrrr"[this_mode & 4], "--www"[this_mode & 2], "-x"[this_mode & 1]);
	this_mode = (mode >> 3) & 7;
	printf("%c%c%c", "----rrrr"[this_mode & 4], "--www"[this_mode & 2], "-x"[this_mode & 1]);
	this_mode = mode & 7;
	printf("%c%c%c", "----rrrr"[this_mode & 4], "--www"[this_mode & 2], "-x"[this_mode & 1]);

	printf("  %5d  %5d  %10"PRIu64"  %s\n", gid, uid, size, name);
}


#define LS_SUBTREE_DIR_BUFFER_SIZE (16*1024)
void udfclient_ls(int args, char *arg1) {
	struct udf_node *udf_node, *entry_node;
	uint8_t       *buffer;
	struct uio     dir_uio;
	struct iovec   dir_uiovec;
	struct dirent *dirent;
	struct stat    stat;
	uint32_t       pos;
	int            eof;
	char          *node_name, *leaf_name;
	int            error;

	if (args > 1) {
		printf("Syntax: ls [file | dir]\n");
		return;
	}
	if (args == 0) arg1 = "";

	node_name = udfclient_realpath(curdir.name, arg1, &leaf_name);

	error = udfclient_lookup_pathname(NULL, &udf_node, node_name);
	if (error) {
		fprintf(stderr, "%s : %s\n", arg1, strerror(error));
		free(node_name); 
		return;
	}

	error = udfclient_getattr(udf_node, &stat);
	if (stat.st_mode & S_IFDIR) {
		printf("Directory listing of %s\n", udf_node ? leaf_name : "/");

		/* start at the start of the directory */
		dir_uio.uio_offset = 0;
		dir_uio.uio_iov     = &dir_uiovec;
		dir_uio.uio_iovcnt  = 1;
		buffer = calloc(1, LS_SUBTREE_DIR_BUFFER_SIZE);
		if (!buffer) return;

		do {
			dir_uiovec.iov_base = buffer;
			dir_uiovec.iov_len  = LS_SUBTREE_DIR_BUFFER_SIZE;
			dir_uio.uio_resid   = LS_SUBTREE_DIR_BUFFER_SIZE;
			dir_uio.uio_rw      = UIO_WRITE;

			error = udfclient_readdir(udf_node, &dir_uio, &eof);
			if (error) {
				fprintf(stderr, "error during readdir: %s\n", strerror(error));
				break;
			}
			pos = 0;
			while (pos < LS_SUBTREE_DIR_BUFFER_SIZE - dir_uio.uio_resid) {
				dirent = (struct dirent *) (buffer + pos);
				error = udfclient_lookup(udf_node, &entry_node, dirent->d_name);
				print_dir_entry(entry_node, dirent->d_name);

				pos += sizeof(struct dirent);
			}
		} while (!eof);
		free(buffer);
	} else {
		print_dir_entry(udf_node, leaf_name);
	}
	free(node_name);
}
#undef LS_SUBTREE_DIR_BUFFER_SIZE


void udfclient_pwd(int args) {
	char pwd[1024];
	char *res;

	if (args) {
		printf("Syntax: pwd\n");
		return;
	}
	res = getcwd(pwd, 1024);
	assert(res);
	printf("UDF working directory is     %s\n", curdir.name);
	printf("Current FS working directory %s\n", pwd);
}


static void udfclient_print_free_amount(char *prefix, uint64_t value, uint64_t max_value) {
	printf("%s %10"PRIu64" Kb (%3"PRIu64" %%) (%8.2f Mb) (%5.2f Gb)\n",
			prefix,  value/1024, (100*value)/max_value, (double) value/(1024*1024), (double) value/(1024*1024*1024));
}


void udfclient_free(int args) {
	struct udf_part_mapping *part_mapping;
	struct udf_partition	*udf_partition;
	struct udf_log_vol *udf_log_vol;
	struct logvol_desc *lvd;
	uint64_t part_size, unalloc_space, freed_space;
	uint64_t total_space, free_space, await_alloc_space;
	uint32_t lb_size;
	int part_num;

	if (args) {
		printf("Syntax: free\n");
		return;
	}

	if (!curdir.udf_node || !(udf_log_vol = curdir.udf_node->udf_log_vol)) {
		printf("Can only report free space in UDF mountpoints\n");
		return;
	}

	lb_size = udf_log_vol->lb_size;
	// sector_size = udf_log_vol->sector_size;

	lvd = udf_log_vol->log_vol;
	udf_dump_id("Logical volume ", 128, lvd->logvol_id, &lvd->desc_charset);

	total_space       = udf_log_vol->total_space;
	free_space        = udf_log_vol->free_space;
	await_alloc_space = udf_log_vol->await_alloc_space;

	SLIST_FOREACH(part_mapping, &udf_log_vol->part_mappings, next_mapping) {
		part_num = part_mapping->udf_virt_part_num;
		udf_logvol_vpart_to_partition(udf_log_vol, part_num, NULL, &udf_partition);
		assert(udf_partition);

		unalloc_space = udf_partition->free_unalloc_space;
		freed_space   = udf_partition->free_freed_space;
		part_size     = udf_partition->part_length;

		switch (part_mapping->udf_part_mapping_type) {
			case UDF_PART_MAPPING_PHYSICAL :
				printf("\tphysical partition %d\n", part_num);
				printf("\t\t%8"PRIu64" K (%"PRIu64" pages) size\n", part_size/1024, part_size / lb_size);
				printf("\t\t%8"PRIu64" K (%"PRIu64" pages) unallocated\n", unalloc_space/1024, unalloc_space / lb_size);
				printf("\t\t%8"PRIu64" K (%"PRIu64" pages) freed\n", freed_space/1024, freed_space / lb_size);
				break;
			case UDF_PART_MAPPING_VIRTUAL  :
				printf("\tvirtual partition mapping %d\n", part_num);
				printf("\t\tnot applicable\n");
				break;
			case UDF_PART_MAPPING_SPARABLE :
				printf("\tsparable partition %d\n", part_num);
				printf("\t\t%8"PRIu64" K (%"PRIu64" pages) size\n", part_size/1024, part_size / lb_size);
				printf("\t\t%8"PRIu64" K (%"PRIu64" pages) unallocated\n", unalloc_space/1024, unalloc_space / lb_size);
				printf("\t\t%8"PRIu64" K (%"PRIu64" pages) freed\n", freed_space/1024, freed_space / lb_size);
				break;
			case UDF_PART_MAPPING_META     :
				printf("\tmetadata 'partition' %d\n", part_num);
				printf("\t\t%8"PRIu64" K (%"PRIu64" pages) unallocated\n", unalloc_space/1024, unalloc_space / lb_size);
				printf("\t\t%8"PRIu64" K (%"PRIu64" pages) freed\n", freed_space/1024, freed_space / lb_size);
				break;
			case UDF_PART_MAPPING_ERROR    :
				printf("\terror partiton %d\n", part_num);
				break;
			default:
				break;
		}
	}
	printf("\n");
	udfclient_print_free_amount("\tConfirmed free space ", free_space,        total_space); 
	udfclient_print_free_amount("\tAwaiting allocation  ", await_alloc_space, total_space);
	udfclient_print_free_amount("\tEstimated free space ", free_space - await_alloc_space, total_space);
	udfclient_print_free_amount("\tEstimated total used ", total_space - free_space + await_alloc_space, total_space);
	printf("\n");
	udfclient_print_free_amount("\tTotal size           ", total_space, total_space);
}


void udfclient_cd(int args, char *arg1) {
	struct udf_node *udf_node;
	struct stat stat;
	char *node_name, *new_curdir_name;
	int   error;

	if (args > 1) {
		printf("Syntax: cd [dir]\n");
		return;
	}

	new_curdir_name = udfclient_realpath(curdir.name, arg1, NULL);

	node_name = strdup(new_curdir_name);	/* working copy */
	error = udfclient_lookup_pathname(NULL, &udf_node, node_name);
	if (error) {
		fprintf(stderr, "%s : %s\n", arg1, strerror(error));
		free(new_curdir_name);
		free(node_name); 
		return;
	}

	error = udfclient_getattr(udf_node, &stat);
	if (stat.st_mode & S_IFDIR) {
		free(curdir.name);
		curdir.name     = new_curdir_name;
		curdir.udf_node = udf_node;
		free(node_name);

		udfclient_pwd(0);
	} else {
		fprintf(stderr, "%s is not a directory\n", arg1);
		free(new_curdir_name);
		free(node_name);
	}
}


void udfclient_lcd(int args, char *arg1) {
	char pwd[1024];
	char *res;

	if (args > 1) {
		printf("Syntax: lcd [dir]\n");
		return;
	}

	if (strcmp(arg1, "" )==0) arg1 = getenv("HOME");
	if (strcmp(arg1, "~")==0) arg1 = getenv("HOME");

	if (chdir(arg1)) {
		fprintf(stderr, "While trying to go to %s :", arg1);
		perror("");
	}
	res = getcwd(pwd, 1024);
	assert(res);
	printf("Changing local directory to %s\n", pwd);
}


void udfclient_lls(int args) {
	if (args) {
		printf("Syntax: lls\n");
		return;
	}
	if (system("/bin/ls")) {
		perror("While listing current directory\n");
	}
}


uint64_t getmtime(void) {
	struct timeval tp;

	gettimeofday(&tp, NULL);
	return 1000000*tp.tv_sec + tp.tv_usec;
}



int udfclient_get_file(struct udf_node *udf_node, char *fullsrcname, char *fulldstname) {
	struct uio	 file_uio;
	struct iovec	 file_iov;
	struct stat	 stat;
	struct timeval	 times[2];
	uint64_t	 file_length;
	uint64_t 	 start, now, then, eta;
	uint64_t	 cur_speed, avg_speed, data_transfered;
	uint64_t	 file_block_size, file_transfer_size, written;
	uint8_t		*file_block;
	char		 cur_txt[32], avg_txt[32], eta_txt[32];
	int		 fileh, len;
	int		 notok, error;

	assert(udf_node);
	assert(fullsrcname);
	assert(strlen(fullsrcname) >= 1);

	error = 0;

	/* terminal directory node? */
	error = udfclient_getattr(udf_node, &stat);
	if (stat.st_mode & S_IFDIR) {
		len = strlen(fulldstname);
		if (strcmp(fulldstname + len -2, "/.")  == 0)
			fulldstname[len-2] = 0;
		if (strcmp(fulldstname + len -3, "/..") == 0)
			return 0;

		error = mkdir(fulldstname, (udf_node->stat.st_mode) & 07777);
		if (!error) {
			/* set owner attribute and times; access permissions allready set on creation.*/
			notok = chown(fulldstname, stat.st_uid, stat.st_gid);
			if (notok && (udf_verbose > UDF_VERBLEV_ACTIONS))
				fprintf(stderr, "failed to set owner of directory, ignoring\n");

			TIMESPEC_TO_TIMEVAL(&times[0], &stat.st_atimespec);	/* access time		*/
			TIMESPEC_TO_TIMEVAL(&times[1], &stat.st_mtimespec);	/* modification time	*/
			notok = utimes(fulldstname, times);
			if (notok)
				fprintf(stderr, "failed to set times on directory, ignoring\n");
		}
		if (error)
			fprintf(stderr, "While creating directory `%s' : %s\n", fulldstname, strerror(errno));

		return 0;
	}

	/* terminal file node; setting mode correctly */
	fileh = open(fulldstname, O_WRONLY | O_CREAT | O_TRUNC, udf_node->stat.st_mode);
	if (fileh >= 0) {
		file_length = udf_node->stat.st_size;
		file_block_size = 256*1024;		/* block read in length */
		file_block = malloc(file_block_size);
		if (!file_block) {
			printf("Out of memory claiming file buffer\n");
			return ENOMEM;
		}

		/* move to uio_newuio(struct uio *uio) with fixed length uio_iovcnt? */
		bzero(&file_uio, sizeof(struct uio));
		file_uio.uio_rw     = UIO_WRITE;	/* WRITE into this space */
		file_uio.uio_iovcnt = 1;
		file_uio.uio_iov    = &file_iov;

		start = getmtime();
		then  = now = start;
		eta = data_transfered = 0;
		strcpy(avg_txt, "---"); strcpy(cur_txt, "---"); strcpy(eta_txt, "---");

		file_uio.uio_offset = 0;		/* begin at the start */
		do {
			/* fill in IO vector space; reuse blob file_block over and over */
			file_transfer_size = MIN(file_block_size, file_length - file_uio.uio_offset);
			file_uio.uio_resid  = file_transfer_size;
			file_uio.uio_iov->iov_base = file_block;
			file_uio.uio_iov->iov_len  = file_block_size;

			error = udf_read_file_part_uio(udf_node, fullsrcname, UDF_C_USERDATA, &file_uio);
			if (error) {
				fprintf(stderr, "While retrieving file block : %s\n", strerror(error));
				printf("\n\n\n");	/* XXX */
				break;
			}

			written = write(fileh, file_block, file_transfer_size);
			assert(written == file_transfer_size);

			if ((getmtime() - now > 1000000) || ((uint64_t) file_uio.uio_offset >= file_length)) {
				if (strlen(fulldstname) < 45) {
					printf("\r%-45s ", fulldstname);
				} else {
					printf("\r...%-42s ", fulldstname + strlen(fulldstname)-42);
				}
				printf("%10"PRIu64" / %10"PRIu64" bytes ", (uint64_t) file_uio.uio_offset, (uint64_t) file_length);
				if (file_length) printf("(%3d%%) ", (int) (100.0*(float) file_uio.uio_offset / file_length));

				then = now;
				now  = getmtime();
				cur_speed = 0;
				avg_speed = 0;
				if (now-start > 0) avg_speed = (1000000 * file_uio.uio_offset) / (now-start);
				if (now-then  > 0) cur_speed = (1000000 * (file_uio.uio_offset - data_transfered)) / (now-then);
				if (avg_speed > 0) eta = (file_length - file_uio.uio_offset) / avg_speed;
				data_transfered = file_uio.uio_offset;

				strcpy(avg_txt, "---"); strcpy(cur_txt, "---"); strcpy(eta_txt, "---");
				if (avg_speed > 0) sprintf(avg_txt, "%d", (int32_t) avg_speed/1000);
				if (cur_speed > 0) sprintf(cur_txt, "%d", (int32_t) cur_speed/1000);
				if (eta       > 0) sprintf(eta_txt, "%02d:%02d:%02d", (int) (eta/3600), (int) (eta/60) % 60, (int) eta % 60);

				printf("%6s KB/s (%6s KB/s) ETA %s", avg_txt, cur_txt, eta_txt);
				fflush(stdout);
			}
		} while ((uint64_t) file_uio.uio_offset < file_length);
		printf(" finished\n");
		free(file_block);

		/* set owner attribute and times; access permissions allready set on creation.*/
		notok = fchown(fileh, stat.st_uid, stat.st_gid);
		if (notok && (udf_verbose > UDF_VERBLEV_ACTIONS))
			fprintf(stderr, "failed to set owner of file, ignoring\n");

		TIMESPEC_TO_TIMEVAL(&times[0], &stat.st_atimespec);	/* access time		*/
		TIMESPEC_TO_TIMEVAL(&times[1], &stat.st_mtimespec);	/* modification time	*/
		notok = futimes(fileh, times);
		if (notok)
			fprintf(stderr, "failed to set times on directory, ignoring\n");

		close(fileh);
	} else {
		printf("Help! can't open file %s for output\n", fulldstname);
	}

	return error;
}


#define GET_SUBTREE_DIR_BUFFER_SIZE (16*1024)
void udfclient_get_subtree(struct udf_node *udf_node, char *srcprefix, char *dstprefix, int recurse, uint64_t *total_size) {
	struct uio          dir_uio;
	struct iovec        dir_iovec;
	uint8_t            *buffer;
	uint32_t            pos;
	char                fullsrcpath[1024], fulldstpath[1024];	/* XXX arbitrary length XXX */
	struct dirent      *dirent;
	struct stat         stat;
	struct udf_node    *entry_node;
	struct fileid_desc *fid;
	struct long_ad      udf_icbptr;
	int                 lb_size, eof;
	int                 found, isdot, isdotdot, error;

	if (!udf_node)
		return;

	udf_node->hold++;
	error = udfclient_getattr(udf_node, &stat);
	if ((stat.st_mode & S_IFDIR) && recurse) {
		buffer = malloc(GET_SUBTREE_DIR_BUFFER_SIZE);
		if (!buffer) {
			udf_node->hold--;
			return;
		}

		lb_size = udf_node->udf_log_vol->lb_size;
		fid = malloc(lb_size);
		assert(fid);

		/* recurse into this directory */
		dir_uio.uio_offset = 0;			/* begin at start */
		do {
			dir_iovec.iov_base = buffer;
			dir_iovec.iov_len  = GET_SUBTREE_DIR_BUFFER_SIZE;
			dir_uio.uio_resid  = GET_SUBTREE_DIR_BUFFER_SIZE;
			dir_uio.uio_iovcnt = 1;
			dir_uio.uio_iov    = &dir_iovec;
			dir_uio.uio_rw     = UIO_WRITE;

			error = udf_readdir(udf_node, &dir_uio, &eof);
			pos = 0;
			while (pos < GET_SUBTREE_DIR_BUFFER_SIZE - dir_uio.uio_resid) {
				dirent = (struct dirent *) (buffer + pos);

				sprintf(fullsrcpath, "%s/%s", srcprefix, dirent->d_name);
				sprintf(fulldstpath, "%s/%s", dstprefix, dirent->d_name);

				/* looking for '.' or '..' ? */
				isdot    = (strcmp(dirent->d_name, "." ) == 0);
				isdotdot = (strcmp(dirent->d_name, "..") == 0);

				pos += sizeof(struct dirent);	/* XXX variable size dirents possible XXX */

				if (isdotdot)
					continue;

				if (isdot) {
					/* hack */
					udfclient_get_subtree(udf_node, fullsrcpath, fulldstpath, 0, total_size);
					continue;
				}

				error = udf_lookup_name_in_dir(udf_node, dirent->d_name, DIRENT_NAMLEN(dirent), &udf_icbptr, fid, &found);
				if (!error) {
					error = ENOENT;
					if (found) 
						error = udf_readin_udf_node(udf_node, &udf_icbptr, fid, &entry_node);
				}

				if (!error)
					udfclient_get_subtree(entry_node, fullsrcpath, fulldstpath, 1, total_size);

			}
		} while (!eof);

		udf_node->hold--;
		free(buffer);
		free(fid);
		return;
	}

	/* leaf node : prefix is complete name but with `/' prefix */
	if (*srcprefix == '/') srcprefix++;
	error = udfclient_get_file(udf_node, srcprefix, dstprefix);
	udf_node->hold--;
	if (!error)
		*total_size += udf_node->stat.st_size;
}
#undef GET_SUBTREE_DIR_BUFFER_SIZE


void udfclient_get(int args, char *arg1, char *arg2) {
	struct udf_node *udf_node;
	char  *source_name, *target_name;
	uint64_t start, now, totalsize, avg_speed;
	int   error;

	if (args > 2) {
		printf("Syntax: get remote [local]\n");
		return;
	}

	source_name = arg1;
	target_name = arg1;
	if (args == 2)
		target_name = arg2;

	/* source name gets substituted */
	source_name = udfclient_realpath(curdir.name, source_name, NULL);
	DEBUG(printf("Attempting to retrieve %s\n", source_name));

	error = udfclient_lookup_pathname(NULL, &udf_node, source_name);
	if (error) {
		fprintf(stderr, "%s : %s\n", arg1, strerror(error));
		free(source_name); 
		return;
	}

	/* get the file/dir tree */
	totalsize = 0;
	start = getmtime();
		udfclient_get_subtree(udf_node, source_name, target_name, 1, &totalsize);
	now  = getmtime();
	if (now-start > 0) {
		avg_speed = (1000000 * totalsize) / (now-start);
		printf("A total of %d kb transfered at an overal average of %d kb/sec\n", (uint32_t) (totalsize/1024), (uint32_t) (avg_speed/1024));
	} else {
		printf("Transfered %d kb\n", (uint32_t)(totalsize/1024));
	}

	/* bugalert: not releasing target_name for its not substituted */
	free(source_name);
}



void udfclient_mget(int args, char *argv[]) {
	struct udf_node *udf_node;
	uint64_t start, now, totalsize, avg_speed;
	char *node_name, *source_name, *target_name;
	int   arg, error;

	if (args == 0) {
		printf("Syntax: mget (file | dir)*\n");
		return;
	}

	/* retrieve the series of file/dir trees and measure time/seed */
	totalsize = 0;
	start = getmtime();

	/* process all args */
	arg = 0;
	node_name = NULL;
	while (args) {
		source_name = target_name = argv[arg];

		node_name = udfclient_realpath(curdir.name, source_name, NULL);
		DEBUG(printf("Attempting to retrieve %s\n", node_name));
	
		error = udfclient_lookup_pathname(NULL, &udf_node, node_name);
		printf("%d: mget trying %s\n", error, node_name);
		if (!error) {
			udfclient_get_subtree(udf_node, source_name, target_name, 1, &totalsize);
		}

		if (node_name) {
			free(node_name);
			node_name = NULL;
		}

		if (error) break;	/* TODO continuation flag? */

		/* advance */
		arg++;
		args--;
	}

	now  = getmtime();
	if (now-start > 0) {
		avg_speed = (1000000 * totalsize) / (now-start);
		printf("A total of %d kb transfered at an overal average of %d kb/sec\n", (uint32_t) (totalsize/1024), (uint32_t) (avg_speed/1024));
	} else {
		printf("Transfered %d kb\n", (uint32_t) (totalsize/1024));
	}

	if (node_name)
		free(node_name);
}


int udfclient_put_file(struct udf_node *udf_node, char *fullsrcname, char *fulldstname) {
	struct uio	 file_uio;
	struct iovec	 file_iov;
	uint64_t	 file_length;
	uint64_t 	 start, now, then, eta;
	uint64_t	 cur_speed, avg_speed, data_transfered;
	uint64_t	 file_block_size, file_transfer_size;
	uint8_t		*file_block;
	char		 cur_txt[32], avg_txt[32], eta_txt[32];
	int		 fileh;
	int   		 error, printed;

	assert(udf_node);
	assert(fullsrcname);

	DEBUG(printf("Attempting to write %s\n", fullsrcname));

	fileh = open(fullsrcname, O_RDONLY, 0666);
	if (fileh == -1) {
		fprintf(stderr, "Can't open local file %s for reading: %s\n", fullsrcname, strerror(errno));
		return ENOENT;
	}

	/* get file length */
	file_length = lseek(fileh, 0, SEEK_END);
	lseek(fileh, 0, SEEK_SET);

	/* check if file will fit; give it a bit of slack space until the space issue is found and fixed */
	if (udf_node->udf_log_vol->free_space < file_length + udf_node->udf_log_vol->await_alloc_space + UDF_MINFREE_LOGVOL) {
		return ENOSPC;
	}

	/* allocate file block to transfer file with */
	file_block_size = 128*1024;
	file_block = malloc(file_block_size);
	if (!file_block) {
		fprintf(stderr, "Out of memory claiming file buffer\n");
		return ENOMEM;
	}

	/* move to uio_newuio(struct uio *uio) with fixed length uio_iovcnt? */
	bzero(&file_uio, sizeof(struct uio));
	file_uio.uio_rw     = UIO_READ;			/* READ from this space */
	file_uio.uio_iovcnt = 1;
	file_uio.uio_iov    = &file_iov;

/* ------------ */
	start = getmtime();
	then  = now = start;
	eta = data_transfered = 0;
	printed = 0;
	strcpy(avg_txt, "---"); strcpy(cur_txt, "---"); strcpy(eta_txt, "---");
/* ------------ */

	error = 0;
	error = udf_truncate_node(udf_node, 0);
	while (!error && ((uint64_t) file_uio.uio_offset < file_length)) {
		file_transfer_size = MIN(file_block_size, file_length - file_uio.uio_offset);

		error = read(fileh, file_block, file_transfer_size);
		if (error<0) {
			fprintf(stderr, "While reading in file block for writing : %s\n", strerror(errno));
			error = errno;
			break;
		}

		file_uio.uio_resid  = file_transfer_size;
		file_uio.uio_iov->iov_base = file_block;
		file_uio.uio_iov->iov_len  = file_block_size;

		error = udf_write_file_part_uio(udf_node, fullsrcname, UDF_C_USERDATA, &file_uio);
		if (error) {
			fprintf(stderr, "\nError while writing file : %s\n", strerror(error));
			break;
		}

/* ------------ */
		if ((getmtime() - now > 1000000) || ((uint64_t) file_uio.uio_offset >= file_length)) {
			printed = 1;
			if (strlen(fulldstname) < 45) {
				printf("\r%-45s ", fulldstname);
			} else {
				printf("\r...%-42s ", fulldstname+strlen(fulldstname)-42);
			}
			printf("%10"PRIu64" / %10"PRIu64" bytes ", (uint64_t) file_uio.uio_offset, (uint64_t) file_length);
			if (file_length) printf("(%3d%%) ", (int) (100.0*(float) file_uio.uio_offset / file_length));

			then = now;
			now  = getmtime();
			cur_speed = 0;
			avg_speed = 0;
			if (now-start > 0) avg_speed = (1000000 * file_uio.uio_offset) / (now-start);
			if (now-then  > 0) cur_speed = (1000000 * (file_uio.uio_offset - data_transfered)) / (now-then);
			if (avg_speed > 0) eta = (file_length - file_uio.uio_offset) / avg_speed;
			data_transfered = file_uio.uio_offset;

			strcpy(avg_txt, "---"); strcpy(cur_txt, "---"); strcpy(eta_txt, "---");
			if (avg_speed > 0) sprintf(avg_txt, "%d", (int32_t) avg_speed/1024);
			if (cur_speed > 0) sprintf(cur_txt, "%d", (int32_t) cur_speed/1024);
			if (eta       > 0) sprintf(eta_txt, "%02d:%02d:%02d", (int) (eta/3600), (int) (eta/60) % 60, (int) eta % 60);
			printf("%6s KB/s (%6s KB/s) ETA %s", avg_txt, cur_txt, eta_txt);
			fflush(stdout);
		}
/* ------------ */
	}
	if (!error && printed) printf(" finished\n");

	close(fileh);
	free(file_block);

	return error;
}


int udfclient_put_subtree(struct udf_node *parent_node, char *srcprefix, char *srcname, char *dstprefix, char *dstname, uint64_t *totalsize) {
	struct udf_node *file_node, *dir_node;
	struct dirent *dirent;
	struct stat    stat;
	DIR           *dir;
	char           fullsrcpath[1024], fulldstpath[1024];
	int            error;

	sprintf(fullsrcpath, "%s/%s", srcprefix, srcname);
	sprintf(fulldstpath, "%s/%s", dstprefix, dstname);

	/* stat source file */
	bzero(&stat, sizeof(struct stat));
	error = lstat(fullsrcpath, &stat);
	if (error) {
		error = errno;	/* lstat symantics; returns -1 on error */
		fprintf(stderr, "Can't stat file/dir \"%s\"! : %s\n", fullsrcpath, strerror(error));
		return error;
	}

	/* test if `srcname' refers to a directory */
	dir = opendir(fullsrcpath);
	if (dir) {
		error = udfclient_lookup(parent_node, &dir_node, dstname);
		if (error) {
			DEBUG(printf("Create dir %s on UDF\n", fulldstpath));
			error = udf_create_directory(parent_node, dstname, &stat, &dir_node);
			if (error) {
				closedir(dir);
				fprintf(stderr, "UDF: couldn't create new directory %s : %s\n", dstname, strerror(error));
				return error;
			}
		}

		dir_node->hold++;
		dirent = readdir(dir);
		while (dirent) {
			if (strcmp(dirent->d_name, ".") && strcmp(dirent->d_name, "..")) {
				/* skip `.' and ',,' */
				error = udfclient_put_subtree(dir_node, fullsrcpath, dirent->d_name, fulldstpath, dirent->d_name, totalsize);
				if (error) break;
			}
			dirent = readdir(dir);
		}
		closedir(dir);
		dir_node->hold--;
		return error;
	}

	/* leaf node : prefix is complete name but with `/' prefix */
	DEBUG(printf("Put leaf: %s\n", fulldstpath));

	error = udfclient_lookup(parent_node, &file_node, dstname);
	if (!file_node) {
		error = udf_create_file(parent_node, dstname, &stat, &file_node);
		if (error) {
			fprintf(stderr, "UDF: couldn't add new file entry in directory %s for %s : %s\n", dstprefix, dstname, strerror(error));
			return error;
		}
	}
	file_node->hold++;
		error = udfclient_put_file(file_node, fullsrcpath, fulldstpath);
	file_node->hold--;

	if (error) fprintf(stderr, "UDF: Couldn't write file %s : %s\n", fulldstpath, strerror(error));
	if (error) udf_remove_file(parent_node, file_node, dstname);

	if (!error) *totalsize += file_node->stat.st_size;

	return error;
}


void udfclient_put(int args, char *arg1, char *arg2) {
	struct udf_node *curdir_node;
	uint64_t start, now, totalsize, avg_speed;
	char *source_name, *target_name;
	int   error;

	if (args > 2) {
		printf("Syntax: put source [destination]\n");
		return;
	}

	if (read_only) {
		printf("Modifying this filingsystem is prevented; use -W flag to enable writing on your own risk!\n");
		return;
	}

	error = udfclient_lookup_pathname(NULL, &curdir_node, curdir.name);
	if (error) {
		printf("Current directory not found?\n");
		return;
	}
	DEBUG(printf("Attempting to copy %s\n", arg1));


	/* determine source and destination entities */
	source_name = arg1;
	target_name = arg1;
	if (args == 2)
		target_name = arg2;

	/* writeout file/dir tree and measure the time/speed */
	totalsize = 0;
	start = getmtime();
		error = udfclient_put_subtree(curdir_node, ".", source_name, ".", target_name, &totalsize);
	now  = getmtime();
	if (now-start > 0) {
		avg_speed = (1000000 * totalsize) / (now-start);
		printf("A total of %d kb transfered at an overal average of %d kb/sec\n", (uint32_t) (totalsize/1024), (uint32_t) (avg_speed/1024));
	} else {
		printf("Transfered %d kb\n", (uint32_t) (totalsize/1024));
	}
}


/* args start at position 0 of argv */
void udfclient_mput(int args, char **argv) {
	struct udf_node *curdir_node;
	uint64_t start, now, totalsize, avg_speed;
	char *source_name, *target_name;
	int   arg, error;

	if (args == 0) {
		printf("Syntax: mput (file | dir)*\n");
		return;
	}

	if (read_only) {
		printf("Modifying this filingsystem is prevented; use -W flag to enable writing on your own risk!\n");
		return;
	}

	error = udfclient_lookup_pathname(NULL, &curdir_node, curdir.name);
	if (error) {
		printf("Current directory not found?\n");
		return;
	}

	/* writeout file/dir trees and measure the time/speed */
	totalsize = 0;
	start = getmtime();

	/* process all args */
	arg = 0;
	while (args) {
		source_name = target_name = argv[arg];
		error = udfclient_put_subtree(curdir_node, ".", source_name, ".", target_name, &totalsize);
		if (error) {
			fprintf(stderr, "While writing file %s : %s\n", source_name, strerror(error));
			break;	/* TODO continuation flag? */
		}

		/* advance */
		arg++;
		args--;
	}

	now  = getmtime();
	if (now-start > 0) {
		avg_speed = (1000000 * totalsize) / (now-start);
		printf("A total of %d kb transfered at an overal average of %d kb/sec\n", (uint32_t)(totalsize/1024), (uint32_t)(avg_speed/1024));
	} else {
		printf("Transfered %d kb\n", (uint32_t)(totalsize/1024));
	}

}



void udfclient_trunc(int args, char *arg1, char *arg2) {
	struct udf_node *udf_node;
	char		*node_name;
	uint64_t	 length;
	int		 error;

	if (args != 2) {
		printf("Syntax: trunc file length\n");
		return;
	}
	length = strtoll(arg2, NULL, 10);

	node_name = udfclient_realpath(curdir.name, arg1, NULL);
	error = udfclient_lookup_pathname(NULL, &udf_node, node_name);
	if (error || !udf_node) {
		printf("Can only truncate existing file!\n");
		free(node_name);
		return;
	}

	udf_truncate_node(udf_node, length);

	free(node_name);
}


void udfclient_sync(void) {
	struct udf_discinfo *udf_disc;

	SLIST_FOREACH(udf_disc, &udf_discs_list, next_disc) {
		udf_sync_disc(udf_disc);
	}
}


#define RM_SUBTREE_DIR_BUFFER_SIZE (32*1024)
int udfclient_rm_subtree(struct udf_node *parent_node, struct udf_node *dir_node, char *name, char *full_parent_name) {
	struct uio          dir_uio;
	struct iovec        dir_iovec;
	uint8_t            *buffer;
	uint32_t            pos;
	char               *fullpath;
	struct dirent      *dirent;
	struct stat         stat;
	struct udf_node    *entry_node;
	struct fileid_desc *fid;
	struct long_ad      udf_icbptr;
	int                 lb_size, eof, found, isdot, isdotdot;
	int                 error;

	if (!dir_node)
		return ENOENT;

	error = udfclient_getattr(dir_node, &stat);
	if (stat.st_mode & S_IFDIR) {
		buffer = malloc(RM_SUBTREE_DIR_BUFFER_SIZE);
		if (!buffer)
			return ENOSPC;
		lb_size = dir_node->udf_log_vol->lb_size;
		fid = malloc(lb_size);
		if (!fid) {
			free(buffer);
			return ENOSPC;
		}

		/* recurse into this directory */
		dir_uio.uio_offset = 0;			/* begin at start */
		do {
			dir_iovec.iov_base = buffer;
			dir_iovec.iov_len  = RM_SUBTREE_DIR_BUFFER_SIZE;
			dir_uio.uio_resid  = RM_SUBTREE_DIR_BUFFER_SIZE;
			dir_uio.uio_iovcnt = 1;
			dir_uio.uio_iov    = &dir_iovec;
			dir_uio.uio_rw     = UIO_WRITE;

			error = udf_readdir(dir_node, &dir_uio, &eof);
			pos = 0;
			while (pos < RM_SUBTREE_DIR_BUFFER_SIZE - dir_uio.uio_resid) {
				dirent = (struct dirent *) (buffer + pos);
				pos += sizeof(struct dirent);	/* XXX variable size dirents possible XXX */

				/* looking for '.' or '..' ? */
				isdot    = (strcmp(dirent->d_name, "." ) == 0);
				isdotdot = (strcmp(dirent->d_name, "..") == 0);

				if (isdot || isdotdot)
					continue;

				error = udf_lookup_name_in_dir(dir_node, dirent->d_name, DIRENT_NAMLEN(dirent), &udf_icbptr, fid, &found);
				if (!error) {
					error = ENOENT;
					if (found) 
						error = udf_readin_udf_node(dir_node, &udf_icbptr, fid, &entry_node);
				}
				if (error)
					break;

				error = udfclient_getattr(entry_node, &stat);
				if (error)
					break;

				/* check if the direntry is a directory or a file */
				if (stat.st_mode & S_IFDIR) {
					fullpath = malloc(strlen(full_parent_name) + strlen(dirent->d_name)+2);
					if (fullpath) {
						sprintf(fullpath, "%s/%s", full_parent_name, dirent->d_name);
						error = udfclient_rm_subtree(dir_node, entry_node, dirent->d_name, fullpath);
					} else {
						error = ENOMEM;
					}
					free(fullpath);
				} else {
					error = udf_remove_file(dir_node, entry_node, dirent->d_name);
					if (!error)
						printf("rm %s/%s\n", full_parent_name, dirent->d_name);
				}
				if (error)
					break;
			}
		} while (!eof);

		free(buffer);
		free(fid);

		/* leaving directory -> delete directory itself */
		if (!error) {
			error = udf_remove_directory(parent_node, dir_node, name);
			if (!error) printf("rmdir %s/%s\n", full_parent_name, name);
		}
		return error;
	}

	return ENOTDIR;
}
#undef RM_SUBTREE_DIR_BUFFER_SIZE



void udfclient_rm(int args, char *argv[]) {
	struct udf_node *remove_node, *parent_node;
	struct stat stat;
	char  *target_name, *leaf_name, *full_parent_name;
	int   error, len, arg;

	if (args == 0) {
		printf("Syntax: rm (file | dir)*\n");
		return;
	}

	/* process all args; effectively multiplying an `rm' command */
	arg = 0;
	while (args) {
		leaf_name = argv[arg];

		/* lookup node; target_name gets substituted */
		target_name = udfclient_realpath(curdir.name, leaf_name, &leaf_name);
		error = udfclient_lookup_pathname(NULL, &remove_node, target_name);
		if (error || !remove_node) {
			printf("rm %s : %s\n", target_name, strerror(error));
			free(target_name);
			return;		/* TODO continuation flag */
			/* continue; */
		}
	
		full_parent_name = udfclient_realpath(target_name, "..", NULL);
		error = udfclient_lookup_pathname(NULL, &parent_node, full_parent_name);
		if (error || !parent_node) {
			printf("rm %s : parent lookup failed : %s\n", target_name, strerror(error));
			free(target_name);
			free(full_parent_name);
			return;		/* TODO continuation flag */
			/* continue; */
		}
	
		error = udfclient_getattr(remove_node, &stat);
		if (!error) {
			if (stat.st_mode & S_IFDIR) {
				len = strlen(target_name);
				if (target_name[len-1] == '/') target_name[len-1] = '\0';
				error = udfclient_rm_subtree(parent_node, remove_node, leaf_name, target_name);
			} else {
				error = udf_remove_file(parent_node, remove_node, leaf_name);
				if (!error) printf("rm %s/%s\n", full_parent_name, leaf_name);
			}
		}
		if (error)
			fprintf(stderr, "While removing file/dir : %s\n", strerror(error));

		free(target_name);
		free(full_parent_name);

		if (error)
			break;		/* TODO continuation flag */

		/* advance */
		arg++;
		args--;
	}
}


void udfclient_mv(int args, char *from, char *to) {
	struct udf_node *rename_me, *present, *old_parent, *new_parent;
	char *rename_from_name, *rename_to_name, *old_parent_name, *new_parent_name;
	int error;

	if (args != 2) {
		printf("Syntax: mv source destination\n");
		return;
	}

	/* `from' gets substituted by its leaf name */
	rename_from_name = udfclient_realpath(curdir.name, from, &from);
	error = udfclient_lookup_pathname(NULL, &rename_me, rename_from_name);
	if (error || !rename_me) {
		printf("Can't find file/dir to be renamed\n");
		free(rename_from_name);
		return;
	}

	old_parent_name = udfclient_realpath(rename_from_name, "..", NULL);
	error = udfclient_lookup_pathname(NULL, &old_parent, old_parent_name);
	if (error || !old_parent) {
		printf("Can't determine rootdir of renamed file?\n");
		free(rename_from_name);
		free(old_parent_name);
		return;
	}

	/* `to' gets substituted by its leaf name */
	rename_to_name = udfclient_realpath(curdir.name, to, &to);
	udfclient_lookup_pathname(NULL, &present, rename_to_name);
	new_parent_name = udfclient_realpath(rename_to_name, "..", NULL);
	error = udfclient_lookup_pathname(NULL, &new_parent, new_parent_name);
	if (error || !new_parent) {
		printf("Can't determine rootdir of destination\n");
		free(rename_from_name);
		free(rename_to_name);
		free(old_parent_name);
		free(new_parent_name);
		return;
	}

	error = udf_rename(old_parent, rename_me, from, new_parent, present, to);
	if (error) {
		printf("Can't move file or directory: %s\n", strerror(error));
		return;
	}

	free(rename_from_name);
	free(rename_to_name);
	free(old_parent_name);
	free(new_parent_name);
}


void udfclient_mkdir(int args, char *arg1) {
	struct stat stat;
	struct udf_node	*udf_node, *parent_node;
	char  *full_create_name, *dirname, *parent_name;
	int error;

	if (args != 1) {
		printf("Syntax: mkdir dir\n");
		return;
	}

	/* get full name of dir to be created */
	full_create_name = udfclient_realpath(curdir.name,      arg1, &dirname);
	parent_name      = udfclient_realpath(full_create_name, "..", NULL);
	error = udfclient_lookup_pathname(NULL, &parent_node, parent_name);
	if (error || !parent_node) {
		printf("Can't determine directory the new directory needs to be created in %d <%s> <%s> <%s>\n", error, parent_name, full_create_name, curdir.name);
		free(full_create_name);
		free(parent_name);
		return;
	}

	bzero(&stat, sizeof(struct stat));
	stat.st_uid  = UINT_MAX;
	stat.st_gid  = UINT_MAX;
	stat.st_mode = 0755 | S_IFDIR;		/* don't forget this! */

	error = udf_create_directory(parent_node, dirname, &stat, &udf_node);
	if (error) {
		printf("Can't create directory %s : %s\n", arg1, strerror(error));
	}

	free(full_create_name);
	free(parent_name);
}


/* `line' gets more and more messed up in the proces */
char *udfclient_get_one_arg(char *line, char **result) {
	unsigned char chr, limiter;
	char *end_arg;

	*result = NULL;

	/* get prepending whitespace */
	while (*line && (*line <= ' ')) line++;

	chr= '\0';
	limiter = ' ';
	if (*line == '"') {
		line++;
		limiter = '"';
	}

	*result = line;

	while (*line) {
		chr = *line;
		if (chr && (chr < ' ')) chr = ' ';
		if (chr == 0 || chr == limiter) {
			break;
		} else {
			*line = chr;
		}
		line++;
	}
	end_arg = line;

	if (chr == limiter) line++;

	/* get appended whitespace */
	while (*line && (*line <= ' ')) line++;

	*end_arg = 0;

	return line;
}


int udfclient_get_args(char *line, char *argv[]) {
	int arg, args;

	for (arg = 0; arg < MAX_ARGS+1; arg++) {
		argv[arg] = "";
	}

	/* get all arguments */
	args = 0;
	while (args < MAX_ARGS+1) {
		line = udfclient_get_one_arg(line, &argv[args]);
		args++;
		if (!*line) {
			return args;
		}
	}

	printf("UDFclient implementation limit: too many arguments\n");
	return 0;
}


void udfclient_interact(void) {
	int  args, params;
	char *cmd;
	char *argv[MAX_ARGS+1];
	char line[4096];

	udfclient_pwd(0);
	while (1) {
		printf("UDF> ");
		clearerr(stdin);

		*line = 0;
		(void) fgets(line, 4096, stdin);

		if ((*line == 0) && feof(stdin)) {
			printf("quit\n");
			return;
		}

		args = udfclient_get_args(line, argv);
		cmd = argv[0];

		params = args -1;
		if (args) {
			if (strcmp(cmd, "")==0) continue;

			if (strcmp(cmd, "ls")==0) {
				udfclient_ls(params, argv[1]);
				continue;
			}
			if (strcmp(cmd, "cd")==0) {
				udfclient_cd(params, argv[1]);
				continue;
			}
			if (strcmp(cmd, "lcd")==0) {
				udfclient_lcd(params, argv[1]);
				continue;
			}
			if (strcmp(cmd, "lls")==0) {
				udfclient_lls(params);
				continue;
			}
			if (strcmp(cmd, "pwd")==0) {
				udfclient_pwd(params);
				continue;
			}
			if (strcmp(cmd, "free")==0) {
				udfclient_free(params);
				continue;
			}
			if (strcmp(cmd, "get")==0) {
				udfclient_get(params, argv[1], argv[2]);
				continue;
			}
			if (strcmp(cmd, "mget")==0) {
				udfclient_mget(params, argv + 1);
				continue;
			}
			if (strcmp(cmd, "put")==0) {
				/* can get destination file/dir (one day) */
				udfclient_put(params, argv[1], argv[2]);
				continue;
			}
			if (strcmp(cmd, "mput")==0) {
				udfclient_mput(params, argv + 1);
				continue;
			}
			if (strcmp(cmd, "trunc")==0) {
				udfclient_trunc(params, argv[1], argv[2]);
				continue;
			}
			if (strcmp(cmd, "mkdir")==0) {
				udfclient_mkdir(params, argv[1]);
				continue;
			}
			if (strcmp(cmd, "rm")==0) {
				udfclient_rm(params, argv + 1);
				continue;
			}
			if (strcmp(cmd, "mv")==0) {
				udfclient_mv(params, argv[1], argv[2]);
				continue;
			}
			if (strcmp(cmd, "sync")==0) {
				udfclient_sync();
				continue;
			}
			if (strcmp(cmd, "help")==0) {
				printf("Selected commands available (use \" pair for filenames with spaces) :\n"
						"ls  [file | dir]\tlists the UDF directory\n"
						"cd  [dir]\t\tchange current UDF directory\n"
						"lcd [dir]\t\tchange current directory\n"
						"lls\t\t\tlists current directory\n"
						"pwd\t\t\tdisplay current directories\n"
						"free\t\t\tdisplay free space on disc\n"
						"get  source [dest]\tretrieve a file / directory from disc\n"
						"mget (file | dir)*\tretrieve set of files / directories\n"
						"put  source [dest]\twrite a file / directory to disc\n"
						"mput (file | dir)*\twrite a set of files / directories\n"
						"trunc file length\ttrunc file to length\n"
						"mkdir dir\t\tcreate directory\n"
						"rm  (file | dir)*\tdelete set of files / directories\n"
						"mv  source dest\t\trename a file (limited)\n"
						"sync\t\t\tsync filingsystem\n"
						"quit\t\t\texits program\n"
						"exit\t\t\talias for quit\n"
				      );
				continue;
			}
			if (strcmp(cmd, "quit")==0 ||
			    strcmp(cmd, "exit")==0) {
				return;
			}
			printf("\nUnknown command %s\n", cmd);
		}
	}
}

#if 0
int usage(char *program) {
	fprintf(stderr, "Usage: %s [options] devicename [devicename]*)\n", program);
	fprintf(stderr, "-u level	UDF system verbose level\n"
			"-r range	use only selected sessions like -3,5,7 or 6-\n"
			"-W		allow writing (temporary flag)\n"
			"-F		force mount writable when marked dirty (use with cause)\n"
			"-b blocksize	use alternative sectorsize; use only on files/discs\n"
			"-D		debug/verbose SCSI command errors\n"
			"-s		byteswap read sectors (for PVRs)\n"
	);
	return 1;
}


extern char	*optarg;
extern int	 optind;
extern int	 optreset;


int main(int argc, char *argv[]) {
	struct udf_discinfo *disc, *next_disc;
	uint32_t alt_sector_size;
	char *progname, *range;
	int   flag, mnt_flags;
	int   error;

	progname = argv[0];
	if (argc == 1) {
		return usage(progname);
	}

	/* be a bit more verbose */
	udf_verbose	= UDF_VERBLEV_ACTIONS;
	uscsilib_verbose= 0;
	mnt_flags       = UDF_MNT_RDONLY;
	range		= NULL;
	alt_sector_size = 0;
	while ((flag = getopt(argc, argv, "u:Dr:WFb:s")) != -1) {
		switch (flag) {
			case 'u' :
				udf_verbose = atoi(optarg);
				break;
			case 'D' :
				uscsilib_verbose = 1;
				break;
			case 'r' :
				range = strdup(optarg);
				if (udf_check_session_range(range)) {
					fprintf(stderr, "Invalid range %s\n", range);
					return usage(progname);
				}
				break;
			case 'W' :
				mnt_flags &= ~UDF_MNT_RDONLY;
				break;
			case 'F' :
				mnt_flags |= UDF_MNT_FORCE;
				break;
			case 'b' :
				alt_sector_size = atoi(optarg);
				break;
			case 's' :
				mnt_flags |= UDF_MNT_BSWAP;
				break;
			default  :
				return usage(progname);
		}
	}
	argv += optind;
	argc -= optind;

	if (argc == 0) return usage(progname);

	if (!(mnt_flags & UDF_MNT_RDONLY)) {
		printf("--------------------------------\n");
		printf("WARNING: writing enabled, use on own risc\n");
		printf("\t* DONT cancel program or data-loss might occure\n");
		printf("\t* set dataspace userlimits very high when writing thousands of files\n");
		printf("\nEnjoy your writing!\n");
		printf("--------------------------------\n\n\n");
		printf("%c", 7); fflush(stdout); sleep(1); printf("%c", 7); fflush(stdout); sleep(1); printf("%c", 7); fflush(stdout);
	}

	/* all other arguments are devices */
	udf_init();
	while (argc) {
		printf("Opening device %s\n", *argv);
		error = udf_mount_disc(*argv, range, alt_sector_size, mnt_flags, &disc);
		if (error) {
			fprintf(stderr, "Can't open my device; bailing out : %s\n", strerror(error));
			exit(1);
		}
		if (read_only) disc->recordable = 0;
		if (read_only) disc->rewritable = 0;

		argv++; argc--;
		if (udf_verbose) printf("\n\n");
	}

	printf("\n");
	printf("Resulting list of alive sets :\n\n");
	udf_dump_alive_sets();

	/* interactive part */
	bzero(&curdir, sizeof(struct curdir));
	curdir.mountpoint = NULL;
	curdir.name = strdup("/");
	udfclient_ls(0, "");

	udfclient_interact();

	/* close part */
	printf("Closing discs\n");
	disc = SLIST_FIRST(&udf_discs_list);
	while (disc) {
		next_disc = SLIST_NEXT(disc, next_disc);

		udf_dismount_disc(disc);

		disc = next_disc;
	}

	return 0;
}
#endif
