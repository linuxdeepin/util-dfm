/* $NetBSD$ */

/*
 * File "udf.c" is part of the UDFclient toolkit.
 * File $Id: udf.c,v 1.307 2022/04/22 15:25:29 reinoud Exp $ $Name:  $
 *
 * Copyright (c) 2003, 2004, 2005, 2006, 2011, 2020
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
#include "dirhash.h"
#include <pthread.h>


/* for scsilib */
const char *dvname="UDF device";


#ifndef MAX
#	define MAX(a,b) ((a)>(b)?(a):(b))
#	define MIN(a,b) ((a)<(b)?(a):(b))
#endif


/* #define DEBUG(a) { a; } */
#define DEBUG(a) if (0) { a; }


/* global settings */
int     udf_verbose = UDF_VERBLEV_ACTIONS;

/* static structures shared over all programs */
struct discslist       udf_discs_list;
struct volumeset_list  udf_volumeset_list;
struct mountables_list udf_mountables;

#define UDF_INODE_NUM_GUESS  2048


/* predefines */
int  udf_validate_tag_and_crc_sums(union dscrptr *dscr);
void udf_node_mark_dirty(struct udf_node *udf_node);

static void udf_set_imp_id(struct regid *regid);
static void udf_set_app_id(struct regid *regid);
static void udf_node_unmark_dirty(struct udf_node *udf_node);
static void udf_init_desc_tag(struct desc_tag *tag, uint16_t id, uint16_t dscr_ver, uint16_t serial_num);
static int  udf_translate_icb_filetype_to_dirent_filetype(int udf_filetype);
static int  udf_remove_directory_prim(struct udf_node *dir_node, struct udf_node *udf_node, char *componentname);
static int  udf_remove_directory_entry(struct udf_node *dir_node, struct udf_node *udf_node, char *componentname);


/* external dumpers */
extern void udf_dump_descriptor(union dscrptr *dscrpt);
extern void udf_dump_file_entry(struct file_entry *fe);
extern void udf_dump_extfile_entry(struct extfile_entry *efe);
extern void udf_dump_alloc_extent(struct alloc_ext_entry *ext, int addr_type);
extern void udf_dump_vat_table(struct udf_part_mapping *udf_part_mapping);
extern void udf_dump_disc_anchors(struct udf_discinfo *disc);
extern void udf_dump_alive_sets(void);
extern void udf_dump_root_dir(struct udf_mountpoint *mountpoint);
extern void udf_dump_timestamp(char *dscr, struct timestamp *t);


/******************************************************************************************
 *
 * Filename space conversion
 *
 ******************************************************************************************/

void udf_to_unix_name(char *result, char *id, int len, struct charspec *chsp) {
    uint16_t  raw_name[1024], unix_name[1024];
    uint16_t *inchp, ch;
    uint8_t	 *outchp;
    int       ucode_chars, nice_uchars;

    assert(sizeof(char) == sizeof(uint8_t));
    outchp = (uint8_t *) result;
    if ((chsp->type == 0) && (strcmp((char*) chsp->inf, "OSTA Compressed Unicode") == 0)) {
        *raw_name = *unix_name = 0;
        ucode_chars = udf_UncompressUnicode(len, (uint8_t *) id, raw_name);
        ucode_chars = UnicodeLength((unicode_t *) raw_name, ucode_chars);
        nice_uchars = UDFTransName(unix_name, raw_name, ucode_chars);
        for (inchp = unix_name; nice_uchars>0; inchp++, nice_uchars--) {
            ch = *inchp;
            /* sloppy unicode -> latin */
            *outchp++ = ch & 255;
            if (!ch) break;
        }
        *outchp++ = 0;
    } else {
        /* assume 8bit char length byte latin-1 */
        assert(*id == 8);
        strncpy(result, id+1, len);
    }
}


void unix_to_udf_name(char *result, char *name, uint8_t *result_len, struct charspec *chsp) {
    uint16_t  raw_name[1024];
    int       udf_chars, name_len;
    char     *inchp;
    uint16_t *outchp;

    /* convert latin-1 or whatever to unicode-16 */
    *raw_name = 0;
    name_len  = 0;
    inchp  = name;
    outchp = raw_name;
    while (*inchp) {
        *outchp++ = (uint16_t) (*inchp++);
        name_len++;
    }

    if ((chsp->type == 0) && (strcmp((char *) chsp->inf, "OSTA Compressed Unicode") == 0)) {
        udf_chars = udf_CompressUnicode(name_len, 8, (unicode_t *) raw_name, (byte *) result);
    } else {
        /* assume 8bit char length byte latin-1 */
        *result++ = 8; udf_chars = 1;
        strcpy(result, name + 1);
        udf_chars += strlen(name);
    }
    *result_len = udf_chars;
}


static char *udf_get_compound_name(struct udf_mountpoint *mountpoint) {
    static char         compound[128+128+32+32+1];
    struct charspec    *charspec;
    struct udf_log_vol *udf_log_vol;
    struct udf_pri_vol *udf_pri_vol;
    char               *unix_name;

    udf_log_vol = mountpoint->udf_log_vol;
    udf_pri_vol = udf_log_vol->primary;

    charspec = &udf_pri_vol->pri_vol->desc_charset;
    assert(charspec->type == 0);
    assert(strcmp((const char *) charspec->inf, "OSTA Compressed Unicode")==0);

    unix_name = compound;

    udf_to_unix_name(unix_name, udf_pri_vol->pri_vol->volset_id, 128, charspec);
    strcat(unix_name, ":");
    unix_name += strlen(unix_name);

    udf_to_unix_name(unix_name, udf_pri_vol->pri_vol->vol_id, 32, charspec);
    strcat(unix_name, ":");
    unix_name += strlen(unix_name);

    udf_to_unix_name(unix_name, udf_log_vol->log_vol->logvol_id, 128, charspec);
    strcat(unix_name, ":");
    unix_name += strlen(unix_name);

    udf_to_unix_name(unix_name, mountpoint->fileset_desc->fileset_id, 32, charspec);

    return compound;
}


/******************************************************************************************
 *
 * Dump helpers for printing out information during parse
 *
 ******************************************************************************************/

void udf_dump_long_ad(char *prefix, struct long_ad *adr) {
    printf("%s at sector %d within partion space %d for %d bytes\n", prefix,
            udf_rw32(adr->loc.lb_num), udf_rw16(adr->loc.part_num),
            udf_rw32(adr->len)
            );
}


void udf_dump_id(char *prefix, int len, char *id, struct charspec *chsp) {
    uint16_t  raw_name[1024];
    uint16_t *pos, ch;
    int       ucode_chars;

    if (prefix) printf("%s ", prefix);
    if ((chsp->type == 0) && (strcmp((char *) chsp->inf, "OSTA Compressed Unicode") == 0)) {
        /* print the identifier using the OSTA compressed unicode */
        printf("`");
        ucode_chars = udf_UncompressUnicode(len, (uint8_t *) id, raw_name);
        for (pos = raw_name; ucode_chars > 0; pos++, ucode_chars--) {
            ch = *pos;		/* OSTA code decompresses to machine endian */
            if (!ch) break;
            if ((ch < 32) || (ch > 255)) {
                printf("[%d]", ch);
            } else {
                printf("%c", ch & 255);
            }
        }
        printf("`");
    } else {
        printf("(roughly) `%s`", id+1);
    }
    if (prefix) printf("\n");
}


void udf_dump_volume_name(char *prefix, struct udf_log_vol *udf_log_vol) {
    if (prefix) printf("%s%s", prefix, udf_log_vol->primary->udf_session->session_offset?" (local) ":" ");
    udf_dump_id(NULL, 128, udf_log_vol->primary->pri_vol->volset_id, &udf_log_vol->primary->pri_vol->desc_charset);
    printf(":");
    udf_dump_id(NULL,  32, udf_log_vol->primary->pri_vol->vol_id, &udf_log_vol->primary->pri_vol->desc_charset);
    printf(":");
    udf_dump_id(NULL, 128, udf_log_vol->log_vol->logvol_id, &udf_log_vol->log_vol->desc_charset);
    if (prefix) printf("\n");
}


/******************************************************************************************
 *
 * UDF tag checkers and size calculator
 *
 ******************************************************************************************/


/* not used extensively enough yet */
int udf_check_tag(union dscrptr *dscr) {
    struct desc_tag *tag = &dscr->tag;
    uint8_t *pos, sum, cnt;

    /* check TAG header checksum */
    pos = (uint8_t *) tag;
    sum = 0;

    for(cnt = 0; cnt < 16; cnt++) {
        if (cnt != 4) sum += *pos;
        pos++;
    }
    if (sum != tag->cksum) {
        /* bad tag header checksum; this is not a valid tag */
        DEBUG(printf("Bad checksum\n"));
        return EINVAL;
    }
    return 0;
}


int udf_check_tag_payload(union dscrptr *dscr) {
    struct desc_tag *tag = &dscr->tag;
    uint16_t crc;

    /* check payload CRC if applicable */
    if (udf_rw16(tag->desc_crc_len) == 0) return 0;

    crc = udf_cksum(((uint8_t *) tag) + UDF_DESC_TAG_LENGTH, udf_rw16(tag->desc_crc_len));
    if (crc != udf_rw16(tag->desc_crc)) {
        DEBUG(printf("ERROR: CRC bad read 0x%0x calc 0x%0x\n", udf_rw16(tag->desc_crc), crc));
        /* bad payload CRC; this is a broken tag */
        return EINVAL;
    }

    return 0;
}


int udf_validate_tag_sum(union dscrptr *dscr) {
    struct desc_tag *tag = &dscr->tag;
    uint8_t *pos, sum, cnt;

    /* calculate TAG header checksum */
    pos = (uint8_t *) tag;
    sum = 0;

    for(cnt = 0; cnt < 16; cnt++) {
        if (cnt != 4) sum += *pos;
        pos++;
    }
    tag->cksum = sum;	/* 8 bit */

    return 0;
}


/* assumes sector number of descriptor to be allready present */
int udf_validate_tag_and_crc_sums(union dscrptr *dscr) {
    struct desc_tag *tag = &dscr->tag;
    uint16_t crc;

    /* check payload CRC if applicable */
    if (udf_rw16(tag->desc_crc_len) > 0) {
        crc = udf_cksum(((uint8_t *) tag) + UDF_DESC_TAG_LENGTH, udf_rw16(tag->desc_crc_len));
        tag->desc_crc = udf_rw16(crc);
    }

    /* calculate TAG header checksum */
    return udf_validate_tag_sum(dscr);
}


int udf_check_tag_presence(union dscrptr *dscr, int TAG) {
    struct desc_tag *tag = &dscr->tag;
    int error;

    error = udf_check_tag(dscr);
    if (error) return error;

    if (udf_rw16(tag->id) != TAG) {
        DEBUG(fprintf(stderr, "looking for tag %d but found %d\n", TAG, udf_rw16(tag->id)));
        return ENOENT;
    }

    return 0;
}



/*
 * for malloc() purposes ... rather have an upperlimit than an exact size
 */
uint64_t udf_calc_tag_malloc_size(union dscrptr *dscr, uint32_t udf_sector_size) {
    uint32_t size, tag_id;

    tag_id = udf_rw16(dscr->tag.id);

    switch (tag_id) {
        case TAGID_LOGVOL :
            size  = sizeof(struct logvol_desc) - 1;					/* maps[1]		*/
            size += udf_rw32(dscr->lvd.mt_l);
            break;
        case TAGID_UNALLOC_SPACE :
            size  = sizeof(struct unalloc_sp_desc) - sizeof(struct extent_ad);	/* alloc_desc[1]	*/
            size += udf_rw32(dscr->usd.alloc_desc_num) * sizeof(struct extent_ad);
            break;
        case TAGID_FID :
            size = UDF_FID_SIZE + dscr->fid.l_fi + udf_rw16(dscr->fid.l_iu);
            size = (size + 3) & ~3;
            return size;		/* RETURN !! */
        case TAGID_LOGVOL_INTEGRITY :
            size  = sizeof(struct logvol_int_desc) - sizeof(uint32_t);		/* tables[1]		*/
            size += udf_rw32(dscr->lvid.l_iu);
            size += (2 * udf_rw32(dscr->lvid.num_part) * sizeof(uint32_t));
            break;
        case TAGID_SPACE_BITMAP :
            size  = sizeof(struct space_bitmap_desc) - 1;				/* data[1]		*/
            size += udf_rw32(dscr->sbd.num_bytes);
            break;
        case TAGID_SPARING_TABLE :
            size  = sizeof(struct udf_sparing_table) - sizeof(struct spare_map_entry);	/* entries[1]	*/
            size += udf_rw16(dscr->spt.rt_l) * sizeof(struct spare_map_entry);
            break;
        case TAGID_FENTRY :
            size  = sizeof(struct file_entry);
            size += udf_rw32(dscr->fe.l_ea) + udf_rw32(dscr->fe.l_ad)-1;		/* data[0] 		*/
            break;
        case TAGID_EXTFENTRY :
            size  = sizeof(struct extfile_entry);
            size += udf_rw32(dscr->efe.l_ea) + udf_rw32(dscr->efe.l_ad)-1;		/* data[0]		*/
            break;
        case TAGID_FSD :
            size  = sizeof(struct fileset_desc);
            break;
        default :
            size = sizeof(union dscrptr);
            break;
    }

    if ((size == 0) || (udf_sector_size == 0)) return 0;
    return ((size + udf_sector_size -1) / udf_sector_size) * udf_sector_size;
}


/* explicit only for FID's */
static int
udf_fidsize(struct fileid_desc *fid)
{
    int size;

    size = UDF_FID_SIZE + fid->l_fi + udf_rw16(fid->l_iu);
    size = (size + 3) & ~3;

    return size;
}


/******************************************************************************************
 *
 * Logical to physical adres transformation
 *
 ******************************************************************************************/


/* convert (udf_log_vol, vpart_num) to a udf_partion structure */
int udf_logvol_vpart_to_partition(struct udf_log_vol *udf_log_vol, uint32_t vpart_num, struct udf_part_mapping **udf_part_mapping_ptr, struct udf_partition **udf_partition_ptr) {
    struct udf_volumeset	 *udf_volumeset;
    struct udf_partition	 *udf_partition;
    struct udf_part_mapping  *udf_part_mapping;
    uint32_t		  part_num;
    int found;

    assert(udf_log_vol);
    assert(!SLIST_EMPTY(&udf_log_vol->part_mappings));

    /* clear result */
    if (udf_part_mapping_ptr) *udf_part_mapping_ptr = NULL;
    if (udf_partition_ptr)    *udf_partition_ptr    = NULL;

    /* map the requested partition map to the physical udf partition */
    found = 0;
    SLIST_FOREACH(udf_part_mapping, &udf_log_vol->part_mappings, next_mapping) {
        if (udf_part_mapping->udf_virt_part_num == vpart_num) {
            found = 1;
            break;
        }
    }
    if (!found) {
        printf("\t\t\tVirtual partition number %d not found!\n", vpart_num);
        return EINVAL;
    }

    assert(udf_part_mapping);	/* call me paranoid */
    part_num = udf_part_mapping->udf_phys_part_num;

    /* search for the physical partition information */
    udf_volumeset = udf_log_vol->primary->volumeset;
    SLIST_FOREACH(udf_partition, &udf_volumeset->parts, next_partition) {
        if (udf_rw16(udf_partition->partition->part_num) == part_num) break;
    }

    if (!udf_partition) {
        printf("\t\t\tNo information known about partition %d yet!\n", part_num);
        printf("\t\t\t\tPlease insert volume %d of this volumeset and try again\n", udf_part_mapping->vol_seq_num);
        return ENOENT;
    }

    if (udf_part_mapping_ptr) *udf_part_mapping_ptr = udf_part_mapping;
    if (udf_partition_ptr)    *udf_partition_ptr    = udf_partition;
    return 0;
}


/* no recursive translations yet (UDF OK) */
/* All translation is done in 64 bits to prevent bitrot and returns the PARTITION relative address */
int udf_vpartoff_to_sessionoff(struct udf_log_vol *udf_log_vol, struct udf_part_mapping *udf_part_mapping, struct udf_partition *udf_partition, uint64_t offset, uint64_t *ses_off, uint64_t *trans_valid_len) {
    struct spare_map_entry	 *sp_entry;
    struct udf_node		 *udf_node;
    struct udf_allocentry	 *alloc_entry;

    uint64_t	part_start, part_length;
    uint64_t	eff_sector, eff_offset;
    uint64_t	trans_sector;
    uint64_t	cur_offset;
    uint32_t	len, lb_num, block_offset;
    uint32_t	entry, entries;
    uint32_t	sector_size, lb_size;

    uint64_t	packet_num, packet_rlb;
    uint64_t	packet_len;

    uint32_t	vat_entries, *vat_pos;
    int		flags;

    assert(udf_part_mapping);
    assert(udf_partition);
    assert(ses_off);
    assert(trans_valid_len);

    /* not ok, but rather this than a dangling random value */
    *ses_off         = UINT_MAX;
    *trans_valid_len = 0;

    lb_size     = udf_log_vol->lb_size;
    sector_size = udf_log_vol->sector_size;
    part_start  = (uint64_t) udf_rw32(udf_partition->partition->start_loc) * sector_size;
    part_length = (uint64_t) udf_rw32(udf_partition->partition->part_len)  * sector_size;

    /* get the offset (in bytes) in the partition and check its validity */
    if (offset >= part_length) {
        printf("\t\toffset %"PRIu64" is outside partition %d!\n", offset, udf_rw16(udf_partition->partition->part_num));
        return EFAULT;
    }

    /* do the address translations based on the partition mapping type */
    /* translation of virt/sparable etc. is assumed to be done in logical block sizes */
    switch (udf_part_mapping->udf_part_mapping_type) {
        case UDF_PART_MAPPING_PHYSICAL :
            /* nothing to be done; physical is logical */
            *ses_off	 = part_start  + offset;				/* 1:1 */
            *trans_valid_len = part_length - offset;				/* rest of partition */
            return 0;

        case UDF_PART_MAPPING_VIRTUAL :
            vat_entries = udf_part_mapping->vat_entries;
            vat_pos = (uint32_t *) udf_part_mapping->vat_translation;

            /* this translation is dependent on logical sector numbers */
            eff_sector = offset / lb_size;
            eff_offset = offset % lb_size;

            /* TODO check range for logical sector against VAT length */
            assert(eff_sector < vat_entries);
            trans_sector     = vat_pos[eff_sector];
            *ses_off         = part_start + (trans_sector * lb_size) + eff_offset;	/* trans sectors are in lb->lb ? */
            *trans_valid_len = lb_size - eff_offset;				/* maximum one logical sector */
            return 0;

        case UDF_PART_MAPPING_SPARABLE :
            /* this translation is dependent on logical sector numbers */
            *ses_off   = part_start + offset;					/* 1:1 */
            eff_sector = offset / lb_size;
            eff_offset = offset % lb_size;

            /* transform on packet-length base */
            packet_len = udf_rw16(udf_part_mapping->udf_pmap->pms.packet_len);	/* in lb */
            entries    = udf_rw16(udf_part_mapping->sparing_table->rt_l);

            packet_num = (eff_sector / packet_len) * packet_len;
            packet_rlb =  eff_sector % packet_len;					/* within packet */

            /* translate this packet; source is in partition, destination is absolute disc address */
            sp_entry = &udf_part_mapping->sparing_table->entries[0];
            for (entry = 0; entry < entries; entry++) {
                if (udf_rw32(sp_entry->org) - packet_num == 0) {
                    /* mappings contain absolute disc addresses, so no partition offsets please */
                    *ses_off = (uint64_t) (udf_rw32(sp_entry->map) + packet_rlb) * lb_size + eff_offset;
                    break;
                }
                sp_entry++;
            }
            *trans_valid_len = (packet_len - packet_rlb) * lb_size;			/* maximum one packet */
            return 0;

        case UDF_PART_MAPPING_META :
            /* We follow the allocation entries to calculate our offset */
            udf_node = udf_part_mapping->meta_file;
            assert(udf_node->addr_type != UDF_ICB_INTERN_ALLOC);

            /* find sector in the allocation space */
            UDF_MUTEX_LOCK(&udf_node->alloc_mutex);
            cur_offset = 0;
            TAILQ_FOREACH(alloc_entry, &udf_node->alloc_entries, next_alloc) {
                len       = alloc_entry->len;
                lb_num    = alloc_entry->lb_num;
                /* vpart_num = alloc_entry->vpart_num; */
                flags     = alloc_entry->flags;

                /* check overlap with this alloc entry */
                if (cur_offset + len > offset) {
                    assert(((offset - cur_offset) % lb_size) == 0);	/* ought to be on sector boundary */
                    if (flags != UDF_EXT_ALLOCATED)
                        break;
                    block_offset = offset - cur_offset;
                    *ses_off     = part_start + lb_num * lb_size + block_offset;	/* 1:1 within the block */
                    *trans_valid_len = len - block_offset;				/* rest of this chunk   */
                    UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);
                    return 0;
                }
                cur_offset += len;
            } /* FOREACH */
            UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);

            printf("\t\toffset %"PRIu64" is not translated within current metadata partition %d file descriptor!\n", offset, udf_rw16(udf_partition->partition->part_num));
            return EFAULT;
        case UDF_PART_MAPPING_ERROR :
        default :
            break;
    }
    printf("Unsupported or bad mapping %d; can't translate\n", udf_part_mapping->udf_part_mapping_type);

    return EFAULT;
}



/******************************************************************************************
 *
 * udf_node creator, destructor and syncer
 *
 ******************************************************************************************/


static mode_t udf_perm_to_unix_mode(uint32_t perm) {
    mode_t mode;

    mode  = ((perm & UDF_FENTRY_PERM_USER_MASK)      );
    mode |= ((perm & UDF_FENTRY_PERM_GRP_MASK  ) >> 2);
    mode |= ((perm & UDF_FENTRY_PERM_OWNER_MASK) >> 4);

    return mode;
}


static uint32_t unix_mode_to_udf_perm(mode_t mode) {
    uint32_t perm;

    perm  = ((mode & S_IRWXO)     );
    perm |= ((mode & S_IRWXG) << 2);
    perm |= ((mode & S_IRWXU) << 4);
    perm |= ((mode & S_IWOTH) << 3);
    perm |= ((mode & S_IWGRP) << 5);
    perm |= ((mode & S_IWUSR) << 7);

    return perm;
}


/*
 * Fill in timestamp structure based on clock_gettime(). Time is reported back as a time_t
 * accompanied with a nano second field.
 *
 * The husec, usec and csec could be relaxed in type.
 */
static void udf_timespec_to_timestamp(struct timespec *timespec, struct timestamp *timestamp) {
    struct tm tm;
    uint64_t husec, usec, csec;

    bzero(timestamp, sizeof(struct timestamp));
    gmtime_r(&timespec->tv_sec, &tm);

    /*
     * Time type and time zone : see ECMA 1/7.3, UDF 2., 2.1.4.1, 3.1.1.
     *
     * Lower 12 bits are two complement signed timezone offset if bit 12
     * (method 1) is clear. Otherwise if bit 12 is set, specify timezone
     * offset to -2047 i.e. unsigned `zero'
     */

    timestamp->type_tz	= udf_rw16((1<<12) + 0);	/* has to be method 1 for CUT/GMT */
    timestamp->year		= udf_rw16(tm.tm_year + 1900);
    timestamp->month	= tm.tm_mon + 1;		/* `tm' structure uses 0..11 for months */
    timestamp->day		= tm.tm_mday;
    timestamp->hour		= tm.tm_hour;
    timestamp->minute	= tm.tm_min;
    timestamp->second	= tm.tm_sec;

    usec   = (timespec->tv_nsec + 500) / 1000;	/* round (if possible)        */
    husec  =   usec / 100;
    usec  -=  husec * 100;				/* we only want 0-99 in usec  */
    csec   =  husec / 100;				/* we       get 0-99 in csec  */
    husec -=   csec * 100;				/* we only want 0-99 in husec */

    timestamp->centisec	= csec;
    timestamp->hund_usec	= husec;
    timestamp->usec		= usec;
}


void udf_set_timestamp_now(struct timestamp *timestamp) {
    struct timespec now;

    clock_gettime(CLOCK_REALTIME, &now);
    udf_timespec_to_timestamp(&now, timestamp);
}


/* implemented as a seperate function to allow tuning */
void udf_set_timespec_now(struct timespec *timespec) {
    clock_gettime(CLOCK_REALTIME, timespec);
}


int udf_insanetimespec(struct timespec *check) {
    struct timespec now;
    struct tm tm;

    gmtime_r(&check->tv_sec, &tm);

    /* since our converters can only deal with timestamps after 1970 we
     * reject earlier */
    if (tm.tm_year < 1970) return 1;

    /* don't accept values from the future; FFS or NFS might not mind, but
     * UDF does! */
    clock_gettime(CLOCK_REALTIME, &now);
    if (now.tv_sec < check->tv_sec)
        return 1;
    if ((now.tv_sec == check->tv_sec) && (now.tv_nsec < check->tv_nsec))
        return 1;

    return 0;
}


/*
 * Timestamp to timespec conversion code is taken with small modifications
 * from FreeBSD /sys/fs/udf by Scott Long <scottl@freebsd.org>
 */

static int mon_lens[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};


static int
udf_isaleapyear(int year)
{
    int i;

    i = (year % 4) ? 0 : 1;
    i &= (year % 100) ? 1 : 0;
    i |= (year % 400) ? 0 : 1;

    return i;
}


static void udf_timestamp_to_timespec(struct timestamp *timestamp, struct timespec *timespec) {
    uint32_t usecs, secs, nsecs;
    uint16_t tz;
    int i, lpyear, daysinyear, year;

    timespec->tv_sec  = secs  = 0;
    timespec->tv_nsec = nsecs = 0;

       /*
    * DirectCD seems to like using bogus year values.
    * Distrust time->month especially, since it will be used for an array
    * index.
    */
    year = udf_rw16(timestamp->year);
    if ((year < 1970) || (timestamp->month > 12)) {
        return;
    }

    /* Calculate the time and day */
    usecs = timestamp->usec + 100*timestamp->hund_usec + 10000*timestamp->centisec;
    nsecs = usecs * 1000;
    secs  = timestamp->second;
    secs += timestamp->minute * 60;
    secs += timestamp->hour * 3600;
    secs += (timestamp->day-1) * 3600 * 24;			/* day : 1-31 */

    /* Calclulate the month */
    lpyear = udf_isaleapyear(year);
    for (i = 1; i < timestamp->month; i++)
        secs += mon_lens[lpyear][i-1] * 3600 * 24;	/* month: 1-12 */

    for (i = 1970; i < year; i++) {
        daysinyear = udf_isaleapyear(i) + 365 ;
        secs += daysinyear * 3600 * 24;
    }

    /*
     * Calculate the time zone.  The timezone is 12 bit signed 2's
     * compliment, so we gotta do some extra magic to handle it right.
     */
    tz  = udf_rw16(timestamp->type_tz);
    tz &= 0x0fff;				/* only lower 12 bits are significant */
    if (tz & 0x0800)			/* sign extention */
        tz |= 0xf000;

    /* TODO check timezone conversion */
#if 1
    /* check if we are specified a timezone to convert */
    if (udf_rw16(timestamp->type_tz) & 0x1000)
        if ((int16_t) tz != -2047)
            secs -= (int16_t) tz * 60;
#endif
    timespec->tv_sec  = secs;
    timespec->tv_nsec = nsecs;
}


static void udf_node_get_fileinfo(struct udf_node *udf_node, union dscrptr *dscrptr) {
    struct stat          *stat;
    struct file_entry    *file_entry;
    struct extfile_entry *extfile_entry;
    struct timestamp     *atime, *mtime, *ctime, *attrtime;
    uint64_t inf_len, unique_id;
    uint32_t uid, gid, udf_perm;
    uint16_t fe_tag;
    uint16_t udf_icbtag_flags, serial_num, link_cnt;
    uint8_t  filetype;

    assert(udf_node);
    assert(dscrptr);
    stat   = &udf_node->stat;

    /* check if its an normal file entry or a extended file entry ICB */
    fe_tag = udf_rw16(dscrptr->tag.id);
    if (fe_tag == TAGID_FENTRY) {
        file_entry = &dscrptr->fe;
#if 0
        prev_direct_entries = udf_rw32(file_entry->icbtag.prev_num_dirs);
        strat_param16  = udf_rw16(* (uint16_t *) (file_entry->icbtag.strat_param));
        entries        = udf_rw16(file_entry->icbtag.max_num_entries);
        strategy       = udf_rw16(file_entry->icbtag.strat_type);
        data_length    = udf_rw32(file_entry->l_ad);
        addr_type      = udf_rw16(file_entry->icbtag.flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK;
        pos            = &file_entry->data[0] + udf_rw32(file_entry->l_ea);
#endif
        filetype         = file_entry->icbtag.file_type;
        inf_len          = udf_rw64(file_entry->inf_len);
        uid              = udf_rw32(file_entry->uid);
        gid              = udf_rw32(file_entry->gid);
        udf_perm         = udf_rw32(file_entry->perm);
        serial_num       = udf_rw16(file_entry->tag.serial_num);
        udf_icbtag_flags = udf_rw16(file_entry->icbtag.flags);
        link_cnt         = udf_rw16(file_entry->link_cnt);
        unique_id        = udf_rw64(file_entry->unique_id);
        atime            = &file_entry->atime;
        mtime            = &file_entry->mtime;
        ctime		 = &file_entry->mtime;		/* XXX assumption */
        attrtime         = &file_entry->attrtime;
    } else if (fe_tag == TAGID_EXTFENTRY) {
        extfile_entry = &dscrptr->efe;
#if 0
        prev_direct_entries = udf_rw32(extfile_entry->icbtag.prev_num_dirs);
        strat_param16  = udf_rw16(* (uint16_t *) (extfile_entry->icbtag.strat_param));
        entries        = udf_rw16(extfile_entry->icbtag.max_num_entries);
        strategy       = udf_rw16(extfile_entry->icbtag.strat_type);
        data_length    = udf_rw32(extfile_entry->l_ad);
        addr_type      = udf_rw16(extfile_entry->icbtag.flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK;
        pos            = &extfile_entry->data[0] + udf_rw32(extfile_entry->l_ea);
#endif
        filetype         = extfile_entry->icbtag.file_type;
        inf_len          = udf_rw64(extfile_entry->inf_len);
        uid              = udf_rw32(extfile_entry->uid);
        gid              = udf_rw32(extfile_entry->gid);
        udf_perm         = udf_rw32(extfile_entry->perm);
        serial_num       = udf_rw16(extfile_entry->tag.serial_num);
        udf_icbtag_flags = udf_rw16(extfile_entry->icbtag.flags);
        link_cnt         = udf_rw16(extfile_entry->link_cnt);	/* how many FID's are linked to this (ext)fentry	*/
        unique_id        = udf_rw64(extfile_entry->unique_id);	/* unique file ID					*/
        atime            = &extfile_entry->atime;
        mtime            = &extfile_entry->mtime;
        ctime		 = &extfile_entry->ctime;
        attrtime         = &extfile_entry->attrtime;
    } else {
        printf("udf_node_set_file_info : help! i can't be here!!! i got a %d tag\n", fe_tag);
        udf_dump_descriptor(dscrptr);
        return;
    }

    /* fill in (parts of) the stat structure */
    /* XXX important info missing still like access mode, times etc. XXX */
    udf_node->udf_filetype     = filetype;
    udf_node->serial_num       = serial_num;
    udf_node->udf_icbtag_flags = udf_icbtag_flags;
    udf_node->link_cnt         = link_cnt;		/* how many FID's are linked to this (ext)fentry	*/
    udf_node->unique_id        = unique_id;		/* unique file ID					*/

    /* fill in stat basics */
    bzero(stat, sizeof(struct stat));
    stat->st_ino     = unique_id;				/* lowest 32 bit(!) only */
    stat->st_mode    = udf_perm_to_unix_mode(udf_perm);	/* CONVERT from udf_perm */
    stat->st_mode   |= (udf_translate_icb_filetype_to_dirent_filetype(filetype) & DT_DIR) ? S_IFDIR : S_IFREG;
    stat->st_uid     = uid;
    stat->st_gid     = gid;

    /* ... times */
    udf_timestamp_to_timespec(atime,    &stat->st_atimespec);
    udf_timestamp_to_timespec(mtime,    &stat->st_mtimespec);
    udf_timestamp_to_timespec(attrtime, &stat->st_ctimespec);
#ifndef NO_STAT_BIRTHTIME
    udf_timestamp_to_timespec(ctime,    &stat->st_birthtimespec);
#endif

    /* ... sizes */
    stat->st_size    = inf_len;
    stat->st_blksize = udf_node->udf_log_vol->lb_size;

    /* special: updatables */
    stat->st_nlink   = link_cnt;
    stat->st_blocks  = (stat->st_size + 512 -1)/512;	/* blocks are hardcoded 512 bytes/sector in stat :-/ */
    return;
}


static void udf_node_set_fileinfo(struct udf_node *udf_node, union dscrptr *dscrptr) {
    struct stat          *stat;
    struct file_entry    *file_entry;
    struct extfile_entry *extfile_entry;
    struct timestamp     *atime, *mtime, *ctime, *attrtime;
    uint64_t inf_len, unique_id;
    uint32_t uid, gid, udf_perm;
    uint16_t fe_tag, serial_num, link_cnt;
    uint8_t  filetype;

    assert(udf_node);
    assert(dscrptr);
    stat   = &udf_node->stat;

    /* set (parts of) the stat structure */
    /* XXX important info missing still like times etc. XXX */
    uid      = stat->st_uid;
    gid      = stat->st_gid;
    inf_len  = stat->st_size;
    udf_perm = unix_mode_to_udf_perm(stat->st_mode);	/* conversion to UDF perm. */

    filetype   = udf_node->udf_filetype;
    unique_id  = udf_node->unique_id;
    serial_num = udf_node->serial_num;
    link_cnt   = udf_node->link_cnt;

    /* check if its to be written in an normal file entry or a extended file entry ICB */
    fe_tag = udf_rw16(dscrptr->tag.id);
    if (fe_tag == TAGID_FENTRY) {
        file_entry = &dscrptr->fe;
#if 0
        prev_direct_entries = udf_rw32(file_entry->icbtag.prev_num_dirs);
        strat_param16  = udf_rw16(* (uint16_t *) (file_entry->icbtag.strat_param));
        entries        = udf_rw16(file_entry->icbtag.max_num_entries);
        strategy       = udf_rw16(file_entry->icbtag.strat_type);
        data_length    = udf_rw32(file_entry->l_ad);
        addr_type      = udf_rw16(file_entry->icbtag.flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK;
        pos            = &file_entry->data[0] + udf_rw32(file_entry->l_ea);
#endif
        file_entry->icbtag.file_type = filetype;
        file_entry->inf_len         = udf_rw64(inf_len);
        file_entry->uid             = udf_rw32(uid);
        file_entry->gid             = udf_rw32(gid);
        file_entry->perm            = udf_rw32(udf_perm);
        file_entry->tag.serial_num  = udf_rw16(serial_num);
        file_entry->link_cnt        = udf_rw16(link_cnt);
        file_entry->unique_id       = udf_rw64(unique_id);
        atime                       = &file_entry->atime;
        mtime                       = &file_entry->mtime;
        ctime                       = mtime;			/* XXX assumption */
        attrtime                    = &file_entry->attrtime;
    } else if (fe_tag == TAGID_EXTFENTRY) {
        extfile_entry = &dscrptr->efe;
#if 0
        prev_direct_entries = udf_rw32(extfile_entry->icbtag.prev_num_dirs);
        strat_param16  = udf_rw16(* (uint16_t *) (extfile_entry->icbtag.strat_param));
        entries        = udf_rw16(extfile_entry->icbtag.max_num_entries);
        strategy       = udf_rw16(extfile_entry->icbtag.strat_type);
        data_length    = udf_rw32(extfile_entry->l_ad);
        addr_type      = udf_rw16(extfile_entry->icbtag.flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK;
        pos            = &extfile_entry->data[0] + udf_rw32(extfile_entry->l_ea);
#endif
        extfile_entry->icbtag.file_type = filetype;
        extfile_entry->inf_len        = udf_rw64(inf_len);
        extfile_entry->uid            = udf_rw32(uid);
        extfile_entry->gid            = udf_rw32(gid);
        extfile_entry->perm           = udf_rw32(udf_perm);
        extfile_entry->tag.serial_num = udf_rw16(serial_num);
        extfile_entry->link_cnt       = udf_rw16(link_cnt);	/* how many FID's are linked to this (ext)fentry	*/
        extfile_entry->unique_id      = udf_rw64(unique_id);	/* unique file ID					*/
        atime          = &extfile_entry->atime;
        mtime          = &extfile_entry->mtime;
        ctime          = &extfile_entry->ctime;
        attrtime       = &extfile_entry->attrtime;
    } else {
        printf("udf_node_set_file_info : help! i can't be here!!! i got a %d tag\n", fe_tag);
        udf_dump_descriptor(dscrptr);
        return;
    }
    /* FILL in {atime, mtime, attrtime} TIMES! */
    udf_timespec_to_timestamp(&stat->st_atimespec,     atime);
    udf_timespec_to_timestamp(&stat->st_mtimespec,     mtime);
    udf_timespec_to_timestamp(&stat->st_ctimespec,     attrtime);
#ifndef NO_STAT_BIRTHTIME
    udf_timespec_to_timestamp(&stat->st_birthtimespec, ctime);
#else
    memcpy(ctime, mtime, sizeof(*ctime));
#endif

    return;
}


/* with 32 bits ino_t, a maximum of about 8 TB discs are supported (1<<32) * 2KB*/
ino_t udf_calc_hash(struct long_ad *icbptr) {
    /* TODO unique file-id would be better */
    return (ino_t) udf_rw32(icbptr->loc.lb_num);
}


void udf_insert_node_in_hash(struct udf_node *udf_node) {
    struct udf_log_vol *log_vol;
    uint32_t     bucket;
    ino_t        hashkey;
    struct long_ad icb;

    icb.loc.lb_num = udf_rw32(TAILQ_FIRST(&udf_node->dscr_allocs)->lb_num);

    log_vol           = udf_node->udf_log_vol;
    hashkey           = udf_calc_hash(&icb);
    udf_node->hashkey = hashkey;
    bucket            = hashkey & UDF_INODE_HASHMASK;
    LIST_INSERT_HEAD(&log_vol->udf_nodes[bucket], udf_node, next_node);
}


/* dispose udf_node's administration */
void udf_dispose_udf_node(struct udf_node *udf_node) {
    struct udf_allocentry *alloc_entry;
    struct udf_buf	 *buf_entry;
    struct udf_node	 *lookup;
    uint32_t bucket;
    ino_t    hashkey;

    if (!udf_node) return;

/* XXX locks? XXX */
    UDF_MUTEX_LOCK(&udf_node->alloc_mutex);

    if (udf_node->dirty) {
        DEBUG(printf("Warning: disposing dirty node\n"));
        udf_node_unmark_dirty(udf_node);
    }

    /* free all associated buffers */
    UDF_MUTEX_LOCK(&udf_bufcache->bufcache_lock);
    UDF_MUTEX_LOCK(&udf_node->buf_mutex);
        /* due to this `trick' we don't need to use a marker */
        while ((buf_entry = TAILQ_FIRST(&udf_node->vn_bufs))) {
            udf_mark_buf_clean(udf_node, buf_entry);	/* its destroyed so not dirty */
            udf_mark_buf_allocated(udf_node, buf_entry);	/* i.e. taken care of */
            udf_detach_buf_from_node(udf_node, buf_entry);
            udf_free_buf_entry(buf_entry);
        }
        /* free in-node filedata blob if present */
        if (udf_node->intern_data)
            free(udf_node->intern_data);
    UDF_MUTEX_UNLOCK(&udf_node->buf_mutex);
    UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);

    /* free our extended attribute administration if present */
    if (udf_node->extattrfile_icb)
        free(udf_node->extattrfile_icb);
    if (udf_node->streamdir_icb)
        free(udf_node->streamdir_icb);
    if (udf_node->extended_attr)
        free(udf_node->extended_attr);

    /* free dscr_allocs queue */
    while ((alloc_entry = TAILQ_FIRST(&udf_node->dscr_allocs))) {
        TAILQ_REMOVE(&udf_node->dscr_allocs, alloc_entry, next_alloc);
        free(alloc_entry);
    }

    /* free allocation queue */
    while ((alloc_entry = TAILQ_FIRST(&udf_node->alloc_entries))) {
        TAILQ_REMOVE(&udf_node->alloc_entries, alloc_entry, next_alloc);
        free(alloc_entry);
    }

    /* if its part of a logical volume, delete it in its hash table */
    if (udf_node->udf_log_vol) {
        hashkey = udf_node->hashkey;
        bucket  = hashkey & UDF_INODE_HASHMASK;
        LIST_FOREACH(lookup, &udf_node->udf_log_vol->udf_nodes[bucket], next_node) {
            /* hashkey doesn't matter; just remove same udf_node pointer */
            if (lookup == udf_node) {
                assert(lookup->hashkey == hashkey);
                DEBUG(printf("removal of udf_node from the hash table\n"));
                LIST_REMOVE(lookup, next_node);
                break;
            }
        }
    }
    UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);

    /* free the node */
    free(udf_node);
}


uint64_t udf_increment_unique_id(struct udf_log_vol *udf_log_vol) {
    uint64_t unique_id, next_unique_id;

    /* lock? */
    unique_id = udf_log_vol->next_unique_id;

    /* increment according to UDF 3/3.2.1.1 */
    next_unique_id = unique_id + 1;
    if (((next_unique_id << 32) >> 32) < 16) next_unique_id |= 16;

    udf_log_vol->next_unique_id = next_unique_id;
    DEBUG(printf("next unique_id <-- %"PRIu64"\n", udf_log_vol->next_unique_id));

    return unique_id;
}


int udf_init_udf_node(struct udf_mountpoint *mountpoint, struct udf_log_vol *udf_log_vol, char *what, struct udf_node **udf_nodeptr) {
    struct udf_node	*udf_node;
    uint32_t lb_size, data_space_avail;
    int descr_ver;

    what = what;	/* not used yet */

    assert(udf_log_vol);
    lb_size = udf_log_vol->lb_size;

    /* get ourselves some space */
    udf_node = calloc(1, sizeof(struct udf_node));
    if (!udf_node) return ENOMEM;

    /* setup basic udf_node */
    udf_node->addr_type = UDF_ICB_LONG_ALLOC;
    udf_node->icb_len   = sizeof(struct long_ad);

    udf_node->serial_num       = 1;
    udf_node->udf_icbtag_flags = UDF_ICB_INTERN_ALLOC;
    udf_node->link_cnt         = 1;						/* how many FID's are linked to this node */
    udf_node->unique_id        = 0;						/* unique file ID unknown		  */

    /* get internal space available for internal nodes */
    /* TODO keep in mind the extended attributes! XXX */
    descr_ver = udf_rw16(udf_log_vol->log_vol->tag.descriptor_ver);
    if (descr_ver == 2) {
        /* TODO reserve some space for the icb creation time */
        data_space_avail = lb_size - sizeof(struct file_entry)    - 0; /* udf_rw32(file_entry->l_ea) */
    } else {
        data_space_avail = lb_size - sizeof(struct extfile_entry) - 0; /* udf_rw32(extfile_entry->l_ea) */
    }
    udf_node->intern_free = data_space_avail;
    udf_node->intern_data = NULL;
    udf_node->intern_len  = 0;

    /* finalise udf_node */
    udf_node->mountpoint  = mountpoint;
    udf_node->udf_log_vol = udf_log_vol;
    TAILQ_INIT(&udf_node->dscr_allocs);
    TAILQ_INIT(&udf_node->alloc_entries);
    TAILQ_INIT(&udf_node->vn_bufs);
    UDF_MUTEX_INIT(&udf_node->alloc_mutex);
    UDF_MUTEX_INIT(&udf_node->buf_mutex);

    /* XXX udf_node_lock NOT USED XXX */
    /* pthread_rwlock_init(&udf_node->udf_node_lock, NULL); */

    *udf_nodeptr = udf_node;
    return 0;
}


int udf_allocate_udf_node_on_disc(struct udf_node *udf_node) {
    struct udf_allocentry *alloc_entry;
    uint32_t  lb_num, lb_size;
    uint16_t  vpart_num;
    int       error;

    assert(udf_node);
    assert(udf_node->udf_log_vol);
    assert(udf_node->udf_log_vol->log_vol);

    lb_size   = udf_node->udf_log_vol->lb_size;
    assert(lb_size);

    /* pre-allocate node; its needed in directory linkage for now */
    error = udf_allocate_lbs(udf_node->udf_log_vol, UDF_C_NODE, /*num lb */ 1, "New FID", &vpart_num, &lb_num, NULL);
    if (error) return error;

    alloc_entry = calloc(1, sizeof(struct udf_allocentry));
    if (!alloc_entry) {
        return ENOMEM;
    }

    alloc_entry->len        = lb_size;
    alloc_entry->vpart_num  = vpart_num;
    alloc_entry->lb_num     = lb_num;
    alloc_entry->flags      = 0;
    TAILQ_INSERT_TAIL(&udf_node->dscr_allocs, alloc_entry, next_alloc);

    assert(error == 0);
    return error;
}


/* note: the udf_node (inode) is not stored in a hashtable! hash value is still invalid */
/* TODO remember our extended attributes and remember our streamdir long_ad  */
int udf_readin_anon_udf_node(struct udf_log_vol *udf_log_vol, union dscrptr *given_dscrptr, struct long_ad *udf_icbptr, char *what, struct udf_node **udf_nodeptr) {
    union  dscrptr  	*dscrptr;
    struct udf_node 	*udf_node;
    struct udf_allocentry	*alloc_entry;
    struct udf_allocentry	*cur_alloc, *next_alloc;
    struct file_entry	*file_entry;
    struct extfile_entry	*extfile_entry;
    struct alloc_ext_entry	*alloc_ext_entry;
    struct long_ad		*l_ad;
    struct short_ad		*s_ad;
    uint64_t		 inf_len, calculated_len;
    uint32_t		 lb_size, entries;
    uint64_t		 data_length;
    uint32_t		 data_space_avail;
    uint32_t		 fe_tag;
    uint64_t		 len;
    uint32_t		 lb_num, vpart_num;
    uint32_t		 icb_len;
    int16_t			 addr_type;
    uint8_t			*pos;
    uint8_t			 flags;
    int			 error, advance_sector;

    if ((udf_icbptr->loc.lb_num == 0) && (udf_icbptr->loc.part_num == 0) && (udf_icbptr->len == 0)) return ENOENT;

    DEBUG(printf("udf_readin_anon_udf_node for %s\n", what));

    error = udf_init_udf_node(/*mountpoint*/ NULL, udf_log_vol, what, &udf_node);
    DEBUG(
        if (error) printf("While reading in `anononymous' udf_node : got error %s\n", strerror(error));
    );
    if (error) return error;

    *udf_nodeptr = udf_node;

    assert(udf_log_vol);
    lb_size     = udf_log_vol->lb_size;

    /* read in descriptor if not provided to us */
    dscrptr   = NULL;
    len       = lb_size;					/* nodes are defined in lb_size only (?) */
    lb_num    = udf_rw32(udf_icbptr->loc.lb_num);
    vpart_num = udf_rw16(udf_icbptr->loc.part_num);
    if (!given_dscrptr) {
        error = udf_read_logvol_descriptor(udf_log_vol, vpart_num, lb_num, what, &dscrptr, NULL);
        if (error) {
            *udf_nodeptr = NULL;
            return error;
        }
    } else {
        dscrptr = given_dscrptr;
    }

    fe_tag = udf_rw16(dscrptr->tag.id);
    if (fe_tag != TAGID_FENTRY && fe_tag != TAGID_EXTFENTRY) {
        /* something wrong with this address; it doesn't start with a file entry */
        printf("UDF: bad udf_node for %s; got a %d tag\n", what, fe_tag);
        if (dscrptr != given_dscrptr) free(dscrptr);
        udf_dispose_udf_node(udf_node);
        *udf_nodeptr = NULL;
        return EFAULT;
    }

    /* get as much info as possible */
    udf_node_get_fileinfo(udf_node, dscrptr);

    /* reset pending write count */
    udf_node->v_numoutput = 0;

    /* initialise various variables for extracting allocation information */
    inf_len = 0; data_length = 0; lb_num = 0; len = 0; vpart_num = 0; pos = NULL;
    data_space_avail = 0;

    next_alloc = calloc(1, sizeof(struct udf_allocentry));
    if (!next_alloc) {
        if (dscrptr != given_dscrptr) free(dscrptr);
        udf_dispose_udf_node(udf_node);
        *udf_nodeptr = NULL;
        return ENOMEM;
    }
    next_alloc->len       = lb_size;
    next_alloc->lb_num    = udf_rw32(udf_icbptr->loc.lb_num);
    next_alloc->vpart_num = udf_rw16(udf_icbptr->loc.part_num);

    entries = 1;
    calculated_len = 0;

    error = 0;
    addr_type = -1;
    do {
        cur_alloc = next_alloc;
        next_alloc = calloc(1, sizeof(struct udf_allocentry));
        if (!next_alloc) {
            if (dscrptr != given_dscrptr) free(dscrptr);
            udf_dispose_udf_node(udf_node);
            *udf_nodeptr = NULL;
            return ENOMEM;
        }
        memcpy(next_alloc, cur_alloc, sizeof(struct udf_allocentry));
        TAILQ_INSERT_TAIL(&udf_node->dscr_allocs, cur_alloc, next_alloc);

        /* process this allocation descriptor */
        /* note that we don't store the file descriptors -> XXX impl. use stuff gets lost here */
        fe_tag = udf_rw16(dscrptr->tag.id);
        switch (fe_tag) {
            case TAGID_FENTRY :
                /* allocation descriptors follow this tag */
                file_entry       = &dscrptr->fe;
                entries          = udf_rw16(file_entry->icbtag.max_num_entries);
                data_length      = udf_rw32(file_entry->l_ad);
                pos              = &file_entry->data[0] + udf_rw32(file_entry->l_ea);
                inf_len          = udf_rw64(file_entry->inf_len);
                addr_type        = udf_rw16(file_entry->icbtag.flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK;
                data_space_avail = lb_size - sizeof(struct file_entry) - udf_rw32(file_entry->l_ea);
                /* process extended attributes */
                /* keep a trace of the descriptors in this udf_node */
                UDF_VERBOSE_MAX(udf_dump_file_entry(file_entry));
                break;
            case TAGID_EXTFENTRY :
                /* allocation descriptors follow this tag */
                extfile_entry    = &dscrptr->efe;
                entries          = udf_rw16(extfile_entry->icbtag.max_num_entries);
                data_length      = udf_rw32(extfile_entry->l_ad);
                pos              = &extfile_entry->data[0] + udf_rw32(extfile_entry->l_ea);
                inf_len          = udf_rw64(extfile_entry->inf_len);
                addr_type        = udf_rw16(extfile_entry->icbtag.flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK;
                data_space_avail = lb_size - sizeof(struct extfile_entry) - udf_rw32(extfile_entry->l_ea);
                /* process extended attributes */
                /* keep a trace of the descriptors in this udf_node */
                UDF_VERBOSE_MAX(udf_dump_extfile_entry(extfile_entry));
                break;
            case TAGID_ALLOCEXTENT :
                /* allocation descriptors follow this tag; treat as if continuation of a (ext)file entry*/
                alloc_ext_entry  = &dscrptr->aee;
                data_length      = udf_rw32(alloc_ext_entry->l_ad);
                pos		 = &alloc_ext_entry->data[0];
                assert(addr_type >= 0);

                /* keep a trace of the descriptors in this udf_node */
                UDF_VERBOSE_MAX(udf_dump_alloc_extent(alloc_ext_entry, addr_type));
                break;
            case TAGID_INDIRECTENTRY :
                printf("create_anon_udf_node called with indirect entry; following chain\n");
                l_ad = &dscrptr->inde.indirect_icb;
                next_alloc->len       = udf_rw32(l_ad->len);
                next_alloc->lb_num    = udf_rw32(l_ad->loc.lb_num);
                next_alloc->vpart_num = udf_rw16(l_ad->loc.part_num);
                entries   = 1;		/* at least one more entry	*/
                advance_sector = 0;	/* don't advance to next sector */

                UDF_VERBOSE_MAX(udf_dump_descriptor(dscrptr));
                break;
            default:
                printf("read_file_part_extents: i can't be here! (tag = %d) in ICB hiargy of %s\n", fe_tag, what);
                udf_dump_descriptor(dscrptr);
                return EFAULT;
        }
        if (fe_tag != TAGID_INDIRECTENTRY) {
            udf_node->addr_type = addr_type;
            if (addr_type == UDF_ICB_INTERN_ALLOC) {
                udf_node->intern_len  = inf_len;
                udf_node->intern_free = data_space_avail;
                udf_node->intern_data = calloc(1, udf_node->intern_free);
                if (udf_node->intern_data) {
                    memcpy(udf_node->intern_data, pos, inf_len);
                } else {
                    error = ENOMEM;
                }

                if (dscrptr != given_dscrptr) free(dscrptr);
                return error;
            }

            icb_len = 0;
            switch (udf_node->addr_type) {
                case UDF_ICB_SHORT_ALLOC :
                    icb_len   = sizeof(struct short_ad);
                    break;
                case UDF_ICB_LONG_ALLOC  :
                    icb_len   = sizeof(struct long_ad);
                    break;
                default :
                    printf("UDF encountered an unknown allocation type %d\n", udf_node->addr_type);
                    break;
            }
            udf_node->icb_len = icb_len;
            advance_sector = 1;
            while (icb_len && data_length) {
                switch (udf_node->addr_type) {
                    case UDF_ICB_SHORT_ALLOC  :
                        s_ad = (struct short_ad *) pos;
                        len       = udf_rw32(s_ad->len);
                        lb_num    = udf_rw32(s_ad->lb_num);
                        vpart_num = cur_alloc->vpart_num;
                        break;
                    case UDF_ICB_LONG_ALLOC   :
                        l_ad = (struct long_ad *) pos;
                        len       = udf_rw32(l_ad->len);
                        lb_num    = udf_rw32(l_ad->loc.lb_num);
                        vpart_num = udf_rw16(l_ad->loc.part_num);
                        if (l_ad->impl.im_used.flags & UDF_ADIMP_FLAGS_EXTENT_ERASED) {
                            printf("UDF: got a `extent erased' flag in a file's long_ad; ignoring\n");
                        }
                        break;
                    default :
                        printf("UDF encountered an unknown allocation type %d\n", udf_node->addr_type);
                        break;
                }
                /* ecma-167 48.14.1.1 */
                flags = (uint8_t) ((uint32_t) (len >> 30) & 3);
                len   = len & ((1<<30)-1);
                if (flags == UDF_SPACE_REDIRECT) {
                    /* fill in next extent */
                    next_alloc->len       = len;
                    next_alloc->lb_num    = lb_num;
                    next_alloc->vpart_num = vpart_num;
                    advance_sector = 0;	/* don't advance to next sector */
                    icb_len = data_length;	/* must be last */
                    DEBUG(printf("Continuing extent flagged at vpart = %d, lb_num = %d, len = %d\n", (int) vpart_num, (int) lb_num, (int) len));
                } else {
                    if (len) {
                        alloc_entry = calloc(1, sizeof(struct udf_allocentry));
                        if (!alloc_entry) {
                            if (dscrptr != given_dscrptr) free(dscrptr);
                            return ENOMEM;
                        }
                        alloc_entry->len       = len;
                        alloc_entry->lb_num    = lb_num;
                        alloc_entry->vpart_num = vpart_num;
                        alloc_entry->flags     = flags;
                        TAILQ_INSERT_TAIL(&udf_node->alloc_entries, alloc_entry, next_alloc);
                    }
                    calculated_len += len;
                }
                data_length  -= icb_len;
                pos          += icb_len;
            } /* while */
            if (advance_sector) {
                /* Note: UDF descriptor length is maximised to one sector */
                next_alloc->lb_num++;	/* advance one sector */
                entries--;
            }
        } /* indirect ICB check */
        if (entries) {
            /* load in new dscrptr */
            if (dscrptr != given_dscrptr) free(dscrptr);
            error = udf_read_logvol_descriptor(udf_log_vol, next_alloc->vpart_num, next_alloc->lb_num, what, &dscrptr, NULL);
        }
    } while (entries && !error);

    /* error from reading next sector in extent is not considered an error */
    if (dscrptr != given_dscrptr && dscrptr) free(dscrptr);

    if (calculated_len != (uint64_t) udf_node->stat.st_size) {
        printf("UDF: create node length mismatch; stated as %g but calculated as %g bytes. fixing\n", (double) udf_node->stat.st_size, (double) calculated_len);
        udf_node->stat.st_size = calculated_len;
    }

    return 0;
}


int udf_readin_udf_node(struct udf_node *dir_node, struct long_ad *udf_icbptr, struct fileid_desc *fid, struct udf_node **res_sub_node) {
    struct udf_node    *sub_node;
    char               *fid_name;
    char                entry_name[NAME_MAX];
    uint32_t            bucket;
    ino_t               hashkey;
    int                 error;

    assert(dir_node);
    assert(udf_icbptr);
    assert(fid);
    assert(res_sub_node);

    /* check if its allready in the logical volume's udf_node cache (inodes) */
    hashkey = udf_calc_hash(udf_icbptr);
    bucket  = hashkey & UDF_INODE_HASHMASK;

    LIST_FOREACH(sub_node, &dir_node->udf_log_vol->udf_nodes[bucket], next_node) {
        if (sub_node->hashkey == hashkey) {
            *res_sub_node = sub_node;
            DEBUG(printf("found node in hashlist\n"));

            return 0;
        }
    }

    /* dump the FID we are trying to read in */
    UDF_VERBOSE_MAX(udf_dump_descriptor((union dscrptr *) fid));

    fid_name = (char *) fid->data + udf_rw16(fid->l_iu);
    udf_to_unix_name(entry_name, fid_name, fid->l_fi, &dir_node->udf_log_vol->log_vol->desc_charset);

    /* build missing vnode */
    error = udf_readin_anon_udf_node(dir_node->udf_log_vol, NULL, udf_icbptr, entry_name, &sub_node);
    if (error)
        return error;

    if (!sub_node) {
        printf("sub_node = NULL? and no error? \n");
    }
    assert(sub_node);

    /* link this UDF node to the mountpoint and remember its hash-key */
    sub_node->mountpoint       = dir_node->mountpoint;
    sub_node->hashkey          = hashkey;

    /* XXX use file version number and filechar from fid ? */
    sub_node->file_version_num = udf_rw16(fid->file_version_num);	/* user set */
    sub_node->udf_filechar     = fid->file_char;

    /* insert/replace in mountpoint's udfnode hashtable with optional check for doubles */
if (0) {
        struct udf_node *lookup;
        LIST_FOREACH(lookup, &dir_node->udf_log_vol->udf_nodes[bucket], next_node) {
            if (lookup->hashkey == sub_node->hashkey) printf("DOUBLE hashnode?\n");
        }
}

    LIST_INSERT_HEAD(&dir_node->udf_log_vol->udf_nodes[bucket], sub_node, next_node);
    DEBUG(printf("inserting hash node for hash = %d\n", (int) hashkey));

    *res_sub_node = sub_node;
    return 0;
}


void udf_syncnode_callback(int reason, struct udf_wrcallback *wrcallback, int error, uint8_t *sectordata) {
    /* struct udf_node *udf_node = (struct udf_node *) wrcallback->structure; */

    wrcallback = wrcallback;	/* unused for now */
    sectordata = sectordata;

    if (reason == UDF_WRCALLBACK_REASON_PENDING) {
        /* what to do? */
        return;
    }
    if (reason == UDF_WRCALLBACK_REASON_ANULATE) {
        /* what to do? */
        return;
    }
    assert(reason == UDF_WRCALLBACK_REASON_WRITTEN);
    if (error) {
        printf("UDF error: syncnode writing failed, not fixing yet!\n");
        return;
    }
}


/* XXX This mark node dirty code will simplify if we store nodes in buffers? XXX */
void udf_node_mark_dirty(struct udf_node *udf_node) {
    struct udf_allocentry *alloc_entry, *my_entry, *tail_entry;
    struct udf_node *search_node, *tail_node;

    if (udf_node->dirty) return;

    my_entry = TAILQ_FIRST(&udf_node->dscr_allocs);
    assert(my_entry);

    /* dscr locks ? */
    UDF_MUTEX_LOCK(&udf_node->udf_log_vol->dirty_nodes_mutex);
if (1) {
        /* insertion sort :-S */
        tail_node  = TAILQ_LAST(&udf_node->udf_log_vol->dirty_nodes, udf_node_list);
        if (!tail_node) {
            TAILQ_INSERT_TAIL(&udf_node->udf_log_vol->dirty_nodes, udf_node, next_dirty);
        } else {
            tail_entry = TAILQ_FIRST(&tail_node->dscr_allocs);
            if (tail_entry->lb_num < my_entry->lb_num) {
                TAILQ_INSERT_TAIL(&udf_node->udf_log_vol->dirty_nodes, udf_node, next_dirty);
            } else {
                /* find my place; could be done smarter */
                TAILQ_FOREACH(search_node, &udf_node->udf_log_vol->dirty_nodes, next_dirty) {
                    alloc_entry = TAILQ_FIRST(&tail_node->dscr_allocs);
                    if (alloc_entry->lb_num > my_entry->lb_num) {
                        TAILQ_INSERT_BEFORE(search_node, udf_node, next_dirty);
                        break;	/* foreach */
                    }
                }
            }
        }
} else {
        /* dumb */
        TAILQ_INSERT_TAIL(&udf_node->udf_log_vol->dirty_nodes, udf_node, next_dirty);
}
    UDF_MUTEX_UNLOCK(&udf_node->udf_log_vol->dirty_nodes_mutex);
    udf_node->dirty = 1;
}


static void udf_node_unmark_dirty(struct udf_node *udf_node) {
    if (!udf_node->dirty) return;

    /* remove me just in case; why were we called otherwise? */
    UDF_MUTEX_LOCK(&udf_node->udf_log_vol->dirty_nodes_mutex);
        TAILQ_REMOVE(&udf_node->udf_log_vol->dirty_nodes, udf_node, next_dirty);
    UDF_MUTEX_UNLOCK(&udf_node->udf_log_vol->dirty_nodes_mutex);
    udf_node->dirty = 0;
}


/* VOP_FSYNC : data FDATASYNC */
int udf_sync_udf_node(struct udf_node *udf_node, char *why) {

    DEBUG(printf("Syncing udf_node `%"PRIu64"` because of %s\n", udf_node->unique_id, why));
    DEBUG(printf("sync udf node: dirty = %d, v_numoutput = %d\n", udf_node->dirty, udf_node->v_numoutput));
    if (!udf_node->dirty) {
        /* Not dirty??!!! */
        udf_node_unmark_dirty(udf_node);
        return 0;
    }

    if (!udf_node->udf_log_vol->writable) {
        fprintf(stderr, "encountered a dirty node on a read-only filingsystem!\n");
        exit(1);
    }

    /*
     * We are really syncing disc but we are only continueing when the
     * node itself is clean... not breaking the semantics.
     */
    /* XXX flushall flag magic XXX */
    udf_bufcache->flushall = 1;
    udf_purgethread_kick("Sync node");
    fflush(stdout);

    /* wait until all dirty bufs associated with this node are processed */
    if (!udf_node->dirty) return 0;

    if (udf_node->v_numoutput) {
        usleep(100);
    }
    if (!udf_node->v_numoutput) return 0;

    UDF_VERBOSE(printf("(wait on node)"));
    while (udf_node->v_numoutput) {
        usleep(100);
    }

    return 0;
}


extern void udf_merge_allocentry_queue(struct udf_alloc_entries *queue, uint32_t lb_size);

/* VOP_FSYNC : metadata FFILESYNC */
/* writeout the udf_node back to disc */
/* TODO don't forget to writeout our extended attributes and write out the link to the associated streamdir as well */
int udf_writeout_udf_node(struct udf_node *udf_node, char *why) {
    struct udf_wrcallback   wr_callback;
    struct udf_allocentry  *dscr_entry, *next_dscr_entry, *alloc_entry;
    union dscrptr          *dscrptr;
    struct file_entry      *fe;
    struct extfile_entry   *efe;
    struct alloc_ext_entry *aee;
    struct lb_addr          parent_icb;
    struct icb_tag         *icbtag;
    struct long_ad         *l_ad;
    struct short_ad        *s_ad;
    uint32_t               *l_adptr;	/* points to length of alloc. descr. */
    uint8_t                *pos;
    char                   *what;
    uint32_t                alloc_entries;
    uint64_t                rest;		/* in bytes */
    uint64_t                len;
    uint64_t                logblks_rec;
    uint32_t		lb_size, descr_ver;

    if (!udf_node->udf_log_vol->writable) {
        fprintf(stderr, "encountered a dirty node on a read-only filingsystem!\n");
        exit(1);
    }

    lb_size     = udf_node->udf_log_vol->lb_size;

    /* assure all file data is written out! and clean up */
    udf_sync_udf_node(udf_node, why);

    /* XXX node lock? XXX */

    UDF_MUTEX_LOCK(&udf_node->alloc_mutex);
        /* clean up allocentry queue so we get a nice clean layout */
        udf_merge_allocentry_queue(&udf_node->alloc_entries, lb_size);
    UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);

    /* allocate for descriptor */
    dscrptr = calloc(1, lb_size);	/* not a bit biggish? */
    if (!dscrptr) return ENOMEM;

    /* calculate logical blocks recorded; zero for interns [UDF 2.3.6.5, ECMA 4/14.9.11, 4/14.6.8] */
    logblks_rec = 0;
    if (udf_node->addr_type != UDF_ICB_INTERN_ALLOC) {
        UDF_MUTEX_LOCK(&udf_node->alloc_mutex);
            TAILQ_FOREACH(alloc_entry, &udf_node->alloc_entries, next_alloc) {
                if (alloc_entry->flags == UDF_SPACE_ALLOCATED) {
                    logblks_rec += (alloc_entry->len + lb_size-1) / lb_size;
                }
            }
        UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);
    }

    /* fill in (ext)fentry descriptor */
    /* copy descriptor version from the logvol's */

    bzero(&parent_icb, sizeof(struct lb_addr));
    descr_ver = udf_rw16(udf_node->udf_log_vol->log_vol->tag.descriptor_ver);
    if (descr_ver == 3) {
        efe = &dscrptr->efe;
        efe->tag.id             = udf_rw16(TAGID_EXTFENTRY);
        efe->tag.descriptor_ver = udf_rw16(descr_ver);
        efe->ckpoint            = udf_rw32(1);			/* [ECMA 4/14.17.17] */
        udf_set_imp_id(&efe->imp_id);
        efe->logblks_rec = udf_rw64(logblks_rec);

        /* set additional fileinfo (access, uid/gid) etc. */
        udf_node_set_fileinfo(udf_node, dscrptr);
        efe->obj_size           = efe->inf_len;			/* not true if there are streams [ECMA 4/48.17.11] */

        icbtag = &efe->icbtag;
    } else if (descr_ver == 2) {
        fe = &dscrptr->fe;
        fe->tag.id             = udf_rw16(TAGID_FENTRY);
        fe->tag.descriptor_ver = udf_rw16(descr_ver);
        fe->ckpoint            = udf_rw32(1);			/* [ECMA 4/14.17.17] */
        udf_set_imp_id(&fe->imp_id);
        fe->logblks_rec = udf_rw64(logblks_rec);

        /* set additional fileinfo (access, uid/gid) etc. */
        udf_node_set_fileinfo(udf_node, dscrptr);
        /* fe->obj_size doesn't exist */

        icbtag = &fe->icbtag;
    } else {
        printf("UDF: i don't know this descriptor version %d\n", descr_ver);
        free(dscrptr);
        return EBADF;
    }

    /* update derived info */
    udf_node->udf_icbtag_flags = (udf_node->udf_icbtag_flags & ~UDF_ICB_TAG_FLAGS_ALLOC_MASK) | udf_node->addr_type;

    /* strategy 4 has no parent nodes */
    /* XXX NO extended attributes recorded YET (!!) XXX (like device nodes !!! ) */

    dscr_entry  = TAILQ_FIRST(&udf_node->dscr_allocs);

    /* ensure allocation of (ext) file descriptor */
    if (!dscr_entry) {
        /* have to allocate one and add to queue ! */
        printf("UDF: XXX no allocation of file descriptor entry yet in sync_udf_node\n");
        return ENOENT;
    }

    /* XXX implement alloc entry walker? XXX */
    UDF_MUTEX_LOCK(&udf_node->alloc_mutex);
    what = "UDF sync: File descriptor";
    alloc_entry = TAILQ_FIRST(&udf_node->alloc_entries);
    do {
        assert(dscr_entry->len <= lb_size);
        if (icbtag) {
            /* fill in ICB fields */			/* XXX we're only writing out strategy type 4 !!! XXX */
            icbtag->prev_num_dirs   = udf_rw32(0);		/* prev_alloc_entries = 0 due to strategy type 4 XXX */
            icbtag->strat_type      = udf_rw16(4);		/* default UDF strategy type 4 XXX what about 4096? */
            icbtag->parent_icb      = parent_icb;
            icbtag->file_type       = udf_node->udf_filetype;
            icbtag->flags           = udf_rw16(udf_node->udf_icbtag_flags);
            icbtag->max_num_entries = udf_rw16(1);		/* due to strategy type 4 ! XXX */
        }

        pos  = 0;
        rest = 0;
        l_adptr = NULL;		/* invalid */
        switch (udf_rw16(dscrptr->tag.id)) {
            case TAGID_FENTRY         :
                /* not used now */
                fe   = &dscrptr->fe;
                pos  = &fe->data[0] + udf_rw32(fe->l_ea);
                rest = dscr_entry->len - sizeof(struct file_entry) - udf_rw32(fe->l_ea);
                l_adptr = &fe->l_ad;
                break;
            case TAGID_EXTFENTRY      :
                efe  = &dscrptr->efe;
                pos  = &efe->data[0] + udf_rw32(efe->l_ea);
                rest = dscr_entry->len - sizeof(struct extfile_entry) - udf_rw32(efe->l_ea);
                l_adptr = &efe->l_ad;
                break;
            case TAGID_ALLOCEXTENT    :
                aee  = &dscrptr->aee;
                pos  = &aee->data[0];
                rest = dscr_entry->len - sizeof(struct alloc_ext_entry);
                l_adptr = &aee->l_ad;
                break;
            case TAGID_INDIRECTENTRY :
                /* do we even do these in strat 4 ? */
                printf("UDF: sanity check; request for writeout of indirect entry\n");
                free(dscrptr);
                UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);
                return ENOENT;	/* panic really */
        }
        assert(l_adptr);

        /* fill in remaining allocation entries 	*/
        /* remember to keep one SPARE for extending	*/

        alloc_entries = 0;
        DEBUG(printf("Allocated at "));

        if (udf_node->addr_type == UDF_ICB_INTERN_ALLOC) {
            /* internal allocation -> copy in data */
            assert(udf_node->intern_len <= rest);

            bzero(pos, rest);
            memcpy(pos, udf_node->intern_data, udf_node->intern_len);

            /* make sure the length for allocation entries is the same as the data it contains (4/8.8.2, 4/14.6.8) */
            *l_adptr = udf_rw32(udf_node->intern_len);

            /* alloc_entry is zero, so it'll fall trough */
            pos	+= udf_node->intern_len;	/* advance pos for length calculation */
            alloc_entry = NULL;			/* not applicable */
        }

        while ((rest > 2*udf_node->icb_len) && alloc_entry) {
            assert(udf_node->icb_len);
            DEBUG(printf("[%p, lb_num = %d, len = %d]   ", alloc_entry, (uint32_t) alloc_entry->lb_num, (uint32_t) alloc_entry->len));

            /* XXX assumption: UDF_SPACE_* is equal to UDF flags XXX */
            len = alloc_entry->len | (((uint32_t) alloc_entry->flags) << 30);
            switch (udf_node->addr_type) {
                case UDF_ICB_SHORT_ALLOC :
                    s_ad = (struct short_ad *) pos;
                    s_ad->len          = udf_rw32(len);
                    s_ad->lb_num       = udf_rw32(alloc_entry->lb_num);
                    assert(alloc_entry->vpart_num == 0);
                    if (alloc_entry->len   == 0)              s_ad->lb_num = udf_rw32(0);
                    if (alloc_entry->flags == UDF_SPACE_FREE) s_ad->lb_num = udf_rw32(0);
                    break;
                case UDF_ICB_LONG_ALLOC  :
                    l_ad = (struct long_ad *) pos;
                    l_ad->len          = udf_rw32(len);
                    l_ad->loc.lb_num   = udf_rw32(alloc_entry->lb_num);
                    l_ad->loc.part_num = udf_rw16(alloc_entry->vpart_num);
                    l_ad->impl.im_used.unique_id = udf_rw64(udf_node->unique_id);
                    if (alloc_entry->len   == 0)              l_ad->loc.lb_num = udf_rw32(0);
                    if (alloc_entry->flags == UDF_SPACE_FREE) l_ad->loc.lb_num = udf_rw32(0);
                    break;
            }
            alloc_entries++;
            *l_adptr = udf_rw32(alloc_entries * udf_node->icb_len);
            alloc_entry = TAILQ_NEXT(alloc_entry, next_alloc);
            pos  += udf_node->icb_len;
            rest -= udf_node->icb_len;
        }
        DEBUG(printf("\nend alloc\n\n"));

        next_dscr_entry = TAILQ_NEXT(dscr_entry, next_alloc);
        if (alloc_entry) {
            /* overflow */

            /* ensure allocation of (ext) file descriptor */
            if (!next_dscr_entry) {
                /* have to allocate one and add to queue ! */
                printf("UDF: XXX no allocation of allocation extent descriptor yet in sync_udf_node\n");
                UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);
                return ENOENT;
            }
            /* flag next extent being specified */
            len = next_dscr_entry->len | ((uint32_t) UDF_SPACE_REDIRECT << 30);
            switch (udf_node->addr_type) {
                case UDF_ICB_SHORT_ALLOC :
                    s_ad = (struct short_ad *) pos;
                    s_ad->len          = udf_rw32(len);
                    s_ad->lb_num       = udf_rw32(next_dscr_entry->lb_num);
                    assert(next_dscr_entry->vpart_num == 0);
                    break;
                case UDF_ICB_LONG_ALLOC  :
                    l_ad = (struct long_ad *) pos;
                    l_ad->len          = udf_rw32(len);
                    l_ad->loc.lb_num   = udf_rw32(next_dscr_entry->lb_num);
                    l_ad->loc.part_num = udf_rw16(next_dscr_entry->vpart_num);
                    l_ad->impl.im_used.unique_id = udf_rw64(udf_node->unique_id);
                    break;
            }
            alloc_entries++;
            pos  += udf_node->icb_len;
            rest -= udf_node->icb_len;
        }
        /* manage lengths */
        dscrptr->tag.desc_crc_len = udf_rw16((pos - (uint8_t *) dscrptr) - UDF_DESC_TAG_LENGTH);

        /* writeout */
        wr_callback.function  = udf_syncnode_callback;
#if 0
        wr_callback.structure = (void *) udf_node;
        wr_callback.vpart_num = dscr_entry->vpart_num;
        wr_callback.lb_num    = dscr_entry->lb_num;
        wr_callback.offset    = 0; /* ? */
        wr_callback.length    = dscr_entry->len;
#endif

        UDF_VERBOSE_MAX(
            udf_validate_tag_and_crc_sums(dscrptr);	/* for dumping */
            udf_dump_descriptor(dscrptr);
        );
        errno = udf_write_logvol_descriptor(udf_node->udf_log_vol, dscr_entry->vpart_num, dscr_entry->lb_num, what, dscrptr, &wr_callback);

        /* advance */
        if (alloc_entry) {
            dscr_entry = next_dscr_entry;
            assert(dscr_entry);

            /* allocate and initialise new allocation extent descriptor	*/
            /* also flag no icbtag follows					*/
            bzero(dscrptr, lb_size);
            aee = &dscrptr->aee;
            aee->tag.id             = udf_rw16(TAGID_ALLOCEXTENT);
            aee->tag.descriptor_ver = udf_node->udf_log_vol->log_vol->tag.descriptor_ver;
            icbtag = NULL;

            what = "UDF sync: allocation extent";
        }

    } while (alloc_entry);
    UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);

    /* XXX Free extra non used allocation extent descriptors XXX */

    /* Mark me clean */
    if (udf_node->mountpoint) {
        /* remove me just in case; why were we called otherwise? */
        udf_node_unmark_dirty(udf_node);
    }

    return 0;
}


/******************************************************************************************
 *
 * Space bitmap reader and writer
 *
 ******************************************************************************************/

/* tested OK */
int udf_read_in_space_bitmap(struct udf_alloc_entries *queue, struct space_bitmap_desc *sbd, uint32_t lb_size, uint64_t *freespace) {
    struct udf_allocentry *alloc_entry;
    uint64_t bits, from, now, start, end;
    uint8_t byte, bit, bitpos, state, *pos;
    int cnt;

    assert(udf_rw16(sbd->tag.id) == TAGID_SPACE_BITMAP);

    DEBUG(printf("processing space bitmap : \n"););
    bits = udf_rw32(sbd->num_bits);

    /*
     * Mark the disc as completely full;
     * Bugallert: use `udf_mark_allocentry_queue()' for the extent might
     * not fit in just one alloc entry for bigger discs
     */
    assert(TAILQ_EMPTY(queue));
    udf_mark_allocentry_queue(queue, lb_size, 0, bits * lb_size, UDF_SPACE_ALLOCATED, NULL, NULL);

    pos = sbd->data;
    from = 0; now = 0; bitpos = 0; byte = *pos; state = byte & 1;
    *freespace = 0;
    while (now < bits) {
        if (bitpos == 0) {
            byte = *pos++;
        }
        bit = byte & 1;
        if (bit != state) {
            if (state) {
                start = from;
                end   = now-1;
                /* printf("[%08d - %08d]", start, end); */
                udf_mark_allocentry_queue(queue, lb_size, start*lb_size, (end-start+1)*lb_size, UDF_SPACE_FREE, NULL, NULL);
                *freespace += (end-start+1)*lb_size;
            }
            from = now;
            state = bit;
        }
        byte >>= 1;
        bitpos = (bitpos+1) & 7;
        now++;
    }
    if (state) {
        start = from;
        end   = now;
        /* printf("[%08d - %08d]", start, end); */
        udf_mark_allocentry_queue(queue, lb_size, start*lb_size, (end-start)*lb_size, UDF_SPACE_FREE, NULL, NULL);
        *freespace += (end-start)*lb_size;
    }

    UDF_VERBOSE_TABLES(
        printf("\t\tFree space found on this partition");
        cnt = 0;
        start = 0;
        TAILQ_FOREACH(alloc_entry, queue, next_alloc) {
            if (alloc_entry->flags == UDF_SPACE_ALLOCATED) {
                /* printf("... "); */
            } else {
                if (cnt == 0) printf("\n\t\t\t");
                printf("[%08"PRIu64" - %08"PRIu64"]   ", start / lb_size, ((start + alloc_entry->len) / lb_size)-1);
                cnt++; if (cnt > 4) cnt = 0;
            }
            start += alloc_entry->len;
        }
        printf("\n");
    );

    /* merge is not nessisary */
    return 0;
}


/* inverse of readin space bitmap; it synchronises the bitmap with the queue */
/* tested OK */
int udf_sync_space_bitmap(struct udf_alloc_entries *queue, struct space_bitmap_desc *sbd, uint32_t lb_size) {
    struct udf_allocentry *alloc_entry;
    uint32_t start, bits, total_bits;
    uint32_t cnt, byte;
    uint8_t  bit, bitmask, setting;
    uint8_t *pos;

    /* merge it just in case */
    udf_merge_allocentry_queue(queue, lb_size);

    total_bits = udf_rw32(sbd->num_bits);
    DEBUG(printf("SYNC SPACE BITMAP DEBUG: total bits = %d\n", total_bits));

    alloc_entry = TAILQ_FIRST(queue);
    start = alloc_entry->lb_num;
    assert(start == 0);

    TAILQ_FOREACH(alloc_entry, queue, next_alloc) {
        DEBUG(printf(" [%d : %d + %d]", alloc_entry->flags, alloc_entry->lb_num, alloc_entry->len));
        bits  = alloc_entry->len / lb_size;
        assert(bits*lb_size == alloc_entry->len);

        byte = start / 8;
        bit  = start - byte*8;

        pos = sbd->data + byte;

        if (byte*8 + bit + bits > total_bits) {		/* XXX > or >= ? */
            /* this should NEVER happen */
            printf("UDF: not enough space writing back space bitmap! HELP!\n");
            return EBADF;
        }

        cnt = 0;
        setting = (alloc_entry->flags != UDF_SPACE_FREE) ? 0 : 255;
        while (cnt < bits) {
            bitmask = (1 << bit);

            /* simple sanity check */
            if (byte*8 + bit >= total_bits) {
                printf("IEEEE!!!! too big; %d instead of %d\n", (byte*8 + bit), total_bits);
            }
            *pos = (*pos & (~bitmask)) | (setting ? bitmask: 0);

            cnt++;
            bit++;
            if (bit == 8) {
                /* byte transition */
                byte++; bit = 0;
                pos++;
#if 0
                /* speedup by doing bytes at a time */
                while (bits-cnt > 8) {
                    *pos = setting;
                    cnt += 8;
                    pos++; byte++;
                }
#endif
            }
        }
        start += bits;
    }
    DEBUG(printf("\n\n"));
    return 0;
}



/******************************************************************************************
 *
 * Filepart readers and writers
 *
 ******************************************************************************************/

/* part of VOP_STRATEGY */
/* internal function; reads in a new buffer */
/* !!! bufcache lock ought to be held on entry !!! */
int udf_readin_file_buffer(struct udf_node *udf_node, char *what, uint32_t sector, int cache_flags, struct udf_buf **buf_entry_p) {
    struct udf_allocentry *alloc_entry;
    struct udf_buf	*buf_entry;
    uint64_t         cur_offset;
    uint64_t         overlap_length, overlap_sectors, transfer_length;
    uint32_t	 lb_size;
    uint32_t         len, lb_num, vpart_num;
    int32_t	         error;
    uint8_t	         flags;

    assert(udf_node);
    assert(buf_entry_p);
    assert(udf_bufcache->bufcache_lock.locked);

    error = udf_get_buf_entry(udf_node, buf_entry_p);
    if (error) return error;

    buf_entry = *buf_entry_p;
    lb_size   = udf_node->udf_log_vol->lb_size;

    /* internal node? */
    if (udf_node->addr_type == UDF_ICB_INTERN_ALLOC) {
        buf_entry->b_lblk    = 0;
        buf_entry->b_flags   = 0;				/* not dirty, not needing alloc */
        buf_entry->b_bcount  = udf_node->intern_len;
        buf_entry->b_resid   = lb_size - udf_node->intern_len;

        memcpy(buf_entry->b_data, udf_node->intern_data, udf_node->intern_len);

        UDF_MUTEX_LOCK(&udf_node->buf_mutex);
            udf_attach_buf_to_node(udf_node, buf_entry);
        UDF_MUTEX_UNLOCK(&udf_node->buf_mutex);
        return error;
    }

    /* `normal' node */

    /* find sector in the allocation space */
    UDF_MUTEX_LOCK(&udf_node->alloc_mutex);
    cur_offset = 0;
    error = EIO;	/* until proven readable */
    TAILQ_FOREACH(alloc_entry, &udf_node->alloc_entries, next_alloc) {
        len       = alloc_entry->len;
        lb_num    = alloc_entry->lb_num;
        vpart_num = alloc_entry->vpart_num;
        flags     = alloc_entry->flags;

        /* check overlap with this alloc entry */
        if (cur_offset + len > sector * lb_size) {
            /* found an extent that fits */
            assert(((sector * lb_size - cur_offset) % lb_size) == 0);	/* ought to be on sector boundary */
            lb_num += sector - (cur_offset / lb_size);

            overlap_length  = cur_offset + len - sector * lb_size;
            overlap_sectors = (overlap_length + lb_size -1) / lb_size;
            transfer_length = MIN(lb_size, overlap_length);			/* max one logical sector */

            /* fill in new buf info */
            buf_entry->b_lblk   = sector;
            buf_entry->b_bcount = transfer_length;
            buf_entry->b_resid  = lb_size - transfer_length;

            /* sector transfered; mark valid */
            buf_entry->b_flags  = 0;		/* not dirty and valid */

            switch (flags) {
                case UDF_SPACE_ALLOCATED :
                    /* on disc or in the write queue/cache to be written out; read one block at a time */
                    error = udf_read_logvol_sector(udf_node->udf_log_vol, vpart_num, lb_num, what, buf_entry->b_data, overlap_sectors, cache_flags);
                    break;
                case UDF_SPACE_FREE :
                    /* fall trough */
                case UDF_SPACE_ALLOCATED_BUT_NOT_USED :
                    error = 0;
                    bzero(buf_entry->b_data, lb_size);
                    break;
                default :
                    fprintf(stderr, "Got an redirect flag, can't happen\n");
                    break;
            }

            if (error) {
                fprintf(stderr, "\tgot error from read_logvol_sector : %s\n", strerror(error));
                break;	/* FOREACH */
            }

            UDF_MUTEX_LOCK(&udf_node->buf_mutex);
                udf_attach_buf_to_node(udf_node, buf_entry);
            UDF_MUTEX_UNLOCK(&udf_node->buf_mutex);

            UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);
            return 0;

        } /* looking for overlap */

        cur_offset += len;
    } /* FOREACH */
    UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);

    *buf_entry_p = NULL;

    udf_mark_buf_clean(udf_node, buf_entry);	/* its destroyed so not dirty */
    udf_mark_buf_allocated(udf_node, buf_entry);	/* i.e. taken care of */
    udf_free_buf_entry(buf_entry);
    return error;
}


/* internal function; shedule writing out of a buffer */
/* part of VOP_STRATEGY */
int udf_writeout_file_buffer(struct udf_node *udf_node, char *what, int cache_flags, struct udf_buf *buf_entry) {
    struct udf_allocentry *from_alloc, *to_alloc, *alloc_entry;
    uint32_t lb_num, lb_size, lblk;
    uint16_t vpart_num;
    int error;

    what = what;	/* not used now */

    if (!udf_node->udf_log_vol->writable) {
        /* ieek */
        fprintf(stderr, "write request from non writable file buffer?\n");
    }

    /* first get logical block offset and block size */
    lblk = buf_entry->b_lblk;
    lb_size = udf_node->udf_log_vol->lb_size;

    error = 0;
    UDF_MUTEX_LOCK(&udf_bufcache->bufcache_lock);
    UDF_MUTEX_LOCK(&udf_node->alloc_mutex);
    UDF_MUTEX_LOCK(&udf_node->buf_mutex);
        /* maybe a bit too strict locking; code is not winning the beauty contest  */

        /* check if we can convert/save disc space */
        if (udf_node->stat.st_size <= udf_node->intern_free) {
            /* convert to internal alloc with backup storage (writing there) */
            if (udf_node->addr_type != UDF_ICB_INTERN_ALLOC) {
                DEBUG(printf("CONVERTING to intern alloc\n"));
                /* first free all previously allocated sectors (if any) */
                error = udf_node_release_extent(udf_node, 0, udf_node->stat.st_size);
            }
            assert(!error);
            if (!udf_node->intern_data) {
                udf_node->intern_data = calloc(1, udf_node->intern_free);
            }
            if (udf_node->intern_data) {
                assert(buf_entry->b_bcount <= udf_node->intern_free);

                memcpy(udf_node->intern_data, buf_entry->b_data, buf_entry->b_bcount);
                udf_node->intern_len = buf_entry->b_bcount;
                udf_node->addr_type = UDF_ICB_INTERN_ALLOC;

                /* we don't write here: mark buffer clean */
                udf_mark_buf_clean(udf_node, buf_entry);
                udf_mark_buf_allocated(udf_node, buf_entry);		/* signal its allocated */
                buf_entry->b_flags &= ~(B_ERROR);

                /* check if all buffers are gone now and mark node for writeout to indicate state change */
                assert(udf_node->v_numoutput == 0);
                udf_node_mark_dirty(udf_node);

                UDF_MUTEX_UNLOCK(&udf_node->buf_mutex);
                UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);
                UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);
                return 0;
            }
            /* fall trough ... for some reason it isn't converted */
        } else {
            if (udf_node->addr_type == UDF_ICB_INTERN_ALLOC) {
                DEBUG(printf("CONVERTING to normal alloc\n"));
                /* won't fit anymore, have to turn it into a `normal' alloced */
                udf_node->intern_len = 0;
                if (udf_node->intern_data)
                    free(udf_node->intern_data);
                udf_node->intern_data = NULL;
                udf_node->icb_len   = sizeof(struct long_ad);
                udf_node->addr_type = UDF_ICB_LONG_ALLOC;

                udf_node_mark_dirty(udf_node);

                /* mark needalloc to make sure it gets an address */
                udf_mark_buf_needing_allocate(udf_node, buf_entry);	/* signal it needs allocation */
            }
        }

        /* merge queue first to get better performance */
        udf_merge_allocentry_queue(&udf_node->alloc_entries, lb_size);

        /* get extent that it represents */
        udf_mark_allocentry_queue(&udf_node->alloc_entries, lb_size, (uint64_t) lblk*lb_size, buf_entry->b_bcount, UDF_SPACE_ALLOCATED, &from_alloc, &to_alloc);
        alloc_entry = from_alloc;
        if (buf_entry->b_flags & B_NEEDALLOC) {
            /* need allocation */
            error = udf_node_allocate_lbs(udf_node, /* num lb */ 1, &vpart_num, &lb_num, NULL);
            assert(!error);
            udf_mark_buf_allocated(udf_node, buf_entry);

            alloc_entry->lb_num    = lb_num;
            alloc_entry->vpart_num = vpart_num;
        }
        assert(TAILQ_NEXT(alloc_entry, next_alloc) == to_alloc || (alloc_entry == to_alloc));

        lb_num    = alloc_entry->lb_num;
        vpart_num = alloc_entry->vpart_num;
    UDF_MUTEX_UNLOCK(&udf_node->buf_mutex);
    UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);
    UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);

    /* printf("writing out filebuffer vpart %d, lb_num %d for %s\n", vpart_num, lb_num, what); */
    error = udf_write_logvol_sector(udf_node->udf_log_vol, vpart_num, lb_num, "File contents", buf_entry->b_data, cache_flags, NULL);

    UDF_MUTEX_LOCK(&udf_bufcache->bufcache_lock);
    UDF_MUTEX_LOCK(&udf_node->buf_mutex);
        if (error) {
            printf("YIKES error during writing of logvol_sector\n");
            udf_mark_buf_needing_allocate(udf_node, buf_entry);
            buf_entry->b_flags |= B_ERROR;
            /* buf_entry->b_errno = error; */
        } else {
            udf_mark_buf_clean(udf_node, buf_entry);
            buf_entry->b_flags &= ~(B_ERROR);
        }
    UDF_MUTEX_UNLOCK(&udf_node->buf_mutex);
    UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);

    return error;
}


/* VOP_READ */
int udf_read_file_part_uio(struct udf_node *udf_node, char *what, int content, struct uio *data_uio) {
    struct udf_buf	*buf_entry;
    off_t  offset;
    uint64_t  sector, lb_size, data_length;
    uint8_t  *base;
    int error, short_buf;

    if (!udf_node) return EINVAL;

    /* NOTE: we are NOT marking the node dirty only by reading it */
    udf_set_timespec_now(&udf_node->stat.st_atimespec);

    if (udf_node->stat.st_size == 0) {
        if (data_uio->uio_resid) return EIO; /* reading past end by default */
        return 0;
    }

    lb_size = udf_node->udf_log_vol->lb_size;
    error = 0;
    while (data_uio->uio_resid) {
        error = 0;
        short_buf = 0;
        sector  = data_uio->uio_offset / lb_size;

        /* lookup sector in buffer set */
        UDF_MUTEX_LOCK(&udf_bufcache->bufcache_lock);
            udf_lookup_node_buf(udf_node, sector, &buf_entry);

            if (!buf_entry || (buf_entry && (buf_entry->b_lblk != sector))) {
                /* `page in' sector from file */
                error = udf_readin_file_buffer(udf_node, what, sector, content, &buf_entry);
            }

            if (!error && buf_entry) {
                offset = data_uio->uio_offset - (off_t) sector*lb_size;
                base   = buf_entry->b_data;
                if (offset >= 0) {
                    data_length = buf_entry->b_bcount - offset;
                    data_length = MIN(data_length, data_uio->uio_resid);
                    uiomove(base + offset, data_length, data_uio);
                }
                short_buf = (buf_entry->b_bcount < lb_size);	/* short buf -> none will follow */
            }
            assert(!error || (error && !buf_entry));
        UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);
        if (error) break; /* while */

        if (data_uio->uio_resid == 0) return 0;		/* finished? */
        if (short_buf) break;				/* while */
    } /* while */

    if (data_uio->uio_resid) {
        printf("UDF: WARNING file is truncated; missing %d bytes while reading %s\n",
                (int) data_uio->uio_resid, what);
        return EIO;
    }

    return error;
}


/* XXX meaning is to be changed; kept for reference still XXX */
void udf_filepart_write_callback(int reason, struct udf_wrcallback *wrcallback, int error, uint8_t *sectordata) {
    /* struct udf_node *udf_node = (struct udf_node *) wrcallback->structure; */

    wrcallback = wrcallback;	/* unused for now */
    sectordata = sectordata;

    if (reason == UDF_WRCALLBACK_REASON_PENDING) {
        /* what to do? */
        return;
    }
    if (reason == UDF_WRCALLBACK_REASON_ANULATE) {
        /* what to do? */
        return;
    }
    assert(reason == UDF_WRCALLBACK_REASON_WRITTEN);
    if (error) {
        printf("UDF error: file part write failed, not fixing yet!\n");
        return;
    }
}


/* VOP_TRUNCATE */
/* doesn't lock udf_node */
int udf_truncate_node(struct udf_node *udf_node, uint64_t length /* ,ioflags */) {
    struct udf_log_vol    *udf_log_vol;
    struct udf_allocentry *alloc_entry, *cut_point;
    struct udf_buf        *buf_entry, *marker;
    uint32_t lb_size;
    uint64_t cur_extent, block_extent, new_extent, too_much;
    uint32_t last_sector;
    int32_t	 error;

    if (!udf_node) return EINVAL;

    if (udf_open_logvol(udf_node->udf_log_vol))
        return EROFS;

    udf_log_vol = udf_node->udf_log_vol;
    lb_size     = udf_log_vol->lb_size;

    /* we might change this node AND access times ... */
    if (!udf_node->dirty) {
        /* mark node as dirty */
        udf_node_mark_dirty(udf_node);
    }

/* XXX lock node !!! XXX */

    /* merge known allocation entry queue */
    new_extent   = length;
    block_extent = (uint64_t) lb_size * ((new_extent + lb_size-1) / lb_size);	/* inclusive */
    too_much     = block_extent - new_extent;
    assert(block_extent >= new_extent);

    UDF_MUTEX_LOCK(&udf_bufcache->bufcache_lock);
    UDF_MUTEX_LOCK(&udf_node->alloc_mutex);
        udf_merge_allocentry_queue(&udf_node->alloc_entries, lb_size);

        /* extend the file when nessisary */
        if (new_extent > (uint64_t) udf_node->stat.st_size) {
            if (udf_node->addr_type == UDF_ICB_INTERN_ALLOC) {
                /* XXX to seperate function XXX */
                /* grow intern node by converting it to a normal buffer first */
                /* First lookup if the node already had a buffer associated */
                udf_lookup_node_buf(udf_node, 0, &buf_entry);
                if (!buf_entry) {
                    error = udf_get_buf_entry(udf_node, &buf_entry);
                    if (error) {
                        UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);
                        UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);
                        return error;
                    }
                    buf_entry->b_lblk   = 0;
                    buf_entry->b_flags  = 0;

                    UDF_MUTEX_LOCK(&udf_node->buf_mutex);
                        udf_attach_buf_to_node(udf_node, buf_entry);
                    UDF_MUTEX_UNLOCK(&udf_node->buf_mutex);
                } else {
                    DEBUG(printf("found buffer associated with intern node!\n"));
                }

                buf_entry->b_bcount = MIN(lb_size, new_extent);
                buf_entry->b_resid  = lb_size - buf_entry->b_bcount;

                memcpy(buf_entry->b_data, udf_node->intern_data, udf_node->intern_len);
                memset(buf_entry->b_data + udf_node->intern_len, 0, lb_size - udf_node->intern_len);

                UDF_MUTEX_LOCK(&udf_node->buf_mutex);
                    udf_mark_buf_dirty(udf_node, buf_entry);
                UDF_MUTEX_UNLOCK(&udf_node->buf_mutex);

                udf_node->intern_len = 0;
                if (udf_node->intern_data)
                    free(udf_node->intern_data);
                udf_node->intern_data = NULL;
                udf_node->icb_len   = sizeof(struct long_ad);
                udf_node->addr_type = UDF_ICB_LONG_ALLOC;
            }
            udf_cut_allocentry_queue(&udf_node->alloc_entries, lb_size, block_extent);
            if (new_extent < block_extent) {
                alloc_entry = TAILQ_LAST(&udf_node->alloc_entries, udf_alloc_entries);
                assert(alloc_entry->len > too_much);

                alloc_entry->len -= too_much;
            }
            /* XXX Andrey suggested buffer extension? XXX */
            udf_node->stat.st_size = new_extent;
        }
    UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);
    UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);

    /* `free' the extent when shrinking the extent */
    if (new_extent < (uint64_t) udf_node->stat.st_size) {
        DEBUG(printf("TRIMMING file %"PRIu64" from %d to %d\n", udf_node->unique_id, (int32_t) udf_node->stat.st_size, (uint32_t) length));

        /* free file buffers that are not needed anymore */
        /* XXX NetBSD kernel : uvm_vnp_setsize() XXX */
        marker = calloc(1, sizeof(struct udf_buf));
        if (!marker) return ENOMEM;

        UDF_MUTEX_LOCK(&udf_bufcache->bufcache_lock);
        UDF_MUTEX_LOCK(&udf_node->buf_mutex);
            /* use marker as we are going to delete items in the list we are traversing */
            last_sector = new_extent / lb_size;
            TAILQ_INSERT_HEAD(&udf_node->vn_bufs, marker, b_vnbufs);
            while ((buf_entry = TAILQ_NEXT(marker, b_vnbufs))) {
                /* advance marker */
                TAILQ_REMOVE(&udf_node->vn_bufs, marker, b_vnbufs);
                TAILQ_INSERT_AFTER(&udf_node->vn_bufs, buf_entry, marker, b_vnbufs);

                /* process buf_entry */
                if (buf_entry->b_lblk > last_sector) {
                    udf_mark_buf_clean(udf_node, buf_entry);	/* its destroyed so not dirty */
                    udf_mark_buf_allocated(udf_node, buf_entry);	/* i.e. taken care of */
                    udf_detach_buf_from_node(udf_node, buf_entry);
                    udf_free_buf_entry(buf_entry);
                }
                /* trim last buffer entry */
                /* XXX NetBSD kernel : uvm_vnp_zerorange(), vtruncbuf() XXX */
                if (buf_entry->b_lblk == last_sector) {
                    buf_entry->b_bcount = udf_node->stat.st_size % lb_size;
                    buf_entry->b_resid  = buf_entry->b_bufsize - buf_entry->b_bcount;
                    /* still data to record? */
                    if (buf_entry->b_bcount == 0) {
                        /* marker not an issue here */
                        udf_mark_buf_clean(udf_node, buf_entry);	/* its destroyed so not dirty */
                        udf_mark_buf_allocated(udf_node, buf_entry);	/* i.e. taken care of */
                        udf_detach_buf_from_node(udf_node, buf_entry);
                        udf_free_buf_entry(buf_entry);
                    }
                }
            }
            TAILQ_REMOVE(&udf_node->vn_bufs, marker, b_vnbufs);
            free(marker);
        UDF_MUTEX_UNLOCK(&udf_node->buf_mutex);
        UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);

        UDF_MUTEX_LOCK(&udf_node->alloc_mutex);
            if (udf_node->addr_type == UDF_ICB_INTERN_ALLOC) {
                /* shrink intern node */
                too_much = udf_node->stat.st_size - new_extent;
                udf_node->intern_len  -= too_much;
                udf_node->intern_free += too_much;
                memset(udf_node->intern_data + new_extent, 0, too_much);
            } else {
                /* shrink alloc entries _after_ deleting bufs to make sure we are not crossed */
                error = udf_node_release_extent(udf_node, block_extent, udf_node->stat.st_size);

                /* after `cleanup', trim now unused entries */
                udf_merge_allocentry_queue(&udf_node->alloc_entries, lb_size);

                if (new_extent == 0) {
                    /* remove all; most common case too */
                    while ((alloc_entry = TAILQ_FIRST(&udf_node->alloc_entries))) {
                        TAILQ_REMOVE(&udf_node->alloc_entries, alloc_entry, next_alloc);
                        free(alloc_entry);
                    }
                    cur_extent = 0;
                } else {
                    /* move code to udf_allocentries.c ? */
                    udf_cut_allocentry_queue(&udf_node->alloc_entries, lb_size, block_extent);

                    /* find cut-point */
                    cur_extent = 0;
                    TAILQ_FOREACH(alloc_entry, &udf_node->alloc_entries, next_alloc) {
                        cur_extent += alloc_entry->len;
                        if (cur_extent == block_extent) break;
                    }
                    cut_point = alloc_entry;	/* all after this point need to be deleted */

                    assert(cut_point);
                    while ((alloc_entry = TAILQ_NEXT(cut_point, next_alloc))) {
                        TAILQ_REMOVE(&udf_node->alloc_entries, alloc_entry, next_alloc);
                        free(alloc_entry);
                    }

                    /* clip last entry to the correct size */
                    if (new_extent < block_extent) {
                        assert(too_much == block_extent - new_extent);

                        alloc_entry = TAILQ_LAST(&udf_node->alloc_entries, udf_alloc_entries);
                        assert(alloc_entry->len > too_much);

                        alloc_entry->len -= too_much;
                        cur_extent -= too_much;
                    }
                }
                assert(cur_extent == new_extent);
            }
            udf_node->stat.st_size = new_extent;
        UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);
    }

    return 0;
}


/* VOP_WRITE */
int udf_write_file_part_uio(struct udf_node *udf_node, char *what, int content, struct uio *data_uio) {
    struct udf_buf  *buf_entry;
    off_t		 offset;
    uint64_t	 new_possible_extent;
    uint64_t	 start_extent, end_extent;
    uint64_t	 lb_size, sector, data_length;
    uint8_t		*base;
    int		 error, appending, allocated;

    if (!udf_node) return EINVAL;

    if (udf_open_logvol(udf_node->udf_log_vol))
        return EROFS;

    /* write chanches both modification and file status times */
    udf_set_timespec_now(&udf_node->stat.st_ctimespec);
    udf_set_timespec_now(&udf_node->stat.st_mtimespec);

    /* Zero length write -> finished */
    if (data_uio->uio_resid == 0) return 0;

    /* TODO lock node directly to avoid multiple writers entering */
/*	pthread_rwlock_wrlock(&udf_node->udf_node_lock); */

    if (!udf_node->dirty) {
        udf_node_mark_dirty(udf_node);
    }

    /* auto-extent file */
    appending = 0;
    allocated = 0;
    new_possible_extent = data_uio->uio_offset + data_uio->uio_resid;
    if (new_possible_extent >= (uint64_t) udf_node->stat.st_size) {
        /* BUGALERT: extra space can be pre-allocated AFTER file length */
        error = udf_truncate_node(udf_node, new_possible_extent);
        appending = 1;
    }

    lb_size = udf_node->udf_log_vol->lb_size;
    while (data_uio->uio_resid) {
        error = 0;
        sector  = data_uio->uio_offset / lb_size;

        /* lookup sector in buffer set */
        UDF_MUTEX_LOCK(&udf_bufcache->bufcache_lock);
            udf_lookup_node_buf(udf_node, sector, &buf_entry);

            if (!buf_entry || (buf_entry && (buf_entry->b_lblk != sector))) {
                /* not found in cache; `page in' sector from file BUT ONLY if we don't completely overwrite it anyway */
                if ((data_uio->uio_resid < lb_size) && (!appending)) {
                    DEBUG(printf("Reading in file buffer for %s for %"PRIu64" bytes\n", what, (uint64_t) data_uio->uio_resid));
                    error = udf_readin_file_buffer(udf_node, what, sector, content, &buf_entry);
                }

                /* check if the extent is allocated; check assumption that the size of a buffer is a lb_size */
                if (buf_entry)
                    assert(buf_entry->b_bufsize == lb_size);
                UDF_MUTEX_LOCK(&udf_node->alloc_mutex);
                    start_extent = (uint64_t) lb_size * sector;
                    end_extent   = MIN((uint64_t) udf_node->stat.st_size, start_extent + lb_size);
                    error = udf_extent_properties(&udf_node->alloc_entries, lb_size, start_extent, end_extent, &allocated);
                UDF_MUTEX_UNLOCK(&udf_node->alloc_mutex);

                /* check free space if space is to allocated */
                if (!buf_entry || !allocated) {
                    /* be coulant on metadata here to avoid not to easy to solve resource problems */
                    if (content == UDF_C_USERDATA) {
                        /* check for space no space anymore for userdata (bit on the safe side really) */
                        assert(udf_node->udf_log_vol);
                        if (!udf_confirm_freespace(udf_node->udf_log_vol, content, lb_size)) {
                            UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);
                            return ENOSPC;
                        }
                    }
                }
                DEBUG(if (allocated) printf("Writing pre-allocated buffer\n"));

                if (!buf_entry) {
                    /* create new buffer */
                    error = udf_get_buf_entry(udf_node, &buf_entry);
                    if (error) {
                        UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);
                        return error;
                    }

                    /* don't forget to set the relative block number! */
                    buf_entry->b_lblk   = sector;

                    /* add it to the buffer list */
                    UDF_MUTEX_LOCK(&udf_node->buf_mutex);
                        udf_attach_buf_to_node(udf_node, buf_entry);
                    UDF_MUTEX_UNLOCK(&udf_node->buf_mutex);

                }
                assert(buf_entry);

                if (allocated) {
                    /*
                     * we could free the old allocated extent and mark it needing allocation
                     * again
                     */
                }
                if (!allocated) {
                    /* mark it needs to be allocated, locks are not very nice here */
                    UDF_MUTEX_LOCK(&udf_node->buf_mutex);
                        udf_mark_buf_needing_allocate(udf_node, buf_entry);
                    UDF_MUTEX_UNLOCK(&udf_node->buf_mutex);
                }
            }
            if (error) {
                UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);
                break; /* while */
            }

            assert(buf_entry);
            offset = data_uio->uio_offset - (off_t) sector*lb_size;
            base   = buf_entry->b_data;

            assert(offset >= 0);
            if (offset >= 0) {
                UDF_MUTEX_LOCK(&udf_node->buf_mutex);
                    udf_mark_buf_dirty(udf_node, buf_entry);

                    data_length = buf_entry->b_bufsize - offset;
                    data_length = MIN(data_length, data_uio->uio_resid);

                    uiomove(base + offset, data_length, data_uio);

                    buf_entry->b_bcount = MAX(buf_entry->b_bcount, offset + data_length);
                    buf_entry->b_resid  = buf_entry->b_bufsize - buf_entry->b_bcount;
                UDF_MUTEX_UNLOCK(&udf_node->buf_mutex);
            }
        UDF_MUTEX_UNLOCK(&udf_bufcache->bufcache_lock);

        if (data_uio->uio_resid == 0) return 0;		/* finished? */
    } /* while */

    return 0;
}


/******************************************************************************************
 *
 * UDF volume and descriptor logic
 *
 ******************************************************************************************/


struct udf_volumeset *udf_search_volumeset(char *volset_id) {
    struct udf_volumeset *volumeset;
    struct udf_pri_vol   *primary;

    /* XXX this is a bit ugly XXX */
    SLIST_FOREACH(volumeset, &udf_volumeset_list, next_volumeset) {
        primary = STAILQ_FIRST(&volumeset->primaries);
        assert(primary->pri_vol);
        if (memcmp(primary->pri_vol->volset_id, volset_id, 128) == 0) return volumeset;
    }
    return NULL;
}


struct udf_pri_vol *udf_search_primary(struct udf_volumeset *set, char *id) {
    struct udf_pri_vol *primary;

    STAILQ_FOREACH(primary, &set->primaries, next_primary) {
        assert(primary->pri_vol);
        if (memcmp(primary->pri_vol->vol_id, id, 32) == 0) return primary;
    }
    return NULL;
}


struct udf_log_vol *udf_search_logical_volume_in_primary(struct udf_pri_vol *primary, char *logvol_id) {
    struct udf_log_vol *here;

    SLIST_FOREACH(here, &primary->log_vols, next_logvol) {
        if (memcmp(here->log_vol->logvol_id, logvol_id, 128) == 0) return here;
    }
    return NULL;
}



/* called not very often ... ; free incomming when not needed */
int udf_proc_pri_vol(struct udf_session *udf_session, struct udf_pri_vol **current, struct pri_vol_desc *incomming) {
    struct udf_volumeset *volset;
    struct udf_pri_vol   *primary;

    assert(current);
    volset = udf_search_volumeset(incomming->volset_id);
    if (!volset) {
        /* create a volume set */
        volset = calloc(1, sizeof(struct udf_volumeset));
        if (!volset) {
            free(incomming);
            return ENOMEM;
        }

        /* populate and link in */
        volset->max_partnum = 0;
        STAILQ_INIT(&volset->primaries);
        SLIST_INSERT_HEAD(&udf_volumeset_list, volset, next_volumeset);
    }
    assert(volset);

    primary = udf_search_primary(volset, incomming->vol_id);
    *current = primary;

    if (!primary) {
        /* create a primary volume */
        primary = calloc(1, sizeof(struct udf_pri_vol));
        if (!primary) {
            free(incomming);
            return ENOMEM;
        }

        /* add to volset's primaries list */
        STAILQ_INSERT_TAIL(&volset->primaries, primary, next_primary);

        *current = primary;
    } else {
        /* mark as current */

        /* ok ... we now need to check if this new descriptor is a newer version */
        if (udf_rw32(incomming->seq_num) <= udf_rw32(primary->pri_vol->seq_num)) {
            if (udf_session->session_num <= (*current)->udf_session->session_num) {
                DEBUG(printf("UDF: DISCARDING primary descriptor for its the same but higher session number\n"));
                /* its an older one; ignore */
                free(incomming);
                return 0;
            }
        }
        DEBUG(printf("UPDATING primary descriptor for it has a higher session number\n"));
    }

    /* update the primary volume descriptor */
    if (primary->pri_vol) free(primary->pri_vol);
    primary->volumeset   = volset;
    primary->pri_vol     = incomming;
    primary->udf_session = udf_session;

    return 0;
}


/* not called often ; free lvid when not used */
int udf_proc_logvol_integrity(struct udf_log_vol *udf_log_vol, struct logvol_int_desc *new_lvid) {
    struct udf_logvol_info *impl;
    uint64_t psize, pfree;
    uint32_t *free_space_pos, *size_pos;
    uint32_t lb_size, part_map;
    uint32_t integrity;
    int error, tagid;

    error = udf_check_tag((union dscrptr *) new_lvid);
    if (error) {
        return error;	/* return error on faulty tag */
    }

    tagid = udf_rw16(new_lvid->tag.id);
    /* getting a terminator tag or zero is an OK condition */
    if ((tagid == TAGID_TERM) || (tagid == 0)) {
        return 0;
    }

    /* not getting an logical volume itegrity volume descriptor is an error now */
    if (tagid != TAGID_LOGVOL_INTEGRITY) {
        printf("IEE! got a %d tag while searching for a logical volume integrity descriptor\n", tagid);
        return EINVAL;	/* XXX error code? XXX */
    }

    /* check CRC on the contents of the logvol integrity */
    error = udf_check_tag_payload((union dscrptr *) new_lvid);
    if (error) {
        return error;
    }

    /* check logvol integrity validity */
    integrity = udf_rw32(new_lvid->integrity_type);
    if ((integrity != UDF_INTEGRITY_OPEN) && (integrity != UDF_INTEGRITY_CLOSED))
        return EINVAL;

    /* allways go for the next in line; silly but thats Ecma-167/UDF */
    /* process information contained in logical volume integrity descriptor */
    udf_log_vol->logvol_state     = integrity;
    udf_log_vol->integrity_serial = udf_rw16(new_lvid->tag.serial_num);
    impl = (struct udf_logvol_info *) (new_lvid->tables + 2*udf_rw32(new_lvid->num_part));

    udf_log_vol->min_udf_readver  = udf_rw16(impl->min_udf_readver);
    udf_log_vol->min_udf_writever = udf_rw16(impl->min_udf_writever);
    udf_log_vol->max_udf_writever = udf_rw16(impl->max_udf_writever);

    udf_log_vol->num_files        = udf_rw32(impl->num_files);
    udf_log_vol->num_directories  = udf_rw32(impl->num_directories);
    udf_log_vol->next_unique_id   = udf_rw64(new_lvid->lvint_next_unique_id);

    /* calculate free space from this integrity descritor */
    lb_size = udf_log_vol->lb_size;

    /* init start positions */
    free_space_pos = &new_lvid->tables[0];
    size_pos       = &new_lvid->tables[udf_log_vol->num_part_mappings];

    /* init counters */
    udf_log_vol->total_space = udf_log_vol->free_space = udf_log_vol->await_alloc_space = 0;
    for (part_map = 0; part_map < udf_log_vol->num_part_mappings; part_map++) {
        psize = udf_rw32(*size_pos); size_pos++;
        pfree = udf_rw32(*free_space_pos); free_space_pos++;
        if (pfree != UINT_MAX) {
            /* if UINT_MAX, its not applicable like virtual space partitions */
            udf_log_vol->total_space += (uint64_t) psize * lb_size;
            udf_log_vol->free_space  += (uint64_t) pfree * lb_size;
        }
    }

    UDF_VERBOSE(
        if (udf_log_vol->logvol_state == UDF_INTEGRITY_OPEN) {
            udf_dump_timestamp("\t\t\t\tmarked open   at ", &new_lvid->time);
        } else {
            udf_dump_timestamp("\t\t\t\tmarked closed at ", &new_lvid->time);
        }
    );
    return 0;
}


void udf_derive_new_logvol_integrity(struct udf_log_vol *udf_log_vol) {
    udf_log_vol->logvol_state     = UDF_INTEGRITY_OPEN;
    udf_log_vol->integrity_serial = 1;

    /* analyse the log vol to check out minimum and maximum read/write versions */
    if (udf_rw16(udf_log_vol->log_vol->tag.descriptor_ver) == 2) {
        udf_log_vol->min_udf_readver  = 0x0102;
        udf_log_vol->min_udf_writever = 0x0150;
        udf_log_vol->max_udf_writever = 0x0150;
    } else {
        udf_log_vol->min_udf_readver  = 0x0201;
        udf_log_vol->min_udf_writever = 0x0201;
        udf_log_vol->max_udf_writever = 0x0201;		/* 2.50? */
    }
    udf_log_vol->num_files       = 0;
    udf_log_vol->num_directories = 0;
    udf_log_vol->next_unique_id  = 16;	/* zero first, rest 15/16+ minimum */
}


int udf_proc_logvol_integrity_sequence(struct udf_log_vol *udf_log_vol) {
    union dscrptr	*dscr;
    uint32_t	 sector, length, lvid_len, num_sectors;
    uint32_t	 lb_size;
    int		 error;

    sector  = udf_rw32(udf_log_vol->log_vol->integrity_seq_loc.loc);
    length  = udf_rw32(udf_log_vol->log_vol->integrity_seq_loc.len);
    lb_size = udf_log_vol->lb_size;

    /* go for the default `open' integrity first as initialisation */
    udf_derive_new_logvol_integrity(udf_log_vol);

    if (!length) {
        fprintf(stderr, "UDF: no volume integrity descriptor sequence space defined... OK for Ecma-167, not for UDF; rejecting\n");
        return EBADF;
    }

    error = 0;
    while (length) {
        error = udf_read_session_descriptor(udf_log_vol->primary->udf_session, sector, "Logical volume integrity descriptor (LVID)", &dscr, &lvid_len);
        if (error) {
            if (dscr) free(dscr);
            dscr = NULL;
            break;
        }

        UDF_VERBOSE_MAX(udf_dump_descriptor(dscr));

        error = udf_proc_logvol_integrity(udf_log_vol, &dscr->lvid);
        if (error) break;
        if (udf_rw16(dscr->tag.id) == TAGID_TERM) break;

        num_sectors = (lvid_len + lb_size-1) / lb_size;
        length -= num_sectors * lb_size;
        sector += num_sectors;

        if (udf_rw32(dscr->lvid.next_extent.len)) {
            sector = udf_rw32(dscr->lvid.next_extent.loc);
            length = udf_rw32(dscr->lvid.next_extent.len);
        }
        /* free consumed descriptor */
        free(dscr);
        dscr = NULL;
    }
    /* free dangling descriptor */
    if (dscr) free(dscr);

    /* either an error has occured or we have processed all descriptors */
    if (error) {
        fprintf(stderr, "WARNING: integrity sequence ended with a bad descriptor; creating new\n");
        udf_derive_new_logvol_integrity(udf_log_vol);
        return ENOENT;
    }

    /*
     * If its marked closed we hope/assume all is fine otherwise it may be
     * marked closed later on when we are using a VAT and its found and
     * correct
     */
    return 0;
}


/* Add partition mapping to specified logvol descriptor */
void udf_add_physical_to_logvol(struct logvol_desc *logvol, uint16_t vol_seq_num, uint16_t phys_part_num) {
    union  udf_pmap *pmap;
    uint8_t         *pmap_pos;

    pmap_pos = logvol->maps + udf_rw32(logvol->mt_l);

    pmap = (union udf_pmap *) pmap_pos;
    pmap->pm1.type        = 1;
    pmap->pm1.len         = sizeof(struct part_map_1);
    pmap->pm1.vol_seq_num = udf_rw16(vol_seq_num);
    pmap->pm1.part_num    = udf_rw16(phys_part_num);

    /* increment partition mapping count */
    logvol->n_pm = udf_rw32(udf_rw32(logvol->n_pm) + 1);
    logvol->mt_l = udf_rw32(udf_rw32(logvol->mt_l) + sizeof(struct part_map_1));

    logvol->tag.desc_crc_len = udf_rw16(udf_rw16(logvol->tag.desc_crc_len) + sizeof(struct part_map_1));
}


#if 0
/* not yet */
void udf_add_sparable_to_logvol(struct logvol_desc *logvol, uint16_t vol_seq_num, uint16_t phys_part_num, uint16_t packet_len, ) {
    union  udf_pmap *pmap;
    uint8_t         *pmap_pos;

    pmap_pos = logvol->maps + udf_rw32(logvol->mt_l);

    pmap = (union udf_pmap *) pmap_pos;
    pmap->pm1.type        = 1;
    pmap->pm1.len         = sizeof(struct part_map_1);
    pmap->pm1.vol_seq_num = vol_seq_num;
    pmap->pm1.part_num    = phys_part_num;

    /* increment partition mapping count */
    logvol->n_pm = udf_rw32(udf_rw32(logvol->n_pm) + 1);
    logvol->mt_l = udf_rw32(udf_rw32(logvol->mt_l) + sizeof(struct part_map_1));

    logvol->tag.desc_crc_len = udf_rw16(udf_rw16(logvol->tag.desc_crc_len) + sizeof(struct part_map_1));
}
#endif


/* not called often; free incomming when not needed */
int udf_proc_log_vol(struct udf_pri_vol *primary, struct udf_log_vol **current, struct logvol_desc *incomming) {
    struct udf_log_vol	 *logical;
    struct udf_part_mapping	 *part_mapping, *data_part_mapping, *search_part_mapping;
    union  udf_pmap		 *pmap;
    uint32_t		  part_cnt, pmap_type, pmap_size;
    uint32_t		  data_part_num;
    uint8_t 		 *pmap_pos;

    logical = udf_search_logical_volume_in_primary(primary, incomming->logvol_id);
    if (!logical) {
        /* create a logical volume */
        logical = calloc(1, sizeof(struct udf_log_vol));
        if (!logical) {
            free(incomming);
            return ENOMEM;
        }

        /* link in */
        SLIST_INSERT_HEAD(&primary->log_vols, logical, next_logvol);
    } else {
        /* ok ... we now need to check if this new descriptor is a newer version */
        if (udf_rw32(incomming->seq_num) < udf_rw32(logical->log_vol->seq_num)) {
            /* its an older one; ignore */
            free(incomming);
            return 0;
        }
    }

    /* update the logical volume descriptor and its mappings; first delete old partition mappings allocated before */
    logical->primary  = primary;
    if (current) *current = logical;

    part_mapping = SLIST_FIRST(&logical->part_mappings);
    while ((part_mapping = SLIST_FIRST(&logical->part_mappings))) {
        /* TODO cleanup old cruft ? (XXX while mounted? i don't think so!) */
        /*
            free(part_mapping->sparing_table);
            free(part_mapping->vat_file_entry);
            free(part_mapping->vat);
            free(part_mapping->meta_file);
            free(part_mapping->meta_mirror_file);
            free(part_mapping->meta_bitmap_file);
        */
        SLIST_REMOVE_HEAD(&logical->part_mappings, next_mapping);
        free(part_mapping);
    }
    SLIST_INIT(&logical->part_mappings);

    /* use the new logical volume and preprocess it */
    if (logical->log_vol) free(logical->log_vol);
    logical->log_vol = incomming;
    logical->lb_size     = udf_rw32(incomming->lb_size);
    logical->sector_size = primary->udf_session->disc->sector_size;

    /* build up the partion mappings */
    logical->num_part_mappings = udf_rw32(incomming->n_pm);

    /* process partition mappings */
    pmap_pos = &logical->log_vol->maps[0];
    for (part_cnt = 0; part_cnt < logical->num_part_mappings; part_cnt++) {
        /* get a new part_mapping structure */
        part_mapping = calloc(1, sizeof(struct udf_part_mapping));
        assert(part_mapping);		/* XXX check with partition mapping destructor etc XXX */

        /*
         * BUG alert: add to *tail* of list for dependencies sake.
         * Since this is the only place that its needed, I decided
         * against changing the SLIST to a TAILQ.
         */
        if (SLIST_EMPTY(&logical->part_mappings))
            SLIST_INSERT_HEAD(&logical->part_mappings, part_mapping, next_mapping);
        else {
            search_part_mapping = SLIST_FIRST(&logical->part_mappings);
            while (SLIST_NEXT(search_part_mapping, next_mapping))
                search_part_mapping = SLIST_NEXT(search_part_mapping, next_mapping);
            SLIST_INSERT_AFTER(search_part_mapping, part_mapping, next_mapping);
        }

        /* process */
        pmap = (union udf_pmap *) pmap_pos;
        pmap_type = pmap->data[0];
        pmap_size = pmap->data[1];

        part_mapping->udf_virt_part_num = part_cnt;
        part_mapping->udf_pmap = pmap;
        switch (pmap_type) {
            case 1:
                part_mapping->udf_part_mapping_type = UDF_PART_MAPPING_PHYSICAL;
                part_mapping->vol_seq_num = udf_rw16(pmap->pm1.vol_seq_num);
                part_mapping->udf_phys_part_num = udf_rw16(pmap->pm1.part_num);
                break;
            case 2:
                part_mapping->vol_seq_num = udf_rw16(pmap->pm2.vol_seq_num);
                part_mapping->udf_phys_part_num = udf_rw16(pmap->pm2.part_num);
                if (strncmp((char *) pmap->pm2.part_id.id, "*UDF Virtual Partition", UDF_REGID_ID_SIZE) == 0) {
                    part_mapping->udf_part_mapping_type = UDF_PART_MAPPING_VIRTUAL;
                    break;
                }
                if (strncmp((char *) pmap->pm2.part_id.id, "*UDF Sparable Partition", UDF_REGID_ID_SIZE) == 0) {
                    part_mapping->udf_part_mapping_type = UDF_PART_MAPPING_SPARABLE;
                    break;
                }
                if (strncmp((char *) pmap->pm2.part_id.id, "*UDF Metadata Partition", UDF_REGID_ID_SIZE) == 0) {
                    part_mapping->udf_part_mapping_type = UDF_PART_MAPPING_META;
                    break;
                }
                printf("HELP ... found unsupported type 2 partition mapping id `%s`; marking broken\n", pmap->pm2.part_id.id);
            default:
                part_mapping->udf_part_mapping_type = UDF_PART_MAPPING_ERROR;
        }

        pmap_pos += pmap_size;		/* variable length array :( */
    }

    /* flag all partion mappings data and metadata writable */
    SLIST_FOREACH(part_mapping, &logical->part_mappings, next_mapping) {
        part_mapping->data_writable     = 1;
        part_mapping->metadata_writable = 1;
    }

    /* update writable flags depending on mapping type */
    SLIST_FOREACH(part_mapping, &logical->part_mappings, next_mapping) {
        switch (part_mapping->udf_part_mapping_type) {
            case UDF_PART_MAPPING_ERROR :
                part_mapping->data_writable     = 0;
                part_mapping->metadata_writable = 0;
                break;
            case UDF_PART_MAPPING_PHYSICAL :
                break;
            case UDF_PART_MAPPING_VIRTUAL :
            case UDF_PART_MAPPING_META :
                /*
                 * These are special in that there is a special metadata partition where no data
                 * is meant to be written on and vice versa
                 */

                /* find the associated data partition */
                data_part_num     = part_mapping->udf_phys_part_num;
                SLIST_FOREACH(data_part_mapping, &logical->part_mappings, next_mapping) {
                    if (data_part_mapping->udf_phys_part_num == data_part_num) {
                        if (data_part_mapping != part_mapping) {
                            data_part_mapping->metadata_writable = 0;
                            break;
                        }
                    }
                }
                part_mapping->data_writable = 0;
                break;
            case UDF_PART_MAPPING_SPARABLE :
                break;
        }
    }

    TAILQ_INIT(&logical->dirty_nodes);
    UDF_MUTEX_INIT(&logical->dirty_nodes_mutex);

    return 0;
}


/* not called often; free incomming when not needed */
int udf_proc_part(struct udf_pri_vol *primary, struct udf_partition **current, struct part_desc *incomming) {
    struct udf_partition *udf_partition;
    struct udf_volumeset *udf_volset;
    uint32_t new_part_num, sector_size;

    assert(primary);
    assert(primary->pri_vol);

    udf_volset = udf_search_volumeset(primary->pri_vol->volset_id);
    assert(udf_volset);

    new_part_num = udf_rw16(incomming->part_num);
    /* check if its a partition type we recognize */
    if (strncmp((char *) incomming->contents.id, "+NSR0", 5) != 0) {
        fprintf(stderr, "Unrecognized partition content type %s encountered; ignoring\n", incomming->contents.id);
        free(incomming);
        return 0;
    }

    /* look if we allready got it */

    SLIST_FOREACH(udf_partition, &udf_volset->parts, next_partition) {
        if (udf_rw16(udf_partition->partition->part_num) == new_part_num) break;
    }

    /* we have space... now check if this is a newer one than the one known */
    if (udf_partition) {
        if (udf_rw32(incomming->seq_num) < udf_rw32(udf_partition->partition->seq_num)) {
            /* its an older version */
            free(incomming);
            return 0;
        }
    } else {
        /* get us a new udf_partition */
        udf_partition = calloc(1, sizeof(struct udf_partition));
        if (!udf_partition) {
            free(incomming);
            return ENOMEM;
        }

        /* link it in */
        SLIST_INSERT_HEAD(&udf_volset->parts, udf_partition, next_partition);
    }
    assert(udf_partition);

    /* copy this new partition descriptor in the list */
    if (udf_partition->partition) free(udf_partition->partition);
    udf_partition->partition   = incomming;
    udf_partition->udf_session = primary->udf_session;
    udf_volset->max_partnum = MAX(udf_volset->max_partnum, new_part_num+1);	/* REVIEW why +1? */

    /* initialise */
    sector_size = primary->udf_session->disc->sector_size;
    UDF_MUTEX_INIT(&udf_partition->partition_space_mutex);
    TAILQ_INIT(&udf_partition->unalloc_space_queue);
    TAILQ_INIT(&udf_partition->freed_space_queue);
    udf_partition->part_offset = udf_rw32(incomming->start_loc) * sector_size;
    udf_partition->part_length = udf_rw32(incomming->part_len)  * sector_size;
/*	udf_partition->access_type = udf_rw32(incomming->access_type); */

    udf_partition->free_unalloc_space = udf_partition->free_freed_space = 0;

    if (current) *current = udf_partition;

    return 0;
}


/* not called often; free incomming when not needed */
int udf_proc_filesetdesc(struct udf_log_vol *udf_log_vol, struct fileset_desc *incomming) {
    struct udf_mountpoint *mp;

    if (udf_rw16(incomming->tag.id) != TAGID_FSD) {
        printf("IEEE! Encountered a non TAGID_FSD in this fileset descriptor sequence!!!\n");
        free(incomming);
        return EFAULT;
    }

    /* lookup fileset descriptor in this logical volume; interestingly fileset_num is KEY! */
    SLIST_FOREACH(mp, &udf_log_vol->mountpoints, logvol_next) {
        if (mp->fileset_desc->fileset_num == incomming->fileset_num) break;
    }

    if (!mp) {
        /* add a new mountpoint! */
        mp = calloc(1, sizeof(struct udf_mountpoint));
        if (!mp) {
            free(incomming);
            return ENOMEM;
        }
        mp->fileset_desc = incomming;

        /* insert into udf_log_vol and into mountables list */
        SLIST_INSERT_HEAD(&udf_log_vol->mountpoints, mp, logvol_next);
        SLIST_INSERT_HEAD(&udf_mountables, mp, all_next);
    } else {
        /* should we update mountpoint? */
        if (udf_rw32(incomming->fileset_desc_num) <= udf_rw32(mp->fileset_desc->fileset_desc_num)) {
            /* we allready got a newer one */
            free(incomming);
            return 0;
        }

        fprintf(stderr, "UDF DEBUG: would be updating mountpoint... HELP!\n");
        /* FIXME delete all inode hash entries */
        /* XXX how to do that? inodes OK but associated vnodes? XXX */
#if 0
        if (!SLIST_EMPTY(&mp->inodes)) {
            printf("UDF: asked to delete mountpoint with inodes in hashtable!\n");
            printf("Can't cope with that... aborting\n");
            exit(1);
        }
#endif

        /* free old information (allready in lists though!) */
        free(mp->fileset_desc);
        free(mp->mount_name);
    }

    mp->udf_log_vol  = udf_log_vol;
    mp->fileset_desc = incomming;
    mp->mount_name   = strdup(udf_get_compound_name(mp));

    return 0;
}


int udf_retrieve_volume_space(struct udf_discinfo *disc, struct udf_session *udf_session, struct extent_ad *extent) {
    struct udf_pri_vol *udf_pri_vol;
    struct udf_log_vol *udf_log_vol;
    union dscrptr	*dscr;
    uint32_t	 sector, length, dscr_len, num_sectors;
    uint32_t	 sector_size;
    int		 tag_id;
    int		 error;

    udf_pri_vol = NULL;

    sector = udf_rw32(extent->loc);
    length = udf_rw32(extent->len);
    sector_size = disc->sector_size;

    error = 0;	/* XXX zero length area's possible? XXX */
    while (length) {
        error = udf_read_session_descriptor(udf_session, sector, "volume descriptor", &dscr, &dscr_len);
        if (error) {
            if (dscr) free(dscr);
            break;
        }

        tag_id = udf_rw16(dscr->tag.id);
        num_sectors = (dscr_len + sector_size-1) / sector_size;

        /* proc volume descriptor starting at sector `volume_sector' */
        UDF_VERBOSE_MAX(udf_dump_descriptor(dscr));
        switch (tag_id) {
            case TAGID_PRI_VOL       :
                error = udf_proc_pri_vol(udf_session, &udf_pri_vol, &dscr->pvd);
                break;
            case TAGID_PARTITION     :
                error = udf_proc_part(udf_pri_vol, NULL, &dscr->pd);
                break;
            case TAGID_LOGVOL        :
                error = udf_proc_log_vol(udf_pri_vol, &udf_log_vol, &dscr->lvd);
                if (!error) {
                    /* first create empty integrity descriptor then modify it on input (for sanity) */
                    udf_derive_new_logvol_integrity(udf_log_vol);
                }
                break;
            case TAGID_TERM          :
                free(dscr);
                return 0;	/* terminator */
            case TAGID_UNALLOC_SPACE :
                /* unallocated space descriptor */
                /* Specifies space that is not claimed yet in partitions (!) */
                UDF_VERBOSE(printf("\t\t`unallocated space descriptor' ignored\n"));
                break;
            case TAGID_IMP_VOL       :
                /* implemenation use volume descriptor */
                /* Specifies information relevant for the implementator */
                UDF_VERBOSE_MAX(printf("\t\t`implementation use volume descriptor' ignored\n"));
                break;
            case TAGID_VOL           :
                fprintf(stderr, "UDF : untested volume space extender encountered\n");
                break;
            default :
                printf("XXX Unhandled volume sequence %d; freeing\n", tag_id);
                free(dscr);
                break;
        }

        length -= num_sectors * sector_size;
        sector += num_sectors;

        if (tag_id == TAGID_VOL) {
            sector = udf_rw32(dscr->vdp.next_vds_ex.loc);
            length = udf_rw32(dscr->vdp.next_vds_ex.len);
            free(dscr);
        }
    }

    return error;
}


int udf_get_filelength(union dscrptr *dscr, uint64_t *length) {
    int32_t	fe_tag;

    fe_tag = udf_rw16(dscr->tag.id);
    if (fe_tag == TAGID_FENTRY) {
        *length = udf_rw64(dscr->fe.inf_len);
        return 0;
    } else if (fe_tag == TAGID_EXTFENTRY) {
        *length = udf_rw64(dscr->efe.inf_len);
        return 0;
    }
    return ENOENT;
}


/* can be passed either a file_entry or an extfil_entry trough fentry! */
int udf_check_for_vat(struct udf_log_vol *udf_log_vol, struct udf_part_mapping *part_mapping, uint32_t vat_lb, union dscrptr *dscr) {
    struct udf_part_mapping *s_part_mapping;
    struct udf_node		*vat_udf_node;
    struct long_ad		 udf_icbptr;
    struct regid 		*regid;
    struct uio		 vat_uio;
    struct iovec		 vat_iovec;
    struct icb_tag		*icbtag;
    struct timestamp	*mtime;
    uint64_t		 vat_length, vat_entries;
    uint32_t		*vat_pos, vpart_num;
    uint8_t			*vat;
    int			 error, found;

    /* prepare a `uio' structure for reading in complete VAT file */
    error = udf_get_filelength(dscr, &vat_length);
    if (error) return error;

    if (vat_length == 0)
        return EFAULT;

    vat = malloc(vat_length);
    if (!vat)
        return ENOMEM;

    /* move to uio_newuio(struct uio *uio) with fixed length uio_iovcnt? */
    bzero(&vat_uio, sizeof(struct uio));
    vat_uio.uio_rw     = UIO_WRITE;	/* WRITE into this space */
    vat_uio.uio_iovcnt = 1;
    vat_uio.uio_iov    = &vat_iovec;
    vat_uio.uio_offset = 0;		/* begin at the start */
    vat_uio.uio_resid  = vat_length;
    /* fill in IO vector */
    vat_uio.uio_iov->iov_base = vat;
    vat_uio.uio_iov->iov_len  = vat_length;

    /* find our virtual partition number corresponding to our physical partition number; this sucks */
    found = 0;
    vpart_num = 0;
    SLIST_FOREACH(s_part_mapping, &udf_log_vol->part_mappings, next_mapping) {
        if (s_part_mapping->udf_phys_part_num == part_mapping->udf_phys_part_num) {
            if (s_part_mapping->udf_part_mapping_type == UDF_PART_MAPPING_PHYSICAL) {
                /* found it ! */
                found = 1;
                vpart_num = s_part_mapping->udf_virt_part_num;
            }
        }
    }
    if (!found) {
        printf("Can't find accompanied physical volume\n");
        return ENOENT;
    }

    /* prepare udf_icbptr file node for easy file reading */
    udf_icbptr.loc.part_num	= vpart_num;
    udf_icbptr.loc.lb_num	= udf_rw32(vat_lb);
    udf_icbptr.len		= udf_log_vol->lb_size;	/* not used, but may not be zero */

    /*
     * this udf_node creation and disposing may look a bit inefficient but
     * its beneficiary for normal file access.  its only used once for
     * reading in the VAT.
     */

    /* create the udf_vat_node; anonymous since it can't be in a mountpoint */
    error = udf_readin_anon_udf_node(udf_log_vol, dscr, &udf_icbptr, "VAT", &vat_udf_node);
    if (!error) {
        DEBUG(printf("READ FILE PART UIO for VAT\n"));
        error = udf_read_file_part_uio(vat_udf_node, "VAT contents", 0, &vat_uio);
        DEBUG(printf("vat_uio rest %d\n", (uint32_t) vat_uio.uio_resid));
    }

    /* XXX allow for SHORT VAT's ? XXX */
    if (!error) {
        if (vat_uio.uio_resid) {
            fprintf(stderr, "Warning: VAT file can't be read in completely\n");
        }

        part_mapping->vat_udf_node = vat_udf_node;
        part_mapping->vat          = (struct udf_vat *) vat;
        part_mapping->vat_length   = vat_length;

        /* extract next unique file ID from the VAT file entry's unique ID incremented by one */
        udf_log_vol->next_unique_id = vat_udf_node->unique_id;	/* ok? */
        udf_increment_unique_id(udf_log_vol);

        /* fentry is confirmed to be either an file_entry or an extfile_entry here */
        if (udf_rw16(dscr->tag.id) == TAGID_FENTRY) {
            icbtag = &dscr->fe.icbtag;
            mtime  = &dscr->fe.mtime;

        } else {
            icbtag = &dscr->efe.icbtag;
            mtime  = &dscr->efe.mtime;
        }

        if (icbtag->file_type == UDF_ICB_FILETYPE_VAT) {
            /* we are in UDF 2.00+ userland */
            part_mapping->vat_translation = ((uint8_t *) part_mapping->vat) + udf_rw16(part_mapping->vat->header_len);
            part_mapping->vat_entries     = (vat_length - udf_rw16(part_mapping->vat->header_len))/4;
            udf_log_vol->num_files        = udf_rw32(part_mapping->vat->num_files);
            udf_log_vol->num_directories  = udf_rw32(part_mapping->vat->num_directories);
            udf_log_vol->min_udf_readver  = udf_rw16(part_mapping->vat->min_udf_readver);
            udf_log_vol->min_udf_writever = udf_rw16(part_mapping->vat->min_udf_writever);
            udf_log_vol->max_udf_writever = udf_rw16(part_mapping->vat->max_udf_writever);

            /* TODO update logvol name */
        } else {
            /* still in the old UDF 1.50 userland; update? its notoriously broken */
            /* check the old UDF 1.50 VAT */
            DEBUG(printf("CHECK UDF 1.50 VAT\n"));
            vat_pos     = (uint32_t *) vat;
            vat_entries = (vat_length-36)/4;	/* definition */

            regid = (struct regid *) (vat_pos + vat_entries);
            error = (strncmp((char *) regid->id, "*UDF Virtual Alloc Tbl", 22) == 0) ? 0 : ENOENT;
            if (!error) {
                part_mapping->vat_entries = vat_entries;
                part_mapping->vat_translation = vat;
                part_mapping->vat = NULL;

                /* num files/dirs? */
            }
        }
        if (!error) {
            UDF_VERBOSE(udf_dump_timestamp("\t\t\t\tmarked closed at ", mtime));
        }
    }

    /* clean up uio structure */
    if (error) {
        if (vat) free(vat);
        if (vat_udf_node) udf_dispose_udf_node(vat_udf_node);
        part_mapping->vat_udf_node = NULL;
    }

    return error;
}


int udf_retrieve_supporting_tables(struct udf_log_vol *udf_log_vol) {
    struct udf_partition	 *udf_partition;
    struct udf_part_mapping	 *part_mapping, *s_part_mapping;
    struct udf_session	 *udf_session;
    struct long_ad		  udf_icbptr;
    union  dscrptr		 *possible_vat_fe;
    union  dscrptr		 *sparing_table_dscr;
    uint32_t		  spar_loc;
    uint64_t		  first_vat_loc, vat_loc, last_vat_loc;
    uint32_t		  sector_size, lb_size;
    uint32_t		  part_num, spar_num, data_part_num, vpart_num;
    int			  session_num;
    int			  error;

    /*
     * if there are any virtual or sparable partition in this logical
     * volume, try to find their supporting tables so we can find the rest
     */
    lb_size     = udf_log_vol->lb_size;
    sector_size = udf_log_vol->sector_size;
    SLIST_FOREACH(part_mapping, &udf_log_vol->part_mappings, next_mapping) {
        part_num = part_mapping->udf_virt_part_num;
        udf_logvol_vpart_to_partition(udf_log_vol, part_num, NULL, &udf_partition);

        UDF_VERBOSE_TABLES(printf("\tFor partition mapping %d->%d\n", part_num, part_mapping->udf_phys_part_num));
        switch (part_mapping->udf_part_mapping_type) {
            case UDF_PART_MAPPING_ERROR    :
                /* nothing to be done for these */
                break;
            case UDF_PART_MAPPING_PHYSICAL :
                /* nothing to be done for these; no supporting tables */
                break;
            case UDF_PART_MAPPING_VIRTUAL  :
                /*
                 * we have to find a good VAT at the END of the session. Since VAT's are
                 * only to be used on WORM's and need to written as last, the strategy is
                 * to go for the predicted end of this session and walk UP
                 */
                udf_session = udf_log_vol->primary->udf_session;
                session_num = udf_session->session_num;

                UDF_VERBOSE_TABLES(printf("\t\tSearching for the VAT :\n"));
                if (udf_session->session_length == 0) {
                    UDF_VERBOSE(
                        printf("\t\tThis virtual partition is inaccessible since its its size is not known;\n");
                        printf("\t\tTry to insert the disc in a CD or DVD recordable device to access it.\n");
                    );
                    part_mapping->udf_part_mapping_type = UDF_PART_MAPPING_ERROR;
                    continue;
                }

                if (udf_session->disc->next_writable[session_num]) {
                    last_vat_loc = udf_session->disc->next_writable[session_num];
                } else {
                    last_vat_loc = udf_session->disc->session_end[session_num];
                }
                last_vat_loc += udf_session->disc->blockingnr;

                 /* give some extra slack since sizes are not allways given up correctly */
                first_vat_loc = last_vat_loc - 256; /* 8 blocks of 32 */
                first_vat_loc = MAX(first_vat_loc, (uint64_t) udf_session->disc->session_start[session_num]);

                /* try to find the fileid for the VAT; NOTE that we are reading backwards :( */
                vat_loc = last_vat_loc;
                do {
                    DEBUG(
                        printf("Trying VAT at sector %d in session\n", (int) vat_loc)
                    );
                    error = udf_read_session_descriptor(udf_session, vat_loc, "VAT file entry", &possible_vat_fe, NULL);
                    if (!error) {
                        error = udf_check_tag_presence(possible_vat_fe, TAGID_FENTRY);
                        if (error)
                            error = udf_check_tag_presence(possible_vat_fe, TAGID_EXTFENTRY);
                    }
                    if (!error) error = udf_check_tag_payload( possible_vat_fe);
                    if (!error) error = udf_check_for_vat(udf_log_vol, part_mapping, vat_loc, possible_vat_fe);
                    if (!error) {
                        break;
                    } else {
                        if (possible_vat_fe) free(possible_vat_fe);
                        vat_loc--;
                        if (vat_loc < first_vat_loc) error = EIO;
                    }
                } while (error != EIO);

                if (error) {
                    printf("WARNING: was looking for a VAT but didnt find it; marking logical volume broken\n");
                    part_mapping->udf_part_mapping_type = UDF_PART_MAPPING_ERROR;
                    udf_log_vol->logvol_state = UDF_INTEGRITY_OPEN;
                    udf_log_vol->broken = 1;
                    continue;
                }
                UDF_VERBOSE_TABLES(printf("\t\t\tfound VAT file-entry at device logical sector %d\n", (uint32_t) vat_loc));
                UDF_VERBOSE_TABLES(printf("\t\t\tFound %d byte VAT descriptor+table\n", (uint32_t) part_mapping->vat_length));

                UDF_VERBOSE_TABLES(udf_dump_descriptor(possible_vat_fe));
                UDF_VERBOSE_MAX(udf_dump_vat_table(part_mapping));

                if (part_mapping->vat_translation) {
                    /* the presence of a correct VAT means the logical volume is in a closed state */
                    udf_log_vol->logvol_state = UDF_INTEGRITY_CLOSED;
                    UDF_VERBOSE(printf("\t\t\t\tmarked closed due to presence of VAT\n"));

                    /* XXX update `free' space by requesting the device's free space? XXX */
                    udf_log_vol->free_space = udf_partition->part_offset + udf_partition->part_length - vat_loc*sector_size;
                }

                if (!udf_session->disc->sequential) {
                    UDF_VERBOSE(printf("\t\t\t\tenabling sequential media emulation\n"));
                    udf_session->disc->sequential = 1;
                }
                break;
            case UDF_PART_MAPPING_SPARABLE :
                /* we have to find a good sparing table; address are in device logical blocks */
                udf_session = udf_log_vol->primary->udf_session;

                for(spar_num = 0; spar_num < part_mapping->udf_pmap->pms.n_st; spar_num++) {
                    spar_loc = udf_rw32(part_mapping->udf_pmap->pms.st_loc[spar_num]);

                    /* fetch spar_loc's table ; on THIS session. */
                    error = udf_read_session_descriptor(udf_session, spar_loc, "Sparing table", &sparing_table_dscr, NULL);
                    if (!error) error = udf_check_tag_presence(sparing_table_dscr, TAGID_SPARING_TABLE);
                    if (!error) {
                        UDF_VERBOSE_TABLES(printf("\t\tFound the sparing table\n"));
                        part_mapping->sparing_table = &sparing_table_dscr->spt;
                        break;
                    } else {
                        if (sparing_table_dscr) free(sparing_table_dscr);
                    }
                }
                if (!part_mapping->sparing_table) {
                    part_mapping->udf_part_mapping_type = UDF_PART_MAPPING_ERROR;
                }
                UDF_VERBOSE_TABLES(udf_dump_descriptor((union dscrptr *) part_mapping->sparing_table));
                break;
            case UDF_PART_MAPPING_META :
                /*
                 * set up common locator parts; the files are located inside the `part_num' partion where
                 * this partition is a added layer on.
                 */

                /* find the associated data partition */
                data_part_num     = udf_rw16(part_mapping->udf_pmap->pmm.part_num);

                /* find our virtual partition number corresponding to our physical partition number; this sucks */
                vpart_num = data_part_num;
                SLIST_FOREACH(s_part_mapping, &udf_log_vol->part_mappings, next_mapping) {
                    if (s_part_mapping->udf_phys_part_num == data_part_num) {
                        if (s_part_mapping->udf_part_mapping_type != UDF_PART_MAPPING_META) {
                            /* found it ! */
                            vpart_num = s_part_mapping->udf_virt_part_num;
                            break;
                        }
                    }
                }
                udf_icbptr.loc.part_num = vpart_num;
                udf_icbptr.len          = lb_size;		/* defined as maximum size */

                UDF_VERBOSE_TABLES(printf("Reading metadata partition filedescriptor\n"));
                udf_icbptr.loc.lb_num   = udf_rw32(part_mapping->udf_pmap->pmm.meta_file_lbn);
                error = udf_readin_anon_udf_node(udf_log_vol, NULL, &udf_icbptr, "Metadata partition file descriptor", &part_mapping->meta_file);
                if (error == 0)
                    UDF_VERBOSE_TABLES(udf_dump_descriptor((union dscrptr *) part_mapping->meta_file));

                udf_icbptr.loc.lb_num   = udf_rw32(part_mapping->udf_pmap->pmm.meta_mirror_file_lbn);
                if ((error == 0) && (udf_icbptr.loc.lb_num != (uint32_t) -1)) {
                    UDF_VERBOSE_TABLES(printf("Reading metadata partition mirror filedescriptor\n"));
                    error = udf_readin_anon_udf_node(udf_log_vol, NULL, &udf_icbptr, "Metadata partition mirror file descriptor", &part_mapping->meta_mirror_file);
                    if (error == 0)
                        UDF_VERBOSE_TABLES(udf_dump_descriptor((union dscrptr *) part_mapping->meta_mirror_file));
error = 0; /* XXX ignoring error code for now */
                }

                udf_icbptr.loc.lb_num   = udf_rw32(part_mapping->udf_pmap->pmm.meta_bitmap_file_lbn);
                if ((error == 0) && (udf_icbptr.loc.lb_num != (uint32_t) -1)) {
                    UDF_VERBOSE_TABLES(printf("Reading metadata partition bitmap filedescriptor\n"));
                    error = udf_readin_anon_udf_node(udf_log_vol, NULL, &udf_icbptr, "Metadata partition bitmap file descriptor", &part_mapping->meta_bitmap_file);
                    if (error == 0)
                        UDF_VERBOSE_TABLES(udf_dump_descriptor((union dscrptr *) part_mapping->meta_bitmap_file));
                }

                /* if something is wrong, then mark it as a broken partition */
                if (error) {
                    /* TODO handle read-errors on the meta data and meta data mirror file descriptors. */
                    part_mapping->udf_part_mapping_type = UDF_PART_MAPPING_ERROR;
                }
                break;
        }
    }
    UDF_VERBOSE_TABLES(printf("\n"));
    if (udf_log_vol->broken) return EIO;

    return 0;
}


int udf_retrieve_space_tables(struct udf_log_vol *udf_log_vol) {
    struct udf_partition	 *udf_partition;
    struct udf_part_mapping  *part_mapping;
    struct part_hdr_desc	 *part_hdr_desc;
    union  dscrptr		 *dscrptr;
    uint32_t		  sector;
    uint32_t		  lb_size;
    uint64_t		  length;
    int			  vpart_num, ppart_num;
    int			  error;

    lb_size = udf_log_vol->lb_size;
    SLIST_FOREACH(part_mapping, &udf_log_vol->part_mappings, next_mapping) {
        vpart_num = part_mapping->udf_virt_part_num;
        ppart_num = part_mapping->udf_phys_part_num;
        UDF_VERBOSE_TABLES(printf("\tFor partition mapping %d->%d\n", vpart_num, ppart_num));

        if ((part_mapping->udf_part_mapping_type != UDF_PART_MAPPING_PHYSICAL) &&
            (part_mapping->udf_part_mapping_type != UDF_PART_MAPPING_SPARABLE)) {
            UDF_VERBOSE_TABLES(printf("\t\tDon't know how to load space tables for type %d\n", part_mapping->udf_part_mapping_type));
            continue;
        }

        /* retrieve and process unallocated- and freed-space information for all used partitions of the logvol */
        error = udf_logvol_vpart_to_partition(udf_log_vol, vpart_num, NULL, &udf_partition);
        assert(udf_partition);
        part_hdr_desc = &udf_partition->partition->pd_part_hdr;

        sector   = udf_rw32(part_hdr_desc->unalloc_space_table.lb_num);
        length   = udf_rw32(part_hdr_desc->unalloc_space_table.len);		/* needed? */
        if (length) {
            error = udf_read_logvol_descriptor(udf_log_vol, vpart_num, sector, "Unallocated space table", &dscrptr, NULL);
            UDF_VERBOSE_MAX(printf("\tUnalloced space table\n"));
            UDF_VERBOSE_MAX(udf_dump_descriptor(dscrptr));
            /* udf_process_space_table(&udf_partition->unalloc_space, dscrptr); */
            free(dscrptr);
        }

        sector   = udf_rw32(part_hdr_desc->unalloc_space_bitmap.lb_num);
        length   = udf_rw32(part_hdr_desc->unalloc_space_bitmap.len);
        if (length && (udf_partition->unalloc_space_bitmap == 0)) {
            error = udf_read_logvol_descriptor(udf_log_vol, vpart_num, sector, "Unallocated space bitmap", &dscrptr, NULL);
            if (!error) {
                UDF_VERBOSE_MAX(printf("\tUnalloced space bitmap\n"));
                UDF_VERBOSE_MAX(udf_dump_descriptor(dscrptr));
                udf_read_in_space_bitmap(&udf_partition->unalloc_space_queue, &dscrptr->sbd, lb_size, &udf_partition->free_unalloc_space);
                UDF_VERBOSE_TABLES(printf("\t\tPhysical partition's unallocated space : %"PRIu64"\n", udf_partition->free_unalloc_space));
                udf_partition->unalloc_space_bitmap = &dscrptr->sbd;
            } else {
                printf("While reading in unallocated space bitmap : %s\n", strerror(error));
                udf_partition->unalloc_space_bitmap = NULL;
                /* TODO mark read-only logvol */
            }
        }

        sector    = udf_rw32(part_hdr_desc->freed_space_table.lb_num);
        length    = udf_rw32(part_hdr_desc->freed_space_table.len);
        if (length) {
            error = udf_read_logvol_descriptor(udf_log_vol, vpart_num, sector, "Freed space table", &dscrptr, NULL);
            UDF_VERBOSE_MAX(printf("\tFreed space table\n"));
            UDF_VERBOSE_MAX(udf_dump_descriptor(dscrptr));
            /* udf_process_space_table(&udf_partition->freed_space, dscrptr); */
            free(dscrptr);
        }

        sector    = udf_rw32(part_hdr_desc->freed_space_bitmap.lb_num);
        length    = udf_rw32(part_hdr_desc->freed_space_bitmap.len);
        if (length && (udf_partition->freed_space_bitmap == NULL)) {
            error = udf_read_logvol_descriptor(udf_log_vol, vpart_num, sector, "Freed space bitmap", &dscrptr, NULL);
            if (!error) {
                UDF_VERBOSE_MAX(printf("\tFreed space bitmap\n"));
                UDF_VERBOSE_MAX(udf_dump_descriptor(dscrptr));
                udf_read_in_space_bitmap(&udf_partition->freed_space_queue, &dscrptr->sbd, lb_size, &udf_partition->free_freed_space);
                UDF_VERBOSE_TABLES(printf("\t\tPhysical partition's freed space : %"PRIu64"\n", udf_partition->free_unalloc_space));
                udf_partition->freed_space_bitmap = &dscrptr->sbd;
            } else {
                printf("While reading in freed space bitmap : %s\n", strerror(error));
                udf_partition->freed_space_bitmap = NULL;
                /* TODO mark read-only logvol */
            }
        }
    }
    UDF_VERBOSE_TABLES(printf("\n"));

    return 0;
}


/*
 * Fileset descriptors are on a logical volume's partitions; since virtual
 * partitions are then also possible its OK to use the VAT for redefining the
 * fileset descriptors.
 */
int udf_retrieve_fileset_descriptor(struct udf_log_vol *udf_log_vol) {
    struct udf_mountpoint	*mountable;
    struct long_ad		*fsd_loc;
    struct fileset_desc	*new_fsd;
    struct udf_node		*vnode;
    uint32_t		 part_num, lb_num, length;
    int32_t			 error;

    error = 0; /* flag OK */

    fsd_loc   = &udf_log_vol->log_vol->_lvd_use.fsd_loc;
    part_num  = udf_rw16(fsd_loc->loc.part_num);
    lb_num    = udf_rw32(fsd_loc->loc.lb_num);
    length    = udf_rw32(fsd_loc->len);

    while (length && !error) {
        UDF_VERBOSE_TABLES(
            printf("\tFileset descriptor extent at sector %d within partion %d for %d bytes\n", lb_num, part_num, length)
        );

        /* only go for ONE fsb at a time */
        error = udf_read_logvol_descriptor(udf_log_vol, part_num, lb_num, "Fileset descriptor", (union dscrptr **) &new_fsd, NULL);
        if (!error) error = udf_check_tag((union dscrptr *) new_fsd);

        /* TODO need a clearer handling unrecorded blocks here */
        if (error || (!new_fsd) || (new_fsd && (udf_rw16(new_fsd->tag.id) == TAGID_TERM))) {
            /* end of sequence */
            UDF_VERBOSE_TABLES(
                printf("\t\t(Terminator) ");
                if (!new_fsd || error) printf("; unrecorded"); else printf("; explicit");
                printf("\n");
            );
            /* clear error to indicate end of sequence and free possible read in descriptor */
            error = 0;
            if (new_fsd) free(new_fsd);
            break;
        }

        UDF_VERBOSE_MAX(udf_dump_descriptor((union dscrptr *) new_fsd));
        udf_proc_filesetdesc(udf_log_vol, new_fsd);

        if (udf_rw32(new_fsd->next_ex.len) == 0) {
            /* next entry */
            lb_num += 1;
            length -= udf_log_vol->lb_size;
        } else {
            /* follow the next extent */
            fsd_loc  = &new_fsd->next_ex;
            part_num = udf_rw16(fsd_loc->loc.part_num);
            lb_num   = udf_rw32(fsd_loc->loc.lb_num);
            length   = udf_rw32(fsd_loc->len);
        }
    }
    UDF_VERBOSE_TABLES(printf("\n"));

    if (error) return error;

    /* if no error occured, create rootdir udf_nodes */
    SLIST_FOREACH(mountable, &udf_log_vol->mountpoints, logvol_next) {
        /* errors are OK */
        udf_readin_anon_udf_node(udf_log_vol, NULL, &mountable->fileset_desc->rootdir_icb,   "Rootdir",   &mountable->rootdir_node);
        udf_readin_anon_udf_node(udf_log_vol, NULL, &mountable->fileset_desc->streamdir_icb, "Streamdir", &mountable->streamdir_node);

        /* keep names the same ? (duplicate code ahead ... ) */
        if (mountable->rootdir_node) {
            vnode = mountable->rootdir_node;

            vnode->mountpoint = mountable;
            vnode->stat.st_uid = vnode->stat.st_gid = UINT_MAX;
            vnode->stat.st_mode = 0777 | S_IFDIR;

            udf_insert_node_in_hash(vnode);
        }
        if (mountable->streamdir_node) {
            vnode = mountable->streamdir_node;

            vnode->mountpoint = mountable;
            vnode->stat.st_uid = vnode->stat.st_gid = UINT_MAX;
            vnode->stat.st_mode = 0777 | S_IFDIR;

            udf_insert_node_in_hash(vnode);
        }
    }

    return 0;
}


int udf_check_writable_filesets(struct udf_log_vol *udf_log_vol, int mnt_flags) {
    struct udf_mountpoint *mp;
    struct udf_part_mapping *udf_part_mapping;
    int writable;

    writable = 1;
    if (mnt_flags & UDF_MNT_RDONLY)
        writable = 0;

    if (mnt_flags & UDF_MNT_FORCE)
        writable = 1;

    if (udf_log_vol->logvol_state == UDF_INTEGRITY_OPEN) {
        if (!(mnt_flags & UDF_MNT_FORCE)) {
            /* we explicitly DISABLE writing */
            /* XXX do we even reach here? XXX */
            if (udf_verbose) {
                printf("\t\t\t\tmounting READ-ONLY due to open integrity\n");
            } else {
                printf("WARNING: mounting logical volume READ-ONLY due to open integrity\n");
            }
            writable = 0;
        } else {
            printf("WARNING: ignoring open integrity\n");
        }
    }

    SLIST_FOREACH(udf_part_mapping, &udf_log_vol->part_mappings, next_mapping) {
        if (udf_part_mapping->udf_part_mapping_type == UDF_PART_MAPPING_META) {
            writable = 0;
            fprintf(stderr, "\t\t\t\t*** marked read-only due to read-only support for Metadata partition ***\n");
        }
    }

    /* follow all mountpoints of this logical volume and set if they are writable */
    SLIST_FOREACH(mp, &udf_log_vol->mountpoints, logvol_next) {
        mp->writable = writable;
    }
    udf_log_vol->writable = writable;

    /* WAS: */
    /* udf_log_vol->primary->udf_session->writable = mark; */
    return 0;
}


/*
 * udf_eliminate_predescessor_volumesets()
 *
 * We are faced with a curious problem : we are to examine the partitions and
 * determine which are successors of eachother.  This is propably most
 * relevant only on WORM media though. We could consider the following rules :
 * 	1) `glue' according to strict UDF level 1 rules ?
 * 	2) use heuristics info i.e. NERO, DirectCD and mkisofs quirks ?
 * 	3) use overlapping partitions to detect relationships ?
 *
 * Using 1 would imply no multi-volume discs and thus glue everything but that
 * could easily be wrong. Selecting by volumeset names is not possible for
 * Nero f.e. just creates random volumeset names every session and uses no
 * volume version information and thus also violates the UDF rules.
 *
 * Using 2 would be tricky; we know a few programs but what if more are
 * developped? We then would be at loss.
 *
 * Using 3 would imply some calculation but is fail-safe in both supporting
 * multiple volumes on one disc (they seperate) and in supporting
 * multi-session WORM media for these will refer to eachother. Offcource NERO
 * could faul this by just extending the zero partion to the whole disc in its
 * ignorance and thus create false overlapping over other independent
 * sessions. This is to be investigated. I don't know how NERO will react on
 * this situation.
 *
 * Propably method 3 would be good to try :
 *
 * Follow the disc and check for all sessions in order to mark the ones with
 * overlapping partitions as `inactive' and keep the latest one active.
 * Sessions with the `local' quirk are seperate allmost by default; should
 * be change the offsets? would not be too difficult but possible.
 */

void udf_eliminate_predescessor_volumesets(struct udf_discinfo *disc) {
    struct udf_volumeset	*anc_vol_set;
    struct udf_volumeset	*sib_vol_set;
    struct pri_vol_desc	*anc_pri_vol;
    struct pri_vol_desc	*sib_pri_vol;
    struct udf_partition	*anc_part;
    struct udf_partition	*sib_part;
    int			 anc_partnum;
    int			 sib_partnum;
    uint32_t		 anc_start, anc_end;
    uint32_t		 sib_start, sib_end;
    uint32_t		 overlap_start, overlap_end;
    uint32_t		 anc_session;
    uint32_t		 sib_session;

    SLIST_FOREACH(anc_vol_set, &udf_volumeset_list, next_volumeset) {
        anc_pri_vol = STAILQ_FIRST(&anc_vol_set->primaries)->pri_vol;
        sib_vol_set = SLIST_NEXT(anc_vol_set, next_volumeset);
        while (sib_vol_set) {
            sib_pri_vol = STAILQ_FIRST(&sib_vol_set->primaries)->pri_vol;
            DEBUG(
                printf("checking volset %s with volset %s\n", anc_pri_vol->volset_id+1, sib_pri_vol->volset_id+1)
            );
            /* compare these two volume sets but only process partitions on _this_ disc */
            SLIST_FOREACH(anc_part, &anc_vol_set->parts, next_partition) {
                if (anc_part->udf_session->disc != disc)  continue;

                anc_session = anc_part->udf_session->session_num;
                anc_start = 0;
#if 0
                if (disc->session_quirks[anc_session] & CD_SESS_QUIRK_SESSION_LOCAL)
                    anc_start += disc->session_start[anc_session];
#endif
                anc_start += udf_rw32(anc_part->partition->start_loc);
                anc_end    = anc_start + udf_rw32(anc_part->partition->part_len);

                SLIST_FOREACH(sib_part, &sib_vol_set->parts, next_partition) {
                    if (sib_part->udf_session->disc != disc)  continue;

                    sib_session = sib_part->udf_session->session_num;
#if 0
                    /* can `session local' volumes even be considered part/successor ? */
                    if (disc->session_quirks[sib_session] & CD_SESS_QUIRK_SESSION_LOCAL) continue;
#endif
                    sib_start = 0;
#if 0
                    if (disc->session_quirks[sib_session] & CD_SESS_QUIRK_SESSION_LOCAL)
                        sib_start += disc->session_start[sib_session];
#endif
                    sib_start += udf_rw32(sib_part->partition->start_loc);
                    sib_end    = sib_start + udf_rw32(sib_part->partition->part_len);
DEBUG(
anc_partnum = udf_rw16(anc_part->partition->part_num);
sib_partnum = udf_rw16(sib_part->partition->part_num);
printf("\t\tchecking partition %d with partition %d ([%d-%d] x [%d-%d])\n", anc_partnum, sib_partnum, anc_start, anc_end, sib_start, sib_end)
);
                    overlap_start = MAX(sib_start, anc_start);
                    overlap_end   = MIN(sib_end,   sib_end);
                    if (overlap_start < overlap_end)  {
DEBUG(
printf("\t\t\tOVERLAP!\n")
);
                        if (sib_session < anc_session) {
                            /* the sibbling is older */
            UDF_VERBOSE_TABLES(
                printf("\tVolume set ");
                udf_dump_id(NULL, 128, anc_pri_vol->vol_id, &anc_pri_vol->desc_charset);
                printf(" is a newer version of volume set ");
                udf_dump_id(NULL, 128, sib_pri_vol->vol_id, &sib_pri_vol->desc_charset);
                printf("\n");
            );
                            sib_vol_set->obsolete = 1;
                            break;
                        }
                    } /* overlap */
                    if (sib_vol_set->obsolete) break;
                } /* sibling partition */
                if (sib_vol_set->obsolete) break;
            } /* ancestor partition */
            sib_vol_set = SLIST_NEXT(sib_vol_set, next_volumeset);
        } /* sibling volume set */
    } /* ancestor volume set */
}


int udf_add_session_to_discinfo(struct udf_discinfo *disc, int session, struct anchor_vdp *avdp, int error) {
    struct udf_session	*udf_session;

    udf_session = calloc(1, sizeof(struct udf_session));
    assert(udf_session);

    if (!error) {
        memcpy(&udf_session->anchor, avdp, sizeof(struct anchor_vdp));
    }

    udf_session->disc              = disc;
    udf_session->session_num       = session;
    udf_session->session_offset    = 0;
    udf_session->session_length    = disc->session_end[session] - disc->session_start[session];
    disc->session_quirks[session]  = 0;

    /* writable session administration */
    udf_session->writable = 0;		/* default off */
    error = udf_init_session_caches(udf_session);

    if (!error) {
        /* detect quirks */
        /* XXX session local disabled due to wrong heuristic XXX */
#if 0
        if (disc->session_start[session] > 0) {
            if ((udf_session->anchor.main_vds_ex.loc < disc->session_start[session])) {
                disc->session_quirks[session] |= CD_SESS_QUIRK_SESSION_LOCAL;
                udf_session->session_offset = disc->session_start[session];
            }
        }
#endif
    }

    /* add to tail of session list */
    STAILQ_INSERT_TAIL(&disc->sessions, udf_session, next_session);

    disc->num_udf_sessions++;

    /* record status of this volume */
    disc->session_is_UDF[session] = error ? 0 : 1;

    return error;
}


int udf_get_anchors(struct udf_discinfo *disc) {
    uint8_t			*sector;
    union dscrptr		*dscr;
    uint32_t		 session_start, session_end;
    int			 session, error;

    /* Get all anchors */
    STAILQ_INIT(&disc->sessions);

    sector = NULL;
    for (session = 0; session < disc->num_sessions; session++) {
        /* check for anchors ; no volume recognition data ? */
        session_start = disc->session_start[session];
        session_end   = disc->session_end  [session]-1;

        sector = calloc(1, disc->sector_size);
        if (!sector) return ENOMEM;

        dscr = (union dscrptr *) sector;
        error = udf_read_physical_sectors(disc, session_end, 1, "Anchor", sector);
        if (!error) error = udf_check_tag_presence(dscr, TAGID_ANCHOR);
        if (!error) UDF_VERBOSE_TABLES(printf("Accepting anchor at session end (%d)\n", session_end));
        if (error) {
            error = udf_read_physical_sectors(disc, session_end - 256, 1, "Anchor", sector);
            if (!error) error = udf_check_tag_presence(dscr, TAGID_ANCHOR);
            if (!error) UDF_VERBOSE_TABLES(printf("Accepting anchor at session end - 256 (%d)\n", session_end - 256));
            if (error) {
                error = udf_read_physical_sectors(disc, session_start + 256, 1, "Anchor", sector);
                if (!error) error = udf_check_tag_presence(dscr, TAGID_ANCHOR);
                if (!error) UDF_VERBOSE_TABLES(printf("Accepting anchor at session sector 256 (%d)\n", session_start + 256));
                if (error) {
                    /* unclosed CD recordable case due to track reservation for iso9660 filesystems */
                    error = udf_read_physical_sectors(disc, session_start + 512, 1, "Anchor", sector);
                    if (!error) error = udf_check_tag_presence(dscr, TAGID_ANCHOR);
                    if (!error) UDF_VERBOSE_TABLES(printf("Accepting anchor at session sector 512 (%d)\n", session_start + 512));
                }
            }
        }

        if (!error) {
            udf_add_session_to_discinfo(disc, session, (struct anchor_vdp *) sector, error);
        } else {
            free(sector);
        }
    }

    return 0;
}


int udf_get_volumeset_space(struct udf_discinfo *disc) {
    struct udf_session *udf_session;
    int one_good_found;
    int error;

    /* Rip all volume spaces */
    one_good_found = 0;
    UDF_VERBOSE(printf("\tretrieving volume space\n"));
    STAILQ_FOREACH(udf_session, &disc->sessions, next_session) {
        UDF_VERBOSE_MAX(printf("Session %d volumes : \n", udf_session->session_num));

        error = udf_retrieve_volume_space(disc, udf_session, &udf_session->anchor.main_vds_ex);
        if (error) {
            printf("\nError retrieving session %d's volume space; prosessing reserve\n", udf_session->session_num);
            error = udf_retrieve_volume_space(disc, udf_session, &udf_session->anchor.reserve_vds_ex);
        }
        if (!error)
            one_good_found = 1;
    }

    return one_good_found ? 0 : ENOENT;
}


int udf_get_logical_volumes_supporting_tables(struct udf_discinfo *disc, int mnt_flags) {
    struct udf_volumeset	*udf_volumeset;
    struct udf_pri_vol	*udf_pri_vol;
    struct udf_log_vol	*udf_log_vol;
    int logvolint_error;
    int one_good_found;
    int error;

    one_good_found = 0;
    SLIST_FOREACH(udf_volumeset, &udf_volumeset_list, next_volumeset) {
        if (!udf_volumeset->obsolete) {
            STAILQ_FOREACH(udf_pri_vol, &udf_volumeset->primaries, next_primary) {
                if (udf_pri_vol->udf_session->disc == disc) {
                    SLIST_FOREACH(udf_log_vol, &udf_pri_vol->log_vols, next_logvol) {
                        /* retrieving logical volume integrity sequence */
                        UDF_VERBOSE(udf_dump_volume_name("\t\tLogical volume ", udf_log_vol));
                        UDF_VERBOSE(printf("\t\t\tintegrity\n"));
                        logvolint_error = udf_proc_logvol_integrity_sequence(udf_log_vol);

                        /* load in supporting tables */
                        UDF_VERBOSE(printf("\t\t\tsupporting tables\n"));
                        error = udf_retrieve_supporting_tables(udf_log_vol);

                        /* if the state is still marked `open', its dirty and we mount read-only for safety */
                        if (logvolint_error) {
                            printf("\t\t\t*** marked read-only due to logvol integrity error ***\n");
                            mnt_flags |= UDF_MNT_RDONLY;
                        }
                        if (udf_log_vol->logvol_state == UDF_INTEGRITY_OPEN) {
                            printf("\t\t\t*** marked read-only due to open logical volume    ***\n");
                            mnt_flags |= UDF_MNT_RDONLY;
                        }

                        /* get fileset descriptors */
                        UDF_VERBOSE(printf("\t\t\tfileset(s)\n"));
                        if (!error) error = udf_retrieve_fileset_descriptor(udf_log_vol);

                        /* check if the logical volume is writable */
                        UDF_VERBOSE(printf("\t\t\tchecking writable filesets\n"));
                        if (!error) error = udf_check_writable_filesets(udf_log_vol, mnt_flags);

                        /* load in free/used space tables for writable volsets */
                        UDF_VERBOSE(printf("\t\t\tused/freed space tables\n"));
                        if (!error) error = udf_retrieve_space_tables(udf_log_vol);

                        if (error) {
                            udf_log_vol->broken = 1;
                        } else {
                            one_good_found = 1;
                        }
                    } /* logical */
                } /* disc */
            } /* primary */
        } /* if */
    } /* volumeset */

    return one_good_found? 0 : ENOENT;
}


/******************************************************************************************
 *
 * Disc sync
 *
 ******************************************************************************************/


void udf_sync_tables_callback(int reason, struct udf_wrcallback *wrcallback, int error, uint8_t *sectordata) {
    /* struct udf_node *udf_node = (struct udf_node *) wrcallback->structure; */

    wrcallback = wrcallback;	/* not used now */
    sectordata = sectordata;

    if (reason == UDF_WRCALLBACK_REASON_PENDING) {
        /* what to do? */
        return;
    }
    if (reason == UDF_WRCALLBACK_REASON_ANULATE) {
        /* what to do? */
        return;
    }
    assert(reason == UDF_WRCALLBACK_REASON_WRITTEN);
    if (error) {
        printf("UDF error: sync tables write errors in syncnode not fixed!\n");
        return;
    }
}


/* TODO space tables are not coupled on a logical volume but on a partition/disc, so call them on that instead of logvol */
int udf_sync_space_tables(struct udf_log_vol *udf_log_vol) {
    struct udf_partition	 *udf_partition;
    struct udf_part_mapping  *part_mapping;
    struct part_hdr_desc	 *part_hdr_desc;
    struct udf_wrcallback	  wr_callback;
    union  dscrptr		 *dscrptr;
    uint64_t		  length;
    uint32_t		  sector;
    uint32_t		  lb_size, part_len;
    uint16_t		  dscr_ver;
    int			  part_num;
    int			  error;

    lb_size = udf_log_vol->lb_size;

    wr_callback.function = udf_sync_tables_callback;
    SLIST_FOREACH(part_mapping, &udf_log_vol->part_mappings, next_mapping) {
        part_num = part_mapping->udf_virt_part_num;
        UDF_VERBOSE_TABLES(printf("\tFor partition mapping %d->%d\n", part_num, part_mapping->udf_phys_part_num));

        /* retrieve and process unallocated- and freed-space information for all used partitions of the logvol */
        error = udf_logvol_vpart_to_partition(udf_log_vol, part_num, NULL, &udf_partition);
        assert(udf_partition);

        part_hdr_desc = &udf_partition->partition->pd_part_hdr;
        // part_start    = udf_rw32(udf_partition->partition->start_loc);
        part_len      = udf_rw32(udf_partition->partition->part_len);
        dscr_ver      = udf_rw16(udf_partition->partition->tag.descriptor_ver);

        sector   = udf_rw32(part_hdr_desc->unalloc_space_table.lb_num);
        length   = udf_rw32(part_hdr_desc->unalloc_space_table.len);		/* needed? */
        if (length) {
            printf("UDF: Can't write space tables yet\n");
#if 0
            error = udf_read_logvol_descriptor(udf_log_vol, part_num, sector, "Unallocated space table", &dscrptr, NULL);
            UDF_VERBOSE_MAX(printf("\tUnalloced space table\n"));
            UDF_VERBOSE_MAX(udf_dump_descriptor(dscrptr));
            //udf_process_space_table(&udf_partition->unalloc_space, dscrptr);
            free(dscrptr);
#endif
        }

        sector   = udf_rw32(part_hdr_desc->unalloc_space_bitmap.lb_num);
        length   = udf_rw32(part_hdr_desc->unalloc_space_bitmap.len);
        /* printf("unalloc dscr at partition sector %d\n", sector); */
        if (length) {
            /* read it in and modify */
            dscrptr = (union dscrptr *) udf_partition->unalloc_space_bitmap;
            if (!dscrptr) {
                printf("Warning: creating empty unallocated space bitmap for partition's is gone\n");
                error = udf_create_empty_space_bitmap(lb_size, dscr_ver, /* num_lbs */ part_len, (struct space_bitmap_desc **) &dscrptr);
                assert(!error);
                assert(udf_calc_tag_malloc_size(dscrptr, lb_size) <= length);
                udf_partition->unalloc_space_bitmap = &dscrptr->sbd;
            }

            udf_sync_space_bitmap(&udf_partition->unalloc_space_queue, &dscrptr->sbd, lb_size);
            UDF_VERBOSE_MAX(printf("\tWriteout unallocated space bitmap\n"));
            UDF_VERBOSE_MAX(udf_validate_tag_and_crc_sums((union dscrptr *) dscrptr); udf_dump_descriptor(dscrptr));
            udf_write_partition_descriptor(udf_partition, sector, "Unallocated space bitmap", dscrptr, &wr_callback);	/* SESSION descriptor!! */
        }

        sector    = udf_rw32(part_hdr_desc->freed_space_table.lb_num);
        length    = udf_rw32(part_hdr_desc->freed_space_table.len);
        if (length) {
            printf("UDF: Can't write space tables yet\n");
#if 0
            error = udf_read_logvol_descriptor(udf_log_vol, part_num, sector, "Freed space table", &dscrptr, NULL);
            UDF_VERBOSE_MAX(printf("\tFreed space table\n"));
            UDF_VERBOSE_MAX(udf_dump_descriptor(dscrptr));
            //udf_process_space_table(&udf_partition->freed_space, dscrptr);
            free(dscrptr);
#endif
        }

        sector    = udf_rw32(part_hdr_desc->freed_space_bitmap.lb_num);
        length    = udf_rw32(part_hdr_desc->freed_space_bitmap.len);
/* printf("freed dscr at partition sector %d\n", sector); */
        if (length) {
            /* read it in and modify */
            dscrptr = (union dscrptr *) udf_partition->freed_space_bitmap;
            if (!dscrptr) {
                printf("Warning: creating empty freed space bitmap for partition's is gone\n");
                error = udf_create_empty_space_bitmap(lb_size, dscr_ver, part_len, (struct space_bitmap_desc **) &dscrptr);
                assert(!error);
                assert(udf_calc_tag_malloc_size(dscrptr, lb_size) <= length);
                udf_partition->freed_space_bitmap = &dscrptr->sbd;
            }

            udf_sync_space_bitmap(&udf_partition->freed_space_queue, &dscrptr->sbd, lb_size);
            UDF_VERBOSE_MAX(printf("\tWriteout freed space bitmap\n"));
            UDF_VERBOSE_MAX(udf_validate_tag_and_crc_sums((union dscrptr *) dscrptr); udf_dump_descriptor(dscrptr));
            udf_write_partition_descriptor(udf_partition, sector, "Freed space bitmap", dscrptr, &wr_callback);	/* SESSION descriptor!! */
        }
    }
    UDF_VERBOSE_TABLES(printf("\n"));

    return 0;
}


int udf_writeout_LVID(struct udf_log_vol *udf_log_vol, int type) {
    union  dscrptr	        *dscr;
    struct logvol_int_desc  *intdesc;
    struct udf_logvol_info  *impl;
    struct udf_session      *session;
    struct udf_partition    *udf_partition;
    struct udf_part_mapping *part_mapping;
    struct desc_tag         *terminator;
    struct udf_wrcallback    wr_callback;
    uint32_t sector, lvid_sector, term_sector;
    uint32_t part_num, *free_space_pos, *size_pos, lb_size;
    uint32_t len, length, lvid_len, num_sectors;
    int error, dscr_ver, tagid;

    /* create a new `fresh' logvol integrity */
    session = udf_log_vol->primary->udf_session;
    lb_size = udf_log_vol->lb_size;
    num_sectors = lb_size / session->disc->sector_size;

    sector  = udf_rw32(udf_log_vol->log_vol->integrity_seq_loc.loc);
    length  = udf_rw32(udf_log_vol->log_vol->integrity_seq_loc.len);

    if (!length)
        return ENOENT;

    intdesc = calloc(1, udf_log_vol->lb_size);
    if (!intdesc)
        return ENOMEM;

    /* search insertion place */
    lvid_sector = 0;
    term_sector = 0;
    while (length) {
        error = udf_read_session_descriptor(udf_log_vol->primary->udf_session, sector, "Logical volume integrity descriptor (LVID)", &dscr, &lvid_len);

        /* getting a terminator tag or zero is an OK condition */
        if (error) {
            tagid = 0;
        } else {
            tagid = udf_rw16(dscr->tag.id);
        }
        if ((tagid == TAGID_TERM) || (tagid == 0)) {
            lvid_sector = sector;
            if (length > lb_size) {
                /* space for a terminator */
                term_sector = sector + num_sectors;
            }
            break;	/* while */
        }
        length -= lb_size;
        sector += num_sectors;

        if (udf_rw32(dscr->lvid.next_extent.len)) {
            sector = udf_rw32(dscr->lvid.next_extent.loc);
            length = udf_rw32(dscr->lvid.next_extent.len);
        }
        /* free consumed descriptor */
        free(dscr);
        dscr = NULL;
    }
    if (dscr) free(dscr);

    if ((!lvid_sector) || (length == 0)) {
        sector  = udf_rw32(udf_log_vol->log_vol->integrity_seq_loc.loc);
        length  = udf_rw32(udf_log_vol->log_vol->integrity_seq_loc.len);
        lvid_sector = sector;
        if (length > lb_size) {
            /* space for a terminator */
            term_sector = lvid_sector + num_sectors;
        }
    }
    assert(lvid_sector);

    /* build up integrity descriptor and write it out */
    dscr_ver = udf_rw16(udf_log_vol->log_vol->tag.descriptor_ver);
    udf_init_desc_tag(&intdesc->tag, TAGID_LOGVOL_INTEGRITY, dscr_ver, udf_log_vol->integrity_serial);

    udf_set_timestamp_now(&intdesc->time);
    intdesc->integrity_type = udf_rw32(type);

    intdesc->lvint_next_unique_id = udf_rw64(udf_log_vol->next_unique_id);

    /* calculate and fill in free space */
    intdesc->num_part = udf_rw32(udf_log_vol->num_part_mappings);
    free_space_pos = &intdesc->tables[0];
    size_pos       = &intdesc->tables[udf_log_vol->num_part_mappings];
    SLIST_FOREACH(part_mapping, &udf_log_vol->part_mappings, next_mapping) {
        part_num = part_mapping->udf_virt_part_num;
        udf_logvol_vpart_to_partition(udf_log_vol, part_num, NULL, &udf_partition);
        assert(udf_partition);

        *size_pos++       = udf_partition->partition->part_len;
        *free_space_pos++ = udf_rw32(udf_partition->free_unalloc_space / udf_log_vol->lb_size);
    }

    /* fill in UDF implementation use parameters */
    impl = (struct udf_logvol_info *) (&intdesc->tables[2*udf_log_vol->num_part_mappings]);
    udf_set_imp_id(&impl->impl_id);
    impl->num_files        = udf_rw32(udf_log_vol->num_files);
    impl->num_directories  = udf_rw32(udf_log_vol->num_directories);
    impl->min_udf_readver  = udf_rw16(udf_log_vol->min_udf_readver);
    impl->min_udf_writever = udf_rw16(udf_log_vol->min_udf_writever);
    impl->max_udf_writever = udf_rw16(udf_log_vol->max_udf_writever);

    intdesc->l_iu = udf_rw32(sizeof(struct udf_logvol_info));		/* ECMA 3/10.10.7, UDF 2.2.6.4. */
    len  = sizeof(struct logvol_int_desc) - sizeof(uint32_t);		/* length of logvol_int_desc without the extra table entry */
    len += sizeof(uint32_t) * 2 * udf_log_vol->num_part_mappings;		/* size and free space */
    len += sizeof(struct udf_logvol_info);					/* extra implementation use area */
    len -= UDF_DESC_TAG_LENGTH;						/* without header */
    intdesc->tag.desc_crc_len = udf_rw16(len);

    udf_write_session_descriptor(session, lvid_sector, "Logvol integrity descriptor (LVID)", (union dscrptr *) intdesc, &wr_callback);
    if (session->disc->rewritable && term_sector) {
        /* only when there is space and its a rewritable media add a terminor */
        error = udf_create_empty_terminator_descriptor(lb_size, dscr_ver, &terminator);
        if (!error) {
            udf_write_session_descriptor(session, term_sector, "Logvol integrity sequence descriptor sequence terminator", (union dscrptr *) terminator, &wr_callback);
            free(terminator);
        }
    }

    free(intdesc);

    return 0;
}


/* mark the logical volume `open'; for non-rewritables (CD-R/DVD+R/DVD-R) this is allmost a no-op */
int udf_open_logvol(struct udf_log_vol *udf_log_vol) {
    int error;

    if (!udf_log_vol->writable) {
        udf_dump_volume_name("\nLogical volume marked read only: ", udf_log_vol);
        return EROFS;
    }

    /* will return many times for each write */
    if (udf_log_vol->logvol_state == UDF_INTEGRITY_OPEN)
        return 0;

    /*
     * Opening and closing logical volumes is derived from the state of
     * the primaries disc.
     */
    udf_dump_volume_name("Opening logical volume", udf_log_vol);
    if (!udf_log_vol->primary->udf_session->disc->sequential) {
        error  = udf_writeout_LVID(udf_log_vol, UDF_INTEGRITY_OPEN);
        assert(!error);
        /* sync caches to make sure all is written out */
        udf_sync_caches(udf_log_vol);
        /* FIXME (callback) XXX ought to wait until we get the ALL-OK signal from the writeout-LVID action XXX */
    } else {
        /* sequential recordable; any write just opens it; the descriptor is allready marked open */
    }

    /* mark it open */
    udf_log_vol->logvol_state = UDF_INTEGRITY_OPEN;

    return 0;
}


/* mark the logical volume in a `closed' state; close the integrity when possible for recordables writeout VAT */
int udf_close_logvol(struct udf_log_vol *udf_log_vol) {
    int error;

    if (udf_log_vol->logvol_state == UDF_INTEGRITY_CLOSED) {
        DEBUG(printf("close logvol: integrity allready closed\n"));
        return 0;
    }

    /*
     * Opening and closing logical volumes is derived from the state of
     * the primaries disc.
     */
    udf_dump_volume_name("Closing logical volume", udf_log_vol);
    if (!udf_log_vol->primary->udf_session->disc->sequential) {
        error  = udf_writeout_LVID(udf_log_vol, UDF_INTEGRITY_CLOSED);
        assert(!error);
    } else {
        /* XXX TODO XXX */
        fprintf(stderr, "write out virtual sectors, compile VAT and write out VAT : not implemented\n");
        return EIO;
    }

    /* sync caches to make sure all is written out */
    udf_sync_caches(udf_log_vol);
    /* FIXME (callback) XXX ought to wait until we get the ALL-OK signal from the writeout-LVID action XXX */

    /* mark it closed again */
    udf_log_vol->logvol_state = UDF_INTEGRITY_CLOSED;
    return 0;
}


int udf_sync_logvol(struct udf_log_vol *udf_log_vol) {
    struct udf_node	*udf_node;
    uint32_t num_dirty, count, prnt;
    int error;

    if (!udf_log_vol->writable)
        return 0;

    if (udf_log_vol->logvol_state == UDF_INTEGRITY_CLOSED) {
        DEBUG(printf("close logvol: its closed so no sync nessisary\n"));
        return 0;
    }

    UDF_VERBOSE(udf_dump_volume_name("\tsyncing ", udf_log_vol));

    /* sync all nodes */
    /* XXX syncing logvol sequential due to insertion sort in add node XXX */
    num_dirty = 0;
    TAILQ_FOREACH(udf_node, &udf_log_vol->dirty_nodes, next_dirty) {
        num_dirty++;
    }

    /*
     * Purge all data out first, this will speed things up later (not
     * strickly nessissary since syncing a node will wait for all the data
     * to be written out first anyway
     */
    count = num_dirty;
    prnt = 0;
    UDF_VERBOSE(printf("\t\tsyncing data\n"));
    TAILQ_FOREACH(udf_node, &udf_log_vol->dirty_nodes, next_dirty) {
        UDF_VERBOSE(printf("\r%8d", count); fflush(stdout));
        udf_sync_udf_node(udf_node, "Sync Logvol");
        count--;
        prnt = 1;
    }
    if (prnt) UDF_VERBOSE(printf("\r                      \r"));

    /*
     * Purge all nodes out... they ought to have no dirty buffers anymore
     * but they will write them out if deemed nessisary
     */
    count = num_dirty;
    prnt = 0;
    UDF_VERBOSE(printf("\t\tsyncing nodes\n"));
    TAILQ_FOREACH(udf_node, &udf_log_vol->dirty_nodes, next_dirty) {
        UDF_VERBOSE(printf("\r%8d", count); fflush(stdout));
        DEBUG(printf("N"); fflush(stdout));
        udf_writeout_udf_node(udf_node, "Sync Logvol");
        count--;
        prnt = 1;
    }
    if (prnt) UDF_VERBOSE(printf("\r                      \r"));

    /* shouldn't be nessisary */
    udf_bufcache->flushall = 1;
    udf_purgethread_kick("Sync Logvol");
    usleep(1);

    if (udf_bufcache->lru_len_dirty_metadata + udf_bufcache->lru_len_dirty_data) {
        printf("Warning: after syncing logvol dirty counts != 0 (%d, %d); please contact author.\n",
                udf_bufcache->lru_len_dirty_metadata, udf_bufcache->lru_len_dirty_data);
    }

    /* sync free and used space tables for writable volsets */
    UDF_VERBOSE(printf("\t\tused/freed space tables\n"));
    error = udf_sync_space_tables(udf_log_vol);

    /* close logical volume */
    udf_close_logvol(udf_log_vol);

    return error;
}


/* convenience routine */
int udf_sync_disc(struct udf_discinfo *disc) {
    struct udf_volumeset	*udf_volumeset;
    struct udf_pri_vol	*udf_pri_vol;
    struct udf_log_vol	*udf_log_vol;

    SLIST_FOREACH(udf_volumeset, &udf_volumeset_list, next_volumeset) {
        if (!udf_volumeset->obsolete) {
            STAILQ_FOREACH(udf_pri_vol, &udf_volumeset->primaries, next_primary) {
                if (udf_pri_vol->udf_session->disc == disc) {
                    SLIST_FOREACH(udf_log_vol, &udf_pri_vol->log_vols, next_logvol) {
                        udf_sync_logvol(udf_log_vol);
                    } /* logical */
                } /* disc */
            } /* primary */
        } /* if */
    } /* volumeset */

    return 0;
}


/******************************************************************************************
 *
 * UDF descriptor buildup and update functions
 *
 ******************************************************************************************/

static void udf_init_desc_tag(struct desc_tag *tag, uint16_t id, uint16_t dscr_ver, uint16_t serial_num) {
    bzero(tag, sizeof(struct desc_tag));
    tag->id			= udf_rw16(id);
    tag->descriptor_ver	= udf_rw16(dscr_ver);
    tag->serial_num		= udf_rw16(serial_num);
    /* the rest gets filled in when we write */
}


static void udf_osta_charset(struct charspec *charspec) {
    bzero(charspec, sizeof(struct charspec));
    charspec->type = 0;
    strcpy((char *) charspec->inf, "OSTA Compressed Unicode");
}


static void udf_encode_osta_id(char *osta_id, uint16_t len, char *text) {
    uint16_t  u16_name[1024];
    uint8_t  *pos;
    uint16_t *pos16;

    bzero(osta_id, len);
    if (!text) return;

    bzero(u16_name, sizeof(uint16_t) * 1023);
    /* convert ascii to 16 bits unicode */
    pos   = (uint8_t *) text;
    pos16 = u16_name;
    while (*pos) {
        *pos16 = *pos;
        pos++; pos16++;
    }
    *pos16 = 0;

    udf_CompressUnicode(len, 8, (unicode_t *) u16_name, (byte *) osta_id);

    /* Ecma 167/7.2.13 states that the length is recorded in the last byte */
    osta_id[len-1] = strlen(text)+1;
}


static void udf_set_app_id(struct regid *regid) {
    bzero(regid, sizeof(struct regid));
    regid->flags	= 0;						/* not dirty and not protected */
    strcpy((char *) regid->id, APP_NAME);
    regid->id_suffix[0] = APP_VERSION_MAIN;
    regid->id_suffix[1] = APP_VERSION_SUB;
}


static void udf_set_imp_id(struct regid *regid) {
    bzero(regid, sizeof(struct regid));
    regid->flags	= 0;						/* not dirty and not protected */
    strcpy((char *) regid->id, IMPL_NAME);
    regid->id_suffix[0] = 4;	/* unix */
    regid->id_suffix[1] = 0;	/* generic */
#if   defined(__ANONYMOUSUDF__)
#elif defined(__NetBSD__)
    regid->id_suffix[1] = 8;	/* NetBSD */
#elif defined(__FreeBSD__)
    regid->id_suffix[1] = 7;	/* FreeBSD */
#elif defined(LINUX)
    regid->id_suffix[1] = 5;	/* Linux */
#endif
}


static void udf_set_entity_id(struct regid *regid, char *name, uint16_t UDF_version) {
    uint16_t *ver;

    bzero(regid, sizeof(struct regid));
    regid->flags    = 0;						/* not dirty and not protected */
    strcpy((char *) regid->id, name);
    ver  = (uint16_t *) regid->id_suffix;
    *ver = udf_rw16(UDF_version);
    regid->id_suffix[2] = 4;	/* unix */
    regid->id_suffix[3] = 0;	/* generic */
#if   defined(__ANONYMOUSUDF__)
#elif defined(__NetBSD__)
    regid->id_suffix[3] = 8;	/* NetBSD */
#elif defined(__FreeBSD__)
    regid->id_suffix[3] = 7;	/* FreeBSD */
#elif defined(LINUX)
    regid->id_suffix[3] = 5;	/* Linux */
#endif
}


void udf_set_contents_id(struct regid *regid, char *content_id) {
    bzero(regid, sizeof(struct regid));
    regid->flags    = 0;
    strcpy((char *) regid->id, content_id);
}


/* XXX creators of empty descriptors could be externalised */

/*
 * result can be further processed using modify functions if demanded and then
 * processed trough udf_proc_pri_vol
 * [ int udf_proc_pri_vol(struct udf_session *udf_session, struct udf_pri_vol **current, struct pri_vol_desc *incomming); ]
 *
 */

int udf_create_empty_primary_volume_descriptor(uint32_t sector_size, uint16_t dscr_ver, uint16_t serial, char *volset_id, char *privol_name, int vds_num, int max_vol_seq, struct pri_vol_desc **dscrptr) {
    struct pri_vol_desc *dscr;

    assert(dscrptr);
    *dscrptr = NULL;

    /* allocate and populate an empty primary volume descriptor */
    dscr = malloc(sector_size);
    if (!dscr) return ENOMEM;
    bzero(dscr, sector_size);

    udf_init_desc_tag(&dscr->tag, TAGID_PRI_VOL, dscr_ver, 1);
    dscr->pvd_num		= udf_rw32(serial);
    udf_encode_osta_id(dscr->vol_id, 32, privol_name);
    dscr->vds_num		= udf_rw16(vds_num);
    dscr->max_vol_seq	= udf_rw16(max_vol_seq);
    if (max_vol_seq > 1) {
        dscr->ichg_lvl		= udf_rw16(3);			/* signal its a single volume intended to be in a set */
        dscr->max_ichg_lvl	= udf_rw16(3);			/* ,, */
        dscr->flags		= udf_rw16(1);			/* signal relevance volumeset id */
    } else {
        dscr->ichg_lvl		= udf_rw16(2);			/* signal its volume intended not to be in a set */
        dscr->max_ichg_lvl	= udf_rw16(2);			/* ,, */
        dscr->flags		= udf_rw16(0);			/* signal relevance volumeset id */
    }

    dscr->charset_list		= udf_rw32(1);			/* only CS0 */
    dscr->max_charset_list		= udf_rw32(1);
    udf_encode_osta_id(dscr->volset_id, 128, volset_id);
    udf_osta_charset(&dscr->desc_charset);
    udf_osta_charset(&dscr->explanatory_charset);
    udf_set_app_id(&dscr->app_id);
    udf_set_imp_id(&dscr->imp_id);
    udf_set_timestamp_now(&dscr->time);

    dscr->tag.desc_crc_len = udf_rw16(sizeof(struct pri_vol_desc) - UDF_DESC_TAG_LENGTH);

    *dscrptr = dscr;
    return 0;
}


int udf_create_empty_partition_descriptor(uint32_t sector_size, uint16_t dscr_ver, uint16_t serial, uint16_t part_num, uint32_t access_type, uint32_t start_loc, uint32_t part_len, uint32_t space_bitmap_size, uint32_t unalloc_space_bitmap, struct part_desc **dscrptr) {
    struct part_desc     *dscr;
    struct part_hdr_desc *part_hdr;

    assert(dscrptr);
    *dscrptr = NULL;

    /* allocate and populate empty partition descriptor */
    dscr = malloc(sector_size);					/* only descriptor, no bitmap! */
    if (!dscr) return ENOMEM;
    bzero(dscr, sector_size);

    udf_init_desc_tag(&dscr->tag, TAGID_PARTITION, dscr_ver, 1);
    dscr->seq_num  = udf_rw32(serial);
    dscr->flags    = udf_rw16(1);					/* bit 0 : space is allocated */
    dscr->part_num = udf_rw16(part_num);

    if (dscr_ver == 2) udf_set_contents_id(&dscr->contents, "+NSR02");
    if (dscr_ver == 3) udf_set_contents_id(&dscr->contents, "+NSR03");
    part_hdr = &dscr->pd_part_hdr;
    part_hdr->unalloc_space_bitmap.len    = udf_rw32(space_bitmap_size);
    part_hdr->unalloc_space_bitmap.lb_num = udf_rw32(unalloc_space_bitmap);

    dscr->access_type = udf_rw32(access_type);
    dscr->start_loc   = udf_rw32(start_loc);
    dscr->part_len    = udf_rw32(part_len);

    udf_set_imp_id(&dscr->imp_id);					/* why is this ignored? */

    dscr->tag.desc_crc_len = udf_rw16(sizeof(struct part_desc) - UDF_DESC_TAG_LENGTH);

    *dscrptr = dscr;
    return 0;
}


int udf_create_empty_unallocated_space_descriptor(uint32_t sector_size, uint16_t dscr_ver, uint16_t serial, struct unalloc_sp_desc **dscrptr) {
    struct unalloc_sp_desc *dscr;

    assert(dscrptr);
    *dscrptr = NULL;

    /* allocate and populate an empty unallocated space descriptor */
    dscr = malloc(sector_size);
    if (!dscr) return ENOMEM;
    bzero(dscr, sector_size);

    udf_init_desc_tag(&dscr->tag, TAGID_UNALLOC_SPACE, dscr_ver, 1);
    dscr->seq_num		= udf_rw32(serial);
    dscr->tag.desc_crc_len	= udf_rw16(sizeof(struct unalloc_sp_desc) - sizeof(struct extent_ad) - UDF_DESC_TAG_LENGTH);

    *dscrptr = dscr;

    return 0;
}


int udf_create_empty_implementation_use_volume_descriptor(uint32_t sector_size, uint16_t dscr_ver, uint16_t serial, char *logvol_name, struct impvol_desc **dscrptr) {
    struct impvol_desc *dscr;
    struct udf_lv_info *lv_info;

    assert(dscrptr);
    *dscrptr = NULL;

    /* allocate and populate an empty implementation use volume descriptor */
    dscr = malloc(sector_size);
    if (!dscr) return ENOMEM;
    bzero(dscr, sector_size);

    udf_init_desc_tag(&dscr->tag, TAGID_IMP_VOL, dscr_ver, 1);
    dscr->seq_num		= udf_rw32(serial);
    udf_set_entity_id(&dscr->impl_id, "*UDF LV Info", 0x102);	/* just pick one; it'll be modifed later */

    lv_info = &dscr->_impl_use.lv_info;
    udf_osta_charset(&lv_info->lvi_charset);
    udf_encode_osta_id(lv_info->logvol_id, 128, logvol_name);
    udf_encode_osta_id(lv_info->lvinfo1, 36, NULL);
    udf_encode_osta_id(lv_info->lvinfo2, 36, NULL);
    udf_encode_osta_id(lv_info->lvinfo3, 36, NULL);
    udf_set_imp_id(&lv_info->impl_id);

    dscr->tag.desc_crc_len	= udf_rw16(sizeof(struct impvol_desc) - UDF_DESC_TAG_LENGTH);

    *dscrptr = dscr;

    return 0;
}


int udf_create_empty_logical_volume_descriptor(uint32_t sector_size, uint16_t dscr_ver, uint16_t serial, char *logvol_name, uint32_t lb_size, uint32_t integrity_start, uint32_t integrity_length, struct logvol_desc **dscrptr) {
    struct logvol_desc *dscr;

    assert(dscrptr);
    *dscrptr = NULL;

    /* allocate and populate an empty logical volume descriptor */
    dscr = malloc(sector_size);
    if (!dscr) return ENOMEM;
    bzero(dscr, sector_size);

    udf_init_desc_tag(&dscr->tag, TAGID_LOGVOL, dscr_ver, 1);
    dscr->seq_num		= udf_rw32(serial);
    udf_osta_charset(&dscr->desc_charset);
    udf_encode_osta_id(dscr->logvol_id, 128, logvol_name);
    dscr->lb_size		= udf_rw32(lb_size);
    udf_set_contents_id(&dscr->domain_id, "*OSTA UDF Compliant");

    /* no fsd yet nor partition mapping */
    udf_set_imp_id(&dscr->imp_id);
    dscr->integrity_seq_loc.loc = udf_rw32(integrity_start);
    dscr->integrity_seq_loc.len = udf_rw32(integrity_length * lb_size);

    dscr->tag.desc_crc_len = udf_rw16(sizeof(struct logvol_desc) - 1 - UDF_DESC_TAG_LENGTH);

    *dscrptr = dscr;
    return 0;
}


int udf_create_empty_space_bitmap(uint32_t sector_size, uint16_t dscr_ver, uint32_t num_lbs, struct space_bitmap_desc **dscrptr) {
    struct space_bitmap_desc *dscr;
    uint64_t bits;
    uint32_t bytes, space_bitmap_size;

    assert(dscrptr);
    *dscrptr = NULL;

    /* reserve space for unallocated space bitmap */
    bits  = num_lbs;
    bytes = (bits + 7)/8;
    space_bitmap_size = (bytes + sizeof(struct space_bitmap_desc)-1);

    /* round space bitmap size to sector size */
    space_bitmap_size = ((space_bitmap_size + sector_size - 1) / sector_size) * sector_size;

    /* allocate and populate an empty space bitmap descriptor */
    dscr = malloc(space_bitmap_size);
    if (!dscr) return ENOMEM;
    bzero(dscr, space_bitmap_size);

    udf_init_desc_tag(&dscr->tag, TAGID_SPACE_BITMAP, dscr_ver, 1);
    /* crc length 8 is recommended, UDF 2.3.1.2, 2.3.8.1, errata DCN-5108 for UDF 2.50 and lower. */
    dscr->tag.desc_crc_len = udf_rw16(8);

    dscr->num_bits  = udf_rw32(bits);
    dscr->num_bytes = udf_rw32(bytes);

    *dscrptr = dscr;
    return 0;
}


/* FIXME: no rootdir setting yet */
/* FIXME: fileset desc. is disc sector size or lb_size ? */
int udf_create_empty_fileset_desc(uint32_t sector_size, uint16_t dscr_ver, uint32_t fileset_num, char *logvol_name, char *fileset_name, struct fileset_desc **dscrptr) {
    struct fileset_desc *dscr;

    assert(dscrptr);
    *dscrptr = NULL;

    /* allocate and populate an empty logical volume descriptor */
    dscr = malloc(sector_size);
    if (!dscr) return ENOMEM;
    bzero(dscr, sector_size);

    udf_init_desc_tag(&dscr->tag, TAGID_FSD, dscr_ver, 1);
    udf_set_timestamp_now(&dscr->time);
    dscr->ichg_lvl         = udf_rw16(3);	/* fixed? */
    dscr->max_ichg_lvl     = udf_rw16(3);	/* fixed? */
    dscr->charset_list     = udf_rw32(1);	/* only CS0 */
    dscr->max_charset_list = udf_rw32(1);	/* only CS0 */
    dscr->fileset_num      = udf_rw32(fileset_num);	/* key for fileset */
    dscr->fileset_desc_num = udf_rw32(0);		/* fileset descriptor number as in copy # */

    udf_osta_charset(&dscr->logvol_id_charset);
    udf_encode_osta_id(dscr->logvol_id, 128, logvol_name);

    udf_osta_charset(&dscr->fileset_charset);
    udf_encode_osta_id(dscr->fileset_id, 32, fileset_name);

    udf_encode_osta_id(dscr->copyright_file_id, 32, NULL);
    udf_encode_osta_id(dscr->abstract_file_id,  32, NULL);

    udf_set_contents_id(&dscr->domain_id, "*OSTA UDF Compliant");

    dscr->tag.desc_crc_len = udf_rw16(sizeof(struct fileset_desc) - UDF_DESC_TAG_LENGTH);

    *dscrptr = dscr;
    return 0;
}


int udf_create_empty_anchor_volume_descriptor(uint32_t sector_size, uint16_t dscr_ver, uint32_t main_vds_loc, uint32_t reserve_vds_loc, uint32_t length, struct anchor_vdp **vdp) {
    assert(vdp);
    assert(main_vds_loc - reserve_vds_loc >= length);

    *vdp = malloc(sector_size);
    if (!*vdp) return ENOMEM;
    bzero(*vdp, sector_size);

    udf_init_desc_tag(&(*vdp)->tag, TAGID_ANCHOR, dscr_ver, 1);
    (*vdp)->main_vds_ex.loc    = udf_rw32(main_vds_loc);
    (*vdp)->main_vds_ex.len    = udf_rw32(length * sector_size);
    (*vdp)->reserve_vds_ex.loc = udf_rw32(reserve_vds_loc);
    (*vdp)->reserve_vds_ex.len = udf_rw32(length * sector_size);

    (*vdp)->tag.desc_crc_len = udf_rw16(512-UDF_DESC_TAG_LENGTH);		/* fixed size in Ecma */
    return 0;
}


int udf_create_empty_terminator_descriptor(uint32_t sector_size, uint16_t dscr_ver, struct desc_tag **tag) {
    assert(tag);

    *tag = malloc(sector_size);
    if (!*tag) return ENOMEM;
    bzero(*tag, sector_size);

    udf_init_desc_tag(*tag, TAGID_TERM, dscr_ver, 1);

    (*tag)->desc_crc_len = udf_rw16(512-UDF_DESC_TAG_LENGTH);		/* fixed size in Ecma */
    return 0;
}


/******************************************************************************************
 *
 * Basic `open' and `close' disc functions
 *
 ******************************************************************************************/


static void udf_process_session_range(struct udf_discinfo *disc, int *enabled, int low, int high) {
    int session;

    if (!disc) return;

    high = MIN(high, disc->num_sessions-1);
    session = low;

    for (session = low; session <= high; session++) {
        enabled[session] = 1;
    }
}


/* range is specified in -3,5,7 or 5-6,8- etc */
static int udf_process_session_range_string(struct udf_discinfo *disc, char *range) {
    struct udf_session *udf_session, *next_udf_session;
    char *pos, *nop;
    int low, high, len, session;
    int enabled[MAX_SESSIONS];

    if (!range) return 0;
    DEBUG(printf("UDF range debugging string '%s'\n", range));

    if (disc) {
        /* disable all */
        for (session = 0; session < disc->num_sessions; session++) {
            enabled[session] = 0;
        }
    }

    /* parse string */
    nop = strdup(range);
    pos = range;
    if (sscanf(pos, "-%u%n%s", &high, &len, nop) >= 1) {
        DEBUG(printf("UDF range match till %d\n", high));
        udf_process_session_range(disc, enabled, 0, high);
        pos += len;
    }
    if (*pos && *pos == ',') pos++;
    while (*pos) {
        if (sscanf(pos, "%u%n%s", &low, &len, nop) >= 1) {
            pos += len;
            if (*pos == '-') {
                pos++;
                if (!*pos) {
                    DEBUG(printf("UDF range match from %d\n", low));
                    udf_process_session_range(disc, enabled, low, INT_MAX);
                    free(nop);
                    return 0;
                }
                if (sscanf(pos, "%u%n%s", &high, &len, nop) >= 1) {
                    pos += len;
                    DEBUG(printf("UDF range match from %d to %d\n", low, high));
                    udf_process_session_range(disc, enabled, low, high);
                }
            } else {
                if (!*pos || (*pos == ',')) {
                    DEBUG(printf("UDF range match %d\n", low));
                    udf_process_session_range(disc, enabled, low, low);
                }
            }
            if (*pos && (*pos != ',')) {
                fprintf(stderr, "UDF range matching : ',' expected at %s\n", pos);
                free(nop);
                return ENOENT;
            }
            pos++;
        } else {
            fprintf(stderr, "UDF range matching : number expected at %s\n", pos);
            free(nop);
            return ENOENT;
        }
    }
    free(nop);

    DEBUG(printf("UDF range matching : all ok till the end\n"));
    if (!disc) return 0;

    udf_session = STAILQ_FIRST(&disc->sessions);
    while (udf_session) {
        next_udf_session = STAILQ_NEXT(udf_session, next_session);
        session = udf_session->session_num;
        if (!enabled[session]) {
            /* remove this session */
            fprintf(stderr, "UDF: disabling UDF session %d on request\n", session);
            STAILQ_REMOVE(&disc->sessions, udf_session, udf_session, next_session);
            free(udf_session);

            disc->session_is_UDF[session] = 0;
        }
        udf_session = next_udf_session;
    }

    return 0;
}


int udf_check_session_range(char *range) {
    return udf_process_session_range_string(NULL, range);
}


void
udf_init(void)
{
    udf_unix_init();
    udf_start_unix_thread();
    dirhash_init();

    SLIST_INIT(&udf_discs_list);
}


int udf_mount_disc(char *devname, char *range, uint32_t sector_size, int mnt_flags, struct udf_discinfo **disc) {
    int discop_flags, error;

    discop_flags = mnt_flags & UDF_MNT_BSWAP ? UDF_DISCOP_BSWAP : 0;
    error = udf_open_disc(devname, discop_flags, disc);
    if ((!error) && sector_size)
        error = udf_discinfo_alter_perception(*disc, sector_size, 0);
    if (error)
        return error;

    error = udf_get_anchors(*disc);
    UDF_VERBOSE(udf_dump_disc_anchors(*disc));

    if (range) {
        UDF_VERBOSE(printf("Selecting UDF sessions '%s' as specified\n", range));
        udf_process_session_range_string(*disc, range);
        UDF_VERBOSE(udf_dump_disc_anchors(*disc));
    }

    /* no UDF partitions so bail out */
    if ((*disc)->num_udf_sessions == 0) return 0;

    UDF_VERBOSE(printf("Start mounting\n"));
    error = udf_get_volumeset_space(*disc);
    if (error) return error;

    UDF_VERBOSE(printf("\teliminating predescessors\n"));
    udf_eliminate_predescessor_volumesets(*disc);

    UDF_VERBOSE_TABLES(udf_dump_alive_sets());

    UDF_VERBOSE(printf("\tretrieving logical volume dependencies %p\n", *disc));
    error = udf_get_logical_volumes_supporting_tables(*disc, mnt_flags);

    UDF_VERBOSE_TABLES(udf_dump_alive_sets());

    /* insert disc in the disc list */
    SLIST_INSERT_HEAD(&udf_discs_list, *disc, next_disc);

    return error;
}


int udf_dismount_disc(struct udf_discinfo *disc) {
    UDF_VERBOSE(printf("Dismounting disc\n"));
    if (!disc->recordable) {
        /* easy way out: it was a read-only system */
        UDF_VERBOSE(printf("\tdismounting readonly disc\n"));
        udf_stop_unix_thread();
        udf_close_disc(disc);
        return 0;
    }

    /* Sync disc before closing it */
    UDF_VERBOSE(printf("\tsyncing disc\n"));
    udf_sync_disc(disc);

    /* wait for the disc to idle */
    UDF_VERBOSE(printf("\twait for syncing disc to idle\n"));
    while (!udf_discinfo_check_disc_ready(disc)) {
        sleep(1);
    }

    /* stop threads and finish writing to it */
    udf_stop_unix_thread();

    UDF_VERBOSE(printf("\tsignal disc its finished with writing\n"));
    udf_discinfo_finish_writing(disc);

    /* wait for the disc to idle again */
    UDF_VERBOSE(printf("\twait for final disc idling\n"));
    while (!udf_discinfo_check_disc_ready(disc)) {
        sleep(1);
    }

    UDF_VERBOSE(printf("\tclose device\n"));
    udf_close_disc(disc);

    return 0;
}


/******************************************************************************************
 *
 * Directory and other conversion UDF logic
 * Move to udf_unix.c / udf_vnops.c one day?
 *
 ******************************************************************************************/

static int udf_translate_icb_filetype_to_dirent_filetype(int udf_filetype) {
    int d_type;

    switch (udf_filetype) {
        case UDF_ICB_FILETYPE_DIRECTORY :
            d_type = DT_DIR;
            break;
        case UDF_ICB_FILETYPE_STREAMDIR :
            d_type = DT_DIR;
            break;
        case UDF_ICB_FILETYPE_FIFO :
            d_type = DT_FIFO;
            break;
        case UDF_ICB_FILETYPE_CHARDEVICE :
            d_type = DT_CHR;
            break;
        case UDF_ICB_FILETYPE_BLOCKDEVICE :
            d_type = DT_BLK;
            break;
        case UDF_ICB_FILETYPE_RANDOMACCESS :
            d_type = DT_REG;
            break;
        case UDF_ICB_FILETYPE_SYMLINK :
            d_type = DT_LNK;
            break;
        case UDF_ICB_FILETYPE_SOCKET :
            d_type = DT_SOCK;
            break;
        default :
            d_type = DT_UNKNOWN;
            break;
    }
    return d_type;
}


/* VOP_GETATTR */
/* allmost NOOP since we remember the stat in the inode */
int udf_getattr(struct udf_node *udf_node, struct stat *stat) {
    *stat = udf_node->stat;

    /* special: updatables */
    stat->st_nlink   = udf_node->link_cnt;
    stat->st_blocks  = (stat->st_size + 512 -1)/512;	/* blocks are hardcoded 512 bytes/sector in stat :-/ */
    return 0;
}


/* VOP_SETATTR */
/* allmost NOOP since we remember the stat in the inode */
/* note VOP_SETATTR can selectively set attrs		*/
int udf_setattr(struct udf_node *udf_node, struct stat *stat) {
    if (!udf_node) return ENOENT;

    if (udf_open_logvol(udf_node->udf_log_vol))
        return EROFS;

    /* FIXME please don't just copy everything ... XXX */
    udf_node->stat = *stat;

    /* not attribute change time */
    udf_set_timespec_now(&udf_node->stat.st_ctimespec);

    udf_node_mark_dirty(udf_node);
    return 0;
}


void udf_resync_fid_stream(uint8_t *buffer, uint32_t *pfid_pos, uint32_t max_fid_pos, int *phas_fids) {
    struct fileid_desc *fid;
    uint32_t fid_pos;
    int has_fids;

    assert(buffer);
    assert(pfid_pos);
    assert(phas_fids);

    has_fids = 0;
    fid_pos  = *pfid_pos;
    while (!has_fids) {
        while (fid_pos <= max_fid_pos) {
            fid = (struct fileid_desc *) (buffer + fid_pos);
            if (udf_rw16(fid->tag.id) == TAGID_FID)
                break;
            /* fid's can only exist 4 bytes aligned */
            fid_pos += 4;
        }
        if (fid_pos > max_fid_pos) {
            /* shouldn't happen ! to prevent chaos, do nothing */
            /* XXX ought to give a warning? XXX */
            has_fids = 0;
            break;
        } else {
            /* check if we found a valid FID */
            fid = (struct fileid_desc *) (buffer + fid_pos);
            has_fids = (udf_check_tag((union dscrptr *) fid) == 0);
            if (has_fids) {
                assert(udf_rw16(fid->tag.id) == TAGID_FID);
                break;
            }
        }
    }
    *pfid_pos  = fid_pos;
    *phas_fids = has_fids;
}


/* read one fid and process it into a dirent and advance to the next */
/* (*fid) has to be allocated a logical block in size, (*dirent) struct dirent length */
int udf_read_fid_stream(struct udf_node *dir_node, uint64_t *offset, struct fileid_desc *fid, struct dirent *dirent) {
    struct uio     dir_uio;
    struct iovec   dir_iovec;
    char          *fid_name;
    uint32_t       entry_length, lb_size;
    int            enough, error;

    assert(fid);
    assert(dirent);
    assert(dir_node);
    assert(offset);
    assert(*offset != 1);

    lb_size = dir_node->udf_log_vol->lb_size;
    entry_length = 0;
    bzero(dirent, sizeof(struct dirent));
    bzero(fid, lb_size);

    if (*offset >= (uint64_t) dir_node->stat.st_size)
        return EINVAL;

    bzero(&dir_uio, sizeof(struct uio));
    dir_uio.uio_rw     = UIO_WRITE;	/* write into this space */
    dir_uio.uio_iovcnt = 1;
    dir_uio.uio_iov    = &dir_iovec;
    dir_iovec.iov_base = fid;
    dir_iovec.iov_len  = lb_size;
    dir_uio.uio_offset = *offset;
    dir_uio.uio_resid  = MIN(dir_node->stat.st_size - (*offset), lb_size);

    error = udf_read_file_part_uio(dir_node, "file id" /* udf_node->dirent.d_name */, UDF_C_FIDS, &dir_uio);
    if (error)
        return error;

    /*
     * Check if we got a whole descriptor.
     * XXX Try to `resync' directory stream when something is very wrong.
     *
     */
    enough = (dir_uio.uio_offset - (*offset) >= UDF_FID_SIZE);
    if (!enough) {
        /* short dir ... */
        return EIO;
    }

    error = udf_check_tag((union dscrptr *) fid);
    if (!error) {
        entry_length = udf_calc_tag_malloc_size((union dscrptr *) fid, lb_size);
        enough = (dir_uio.uio_offset - (*offset) >= entry_length);
    }
    if (!enough) {
        /* short dir ... */
        return EIO;
    }

    if (!error) error = udf_check_tag_payload((union dscrptr *) fid);
    if (error) {
        printf("BROKEN DIRECTORY ENTRY\n");
#if 0
        // udf_dump_desc(&fid->tag);
        // udf_dump_fileid(fid);
#endif
        /* RESYNC? */
        /* TODO: use udf_resync_fid_stream */
        return EIO;
    }

    /* we got a whole and valid descriptor */
    /* create resulting dirent structure */
    fid_name = (char *) fid->data + udf_rw16(fid->l_iu);
    dirent->d_fileno = udf_rw32(fid->icb.impl.im_used.unique_id);	/* only 32 bits salvageable */
#if !defined(__DragonFly__)
    dirent->d_reclen = sizeof(struct dirent);
#endif
    dirent->d_type   = DT_UNKNOWN;
    udf_to_unix_name(dirent->d_name, fid_name, fid->l_fi, &dir_node->udf_log_vol->log_vol->desc_charset);
#ifndef NO_DIRENT_NAMLEN
    dirent->d_namlen = strlen(dirent->d_name);
#endif

    if (fid->file_char & UDF_FILE_CHAR_DIR) dirent->d_type = DT_DIR;
    if (fid->file_char & UDF_FILE_CHAR_PAR) strcpy(dirent->d_name, "..");

    /* advance */
    *offset += entry_length;

    return error;
}


/* VOP_READDIR */
/* read in dirent's until the result_uio can't hold another */
int udf_readdir(struct udf_node *dir_node, struct uio *result_uio, int *eof_res /* int *cookies, int ncookies */) {
    struct fileid_desc *fid;
    struct dirent  dirent;
    uint64_t diroffset, transoffset;
    uint32_t lb_size;
    int      eof;
    int      error;

    assert(eof_res);
    if (!dir_node)
        return EINVAL;
    if (!dir_node->udf_log_vol)
        return EINVAL;

    assert(result_uio->uio_resid >= sizeof(struct dirent));
    lb_size = dir_node->udf_log_vol->lb_size;

    fid = malloc(lb_size);
    if (!fid) return ENOMEM;

    /* check if we ought to insert dummy `.' node */
    if (result_uio->uio_offset == 0) {
        bzero(&dirent, sizeof(struct dirent));
        strcpy(dirent.d_name, ".");
        dirent.d_type   = DT_DIR;
#ifndef NO_DIRENT_NAMLEN
        dirent.d_namlen = 2;
#endif
        uiomove(&dirent, sizeof(struct dirent), result_uio);

        /* mark with magic value (yeah it suxxs) that we have done the dummy */
        result_uio->uio_offset = 1;
    }

    /* start directory reading */
    diroffset   = result_uio->uio_offset;
    transoffset = diroffset;
    while (diroffset < (uint64_t) dir_node->stat.st_size) {
        /* read just the offset when its flagged */
        if (diroffset == 1) {
            diroffset = result_uio->uio_offset = 0;
        }

        /* read in FIDs */
        error = udf_read_fid_stream(dir_node, &diroffset, fid, &dirent);
        if (error) {
            printf("Error while reading directory file: %s\n", strerror(error));
            free(fid);
            return error;
        }

        /* if there is not enough space for the dirent break off read */
        if (result_uio->uio_resid < sizeof(struct dirent))
            break;

        /* remember the last entry we transfered */
        transoffset = diroffset;

        /* skip deleted entries */
        if (fid->file_char & UDF_FILE_CHAR_DEL)
            continue;

        /* skip not visible entries */
        if (fid->file_char & UDF_FILE_CHAR_VIS)
            continue;

        uiomove(&dirent, sizeof(struct dirent), result_uio);
    }

    /* pass on last transfered offset */
    result_uio->uio_offset = transoffset;

    free(fid);

    eof = (result_uio->uio_offset >= (int64_t) dir_node->stat.st_size);
    if (eof_res) *eof_res = 1;
        *eof_res = eof;

    return 0;
}


static int
dirhash_fill(struct udf_node *dir_node)
{
    struct dirhash *dirh;
    struct fileid_desc *fid;
    struct dirent *dirent;
    uint64_t file_size, pre_diroffset, diroffset;
    uint32_t lb_size;
    int error;

    /* make sure we have a dirhash to work on */
    dirh = dir_node->dir_hash;
    assert(dirh);
    assert(dirh->refcnt > 0);

    if (dirh->flags & DIRH_BROKEN)
        return EIO;
    if (dirh->flags & DIRH_COMPLETE)
        return 0;

    /* make sure we have a clean dirhash to add to */
    dirhash_purge_entries(dirh);

    /* get directory filesize */
    file_size = dir_node->stat.st_size;

    /* allocate temporary space for fid */
    lb_size = dir_node->udf_log_vol->lb_size;
    fid = malloc(lb_size);
    assert(fid);

    /* allocate temporary space for dirent */
    dirent = malloc(sizeof(struct dirent));
    assert(dirent);

    error = 0;
    diroffset = 0;
    while (diroffset < file_size) {
        /* transfer a new fid/dirent */
        pre_diroffset = diroffset;
        error = udf_read_fid_stream(dir_node, &diroffset, fid, dirent);
        if (error) {
            /* TODO what to do? continue but not add? */
            dirh->flags |= DIRH_BROKEN;
            dirhash_purge_entries(dirh);
            break;
        }

        if ((fid->file_char & UDF_FILE_CHAR_DEL)) {
            /* register deleted extent for reuse */
            dirhash_enter_freed(dirh, pre_diroffset,
                udf_fidsize(fid));
        } else {
            /* append to the dirhash */
            dirhash_enter(dirh, dirent, pre_diroffset,
                udf_fidsize(fid), 0);

            /* XXX speedup HACK: preread in our nodes to compensate for too lazy backend */
            {
                struct udf_node *res_node;
                error = udf_readin_udf_node(dir_node, &fid->icb, fid, &res_node);
            }
        }
    }
    dirh->flags |= DIRH_COMPLETE;

    free(fid);
    free(dirent);

    return error;
}


/* XXX yes, move namelen to unsigned int */
int udf_lookup_name_in_dir(struct udf_node *dir_node, char *name, int namelen, struct long_ad *icb_loc, struct fileid_desc *fid, int *found) {
    struct dirhash       *dirh;
    struct dirhash_entry *dirh_ep;
    struct dirent *dirent;
    uint64_t diroffset;
    int hit, error;

    /* set default return */
    *found = 0;

    /* get our dirhash and make sure its read in */
    dirhash_get(&dir_node->dir_hash);
    error = dirhash_fill(dir_node);
    if (error) {
        dirhash_put(dir_node->dir_hash);
        return error;
    }
    dirh = dir_node->dir_hash;

    /* allocate temporary space for dirent */
    dirent  = malloc(sizeof(struct dirent));
    if (!dirent)
        return ENOMEM;

    DEBUG(printf("dirhash_lookup looking for `%*.*s`\n",
        namelen, namelen, name));

    /* search our dirhash hits */
    memset(icb_loc, 0, sizeof(*icb_loc));
    dirh_ep = NULL;
    for (;;) {
        hit = dirhash_lookup(dirh, name, namelen, &dirh_ep);
        /* if no hit, abort the search */
        if (!hit)
            break;

        /* check this hit */
        diroffset = dirh_ep->offset;

        /* transfer a new fid/dirent */
        error = udf_read_fid_stream(dir_node, &diroffset, fid, dirent);
        if (error)
            break;

        DEBUG(printf("dirhash_lookup\tchecking `%*.*s`\n",
            (int) DIRENT_NAMLEN(dirent),  (int) DIRENT_NAMLEN(dirent), dirent->d_name));

        /* see if its our entry */
        assert(DIRENT_NAMLEN(dirent) == (unsigned int) namelen);
        if (strncmp(dirent->d_name, name, namelen) == 0) {
            *found = 1;
            *icb_loc = fid->icb;
            break;
        }
    }
    free(dirent);

    dirhash_put(dir_node->dir_hash);

    return error;
}


static int udf_count_direntries(struct udf_node *dir_node, int count_dotdot, uint32_t *dir_entries) {
    struct fileid_desc *fid;
    struct dirent  dirent;
    uint64_t pos;
    uint32_t lb_size;
    int      eof;
    int      error;

    if (!dir_node) return EINVAL;
    lb_size = dir_node->udf_log_vol->lb_size;

    /* count all directory entries with optional the dotdot too */
    /* only defined in directories XXX DT_COMP also possible XXX */
    if ((dir_node->stat.st_mode & S_IFDIR) == 0)
        return ENOTDIR;

    /* get space to read fid in */
    fid = malloc(lb_size);
    if (!fid) return ENOMEM;

    /* start directory reading */
    *dir_entries = 0;
    pos = 0;

    eof = (pos == (uint64_t) dir_node->stat.st_size);
    while (!eof) {
        /* read in FIDs */
        error = udf_read_fid_stream(dir_node, &pos, fid, &dirent);
        if (error) {
            printf("Error while counting directory entries : %s\n", strerror(error));
            free(fid);
            return error;
        }

        /* process this FID/dirent */
        if ((fid->file_char & UDF_FILE_CHAR_DEL) == 0) {
            if (fid->file_char & UDF_FILE_CHAR_PAR) {
                if (count_dotdot) *dir_entries = *dir_entries + 1;
            } else {
                *dir_entries = *dir_entries + 1;
            }
        }
        /* pos is automatically advanced */
        eof = (pos == (uint64_t) dir_node->stat.st_size);
    }
    /* end of directory */
    free(fid);

    return 0;
}


static int udf_writeout_fid_info(struct udf_node *dir_node, struct fileid_desc *fid, uint64_t offset, uint16_t fid_len) {
    struct uio     uio;
    struct iovec   iovec;
    int flags;

    bzero(&uio, sizeof(struct uio));
    uio.uio_rw     = UIO_READ;	/* read from this space */
    uio.uio_iovcnt = 1;
    uio.uio_iov    = &iovec;
    iovec.iov_base = fid;
    iovec.iov_len  = fid_len;
    uio.uio_offset = offset;
    uio.uio_resid  = fid_len;

    flags = UDF_C_FIDS;
    return udf_write_file_part_uio(dir_node, "file id.", flags, &uio);
}


/* search for a space to record the fid in, not checking if it is allready in it ! */
/* ALERT: not to be used to update a fid ... use writeout_fid_info for that        */
/* ONLY used by udf_create_directory_entry */
static int udf_insert_fid_info(struct udf_node *dir_node, struct udf_node *udf_node, struct fileid_desc *i_fid, uint16_t fidsize) {
    struct dirhash       *dirh;
    struct dirhash_entry *dirh_ep;
    struct fileid_desc   *fid;
    struct dirent dirent;
    uint64_t dir_size, fid_pos, chosen_fid_pos, end_fid_pos;
    uint32_t this_fidsize, chosen_size;
    uint32_t lb_size, lb_rest;
    uint32_t  size_diff, chosen_size_diff;
    char    *fid_name;
    int      descr_ver, hit, error;

    udf_node = udf_node;	/* passed only for printing diagnostic info if required */

    if (!dir_node)
        return EINVAL;

    /* only defined in directories XXX DT_COMP also possible XXX */
    if ((dir_node->stat.st_mode & S_IFDIR) == 0)
        return ENOTDIR;

    /* needs to be 4 bytes aligned to be legal! if not, something is seriously wrong so abort */
    assert((fidsize & 3) == 0);

    /* get our dirhash and make sure its read in */
    dirhash_get(&dir_node->dir_hash);
    error = dirhash_fill(dir_node);
    if (error) {
        dirhash_put(dir_node->dir_hash);
        return error;
    }
    dirh = dir_node->dir_hash;

    /* get info */
    lb_size   = dir_node->udf_log_vol->lb_size;
    dir_size  = dir_node->stat.st_size;
    descr_ver = udf_rw16(dir_node->udf_log_vol->log_vol->tag.descriptor_ver);

    /* get space to read fid in */
    fid = malloc(lb_size);
    if (!fid)
        return ENOMEM;

    /* find position that will fit the FID */
    chosen_fid_pos   = dir_size;
    chosen_size      = 0;
    chosen_size_diff = UINT_MAX;

    /* shut up gcc */
#ifndef NO_DIRENT_NAMLEN
    dirent.d_namlen = 0;
#endif

    /* search our dirhash hits */
    error = 0;
    dirh_ep = NULL;
    for (;;) {
        hit = dirhash_lookup_freed(dirh, fidsize, &dirh_ep);
        /* if no hit, abort the search */
        if (!hit)
            break;

        /* check this hit for size */
        this_fidsize = dirh_ep->entry_size;

        /* check this hit */
        fid_pos     = dirh_ep->offset;
        end_fid_pos = fid_pos + this_fidsize;
        size_diff   = this_fidsize - fidsize;
        lb_rest = lb_size - (end_fid_pos % lb_size);

        /* select if not splitting the tag and its smaller */
        if ((size_diff <= chosen_size_diff) &&
            (lb_rest >= sizeof(struct desc_tag)))
        {
            /* UDF 2.3.4.2+3 specifies rules for iu size */
            if ((size_diff == 0) || (size_diff >= 32)) {
                chosen_fid_pos   = fid_pos;
                chosen_size      = this_fidsize;
                chosen_size_diff = size_diff;
            }
        }
    }

    /* extend directory if no other candidate found */
    if (chosen_size == 0) {
        chosen_fid_pos   = dir_size;
        chosen_size      = fidsize;

        /* special case UDF 2.00+ 2.3.4.4, no splitting up fid tag */
        if (dir_node->addr_type == UDF_ICB_INTERN_ALLOC) {
            /* pre-grow directory to see if we're to switch */
            // udf_grow_node(dir_node, dir_size + chosen_size);
            error = udf_truncate_node(dir_node, chosen_fid_pos + chosen_size);
            assert(!error);
        }

        /* make sure the next fid desc_tag won't be splitted */
        if (dir_node->addr_type != UDF_ICB_INTERN_ALLOC) {
            end_fid_pos = chosen_fid_pos + chosen_size;
            lb_rest = lb_size - (end_fid_pos % lb_size);

            /* pad with implementation use regid if needed */
            if (lb_rest < sizeof(struct desc_tag))
                chosen_size += 32;
        }
    }
    chosen_size_diff = chosen_size - fidsize;

    /* populate the FID */
    memset(fid, 0, lb_size);
    udf_init_desc_tag(&fid->tag, TAGID_FID, descr_ver, 1);		/* tag serial number    */
    fid->file_version_num    = i_fid->file_version_num;
    fid->file_char           = i_fid->file_char;
    fid->icb                 = i_fid->icb;
    fid->l_iu                = udf_rw16(0);

    if (chosen_size > fidsize) {
        /* insert implementation-use regid to space it correctly */
        fid->l_iu = udf_rw16(chosen_size_diff);

        /* set implementation use */
        udf_set_imp_id((struct regid *) fid->data);
    }

    /* copy name */
    fid->l_fi = i_fid->l_fi;
    memcpy(fid->data + udf_rw16(fid->l_iu), i_fid->data, fid->l_fi);

    fid->tag.desc_crc_len = chosen_size - UDF_DESC_TAG_LENGTH;

    /* writeout modified piece */
    udf_validate_tag_and_crc_sums((union dscrptr *) fid);
    error = udf_writeout_fid_info(dir_node, fid, chosen_fid_pos, chosen_size);
    assert(!error);

    /* append to the dirhash */
    fid_name = (char *) fid->data + udf_rw16(fid->l_iu);
    dirent.d_fileno = udf_rw32(fid->icb.impl.im_used.unique_id);	/* only 32 bits salvageable */
#if !defined(__DragonFly__)
    dirent.d_reclen = sizeof(struct dirent);
#endif
    dirent.d_type   = DT_UNKNOWN;
    udf_to_unix_name(dirent.d_name, fid_name, fid->l_fi, &dir_node->udf_log_vol->log_vol->desc_charset);
#ifndef NO_DIRENT_NAMLEN
    dirent.d_namlen = strlen(dirent.d_name);
#endif

    if (fid->file_char & UDF_FILE_CHAR_DIR) dirent.d_type = DT_DIR;
    if (fid->file_char & UDF_FILE_CHAR_PAR) strcpy(dirent.d_name, "..");

    dirhash_enter(dirh, &dirent, chosen_fid_pos, udf_fidsize(fid), 1);

    free(fid);
    dirhash_put(dir_node->dir_hash);

    return error;
}


/* create a file in the given directory with the given name and attributes using udf's file_char and udf'd filetype */
/* note
 * 1) that with `refering' node specified its effectively `link()'
 * 2) that with `refering' node specified, `filetype' is discarded as it ought to be the same as the `refering' one
 *
 * XXX this function needs to be splitted into node creation and directory
 * attachment; its now doing both in one go.
 */
int udf_create_directory_entry(struct udf_node *dir_node, char *name, int filetype, int filechar, struct udf_node *refering, struct stat *stat, struct udf_node **new_node) {
    struct udf_allocentry *alloc_entry;
    struct udf_log_vol    *udf_log_vol;
    struct udf_node       *udf_node;
    struct charspec        osta_charspec;
    struct fileid_desc    *fid;
    struct long_ad         icb_loc;
    uint32_t     lb_num, lb_size;
    uint16_t     vpart_num, descr_ver, len;
    int          found, error;

    assert(dir_node);
    assert(name);
    assert(dir_node->udf_log_vol);
    udf_log_vol = dir_node->udf_log_vol;
    lb_size     = udf_log_vol->lb_size;
    descr_ver   = udf_rw16(udf_log_vol->log_vol->tag.descriptor_ver);

    *new_node = NULL;

    /* lookup if it allready exists (sanity mainly) */
    fid = malloc(lb_size);
    assert(fid);

    error = udf_lookup_name_in_dir(dir_node, name, strlen(name), &icb_loc, fid, &found);
    if (!error && found) {
        /* it existed! allready there */
        free(fid);
        return EEXIST;
    }

    if (!refering) {
        /*
         * Get ourselves an empty node and space to record file
         * descriptor in.
         */
        error = udf_init_udf_node(dir_node->mountpoint, udf_log_vol, "New direntry", &udf_node);
        if (error) {
            free(fid);
            return error;
        }

        udf_node->udf_filetype = filetype;
        udf_node->udf_filechar = filechar;
        udf_node->unique_id = udf_increment_unique_id(udf_log_vol);

        /* snif */
        error = udf_allocate_udf_node_on_disc(udf_node);
        if (error) {
            assert(udf_node != dir_node);
            udf_dispose_udf_node(udf_node);
            free(fid);
            return error;
        }

        udf_node->stat = *stat;
        /* note passed creation times; do sanitise them */
#ifndef NO_STAT_BIRTHTIME
        if (udf_insanetimespec(&stat->st_birthtimespec))
            udf_set_timespec_now(&udf_node->stat.st_birthtimespec);
#endif
        if (udf_insanetimespec(&stat->st_ctimespec))
            udf_set_timespec_now(&udf_node->stat.st_ctimespec);
        if (udf_insanetimespec(&stat->st_atimespec))
            udf_set_timespec_now(&udf_node->stat.st_atimespec);
        if (udf_insanetimespec(&stat->st_mtimespec))
            udf_set_timespec_now(&udf_node->stat.st_mtimespec);
    } else {
        /* refering->ignore passed stat info */
        udf_node = refering;
        filetype = udf_node->udf_filetype;

        /* linking changes metadata modification */
        udf_set_timespec_now(&udf_node->stat.st_ctimespec);
    }
    alloc_entry = TAILQ_FIRST(&udf_node->dscr_allocs);
    vpart_num   = alloc_entry->vpart_num;
    lb_num      = alloc_entry->lb_num;

    /* build up new directory entry */
    memset(fid, 0, lb_size);
    udf_osta_charset(&osta_charspec);
    udf_init_desc_tag(&fid->tag, TAGID_FID, descr_ver, 1);		/* tag serial number    */

    if (filechar & UDF_FILE_CHAR_PAR) {
        /* parent or `..' is not allowed to have a name length ... wierd but ok */
        fid->l_fi = 0;
    } else {
        unix_to_udf_name((char *) fid->data, name, &fid->l_fi, &osta_charspec);
    }
    fid->file_version_num = udf_rw16(1);					/* new file/dir; version starts at 1 */
    fid->file_char        = filechar;					/* what is it                        */
    fid->l_iu             = udf_rw32(0);					/* no impl. use                      */
    fid->icb.len          = udf_rw32(lb_size);				/* fill in location                  */
    fid->icb.loc.part_num = udf_rw16(vpart_num);
    fid->icb.loc.lb_num   = udf_rw32(lb_num);

    /* fill in lower 32 bits of unique ID (UDF 3/3.2.2.1) in the impl use part of the FID's long_ad */
    fid->icb.impl.im_used.unique_id = udf_rw32(((udf_node->unique_id << 32) >> 32));

    /* calculate minimum size needed for directory entry */
    len = UDF_FID_SIZE + fid->l_fi;
    len = (len + 3) & ~3;
    fid->tag.desc_crc_len = udf_rw16(len - UDF_DESC_TAG_LENGTH);

    error = udf_insert_fid_info(dir_node, udf_node, fid, len);
    free(fid);	/* Ahum... easily forgotten here */

    if (error) {
        fprintf(stderr, "UDF: fid insertion failed : %s\n", strerror(error));
        if (!refering)
            udf_dispose_udf_node(udf_node);
        return error;
    }

    if (udf_node) {
        /* only insert file in hashlist if its not an explicit reference */
        if (!refering) {
            udf_insert_node_in_hash(udf_node);
        } else {
            refering->link_cnt++;
            udf_node_mark_dirty(refering);
        }
        udf_node_mark_dirty(udf_node);
    }

    *new_node = udf_node;
    return error;
}


/*
 * Rename file from old_name to new_name. `present' is the file to be replaced
 * if found present allready.  Care should be taken that the directory tree is
 * kept intact. To prevent this no path should be possible from the new parent
 * to the node to be renamed if it considers a directory and the new_parent is
 * not equal to the old parent.
 */
/*
 * VOP_RENAME(struct vnode *fdvp, struct vnode *vp,
 *     struct componentname *fcnp, struct componentname *tdvp,
 *     struct vnode *tvp, struct componentname *tcnp
 *     );
 */
int udf_rename(struct udf_node *old_parent, struct udf_node *rename_me, char *old_name, struct udf_node *new_parent, struct udf_node *present, char *new_name) {
    struct udf_node *new_node;
    int error;

    /* sanity */
    if (!old_parent) return ENOENT;
    if (!new_parent) return ENOENT;
    if (!rename_me)  return ENOENT;
    if (!(old_parent->stat.st_mode & S_IFDIR)) return ENOTDIR;
    if (!(new_parent->stat.st_mode & S_IFDIR)) return ENOTDIR;

    if (udf_open_logvol(old_parent->udf_log_vol))
        return EROFS;

    if (udf_open_logvol(new_parent->udf_log_vol))
        return EROFS;

    if ((present && (present->stat.st_mode & S_IFDIR)) || (old_parent != new_parent)) {
        /* cross directory moves */
        fprintf(stderr, "Cross directory renaming is not implemented yet.\n");
        return ENOTSUP;
    }

    /* if it was present, delete old contents; reference counting is done  */
    if (present) {
        /* TODO what about non dir, non file entries? */
        if (present->stat.st_mode & S_IFDIR) {
            error = udf_remove_directory(new_parent, present, new_name);
        } else {
            error = udf_remove_file(new_parent, present, new_name);
        }
        if (error)
            return error;
    }

    /* insert new_name HARD-linked to the `rename_me' node */
    error = udf_create_directory_entry(new_parent, new_name, rename_me->udf_filetype, rename_me->udf_filechar, rename_me, NULL, &new_node);
    if (error) return error;

    /* extra sanity */
    if (!new_node) return ENOENT;

    /* 3) remove old link and mark directories dirty */
    error = udf_remove_directory_entry(old_parent, rename_me, old_name);
    udf_node_mark_dirty(old_parent);
    udf_node_mark_dirty(new_parent);

    return error;
}


/* VOP_CREATE */
int udf_create_file(struct udf_node *dir_node, char *componentname, struct stat *stat, struct udf_node **new_node) {
    struct udf_log_vol *udf_log_vol;
    struct udf_node *udf_node;
    uint32_t lb_size;
    int error;

    if (!dir_node) return EINVAL;

    udf_log_vol = dir_node->udf_log_vol;
    if (!udf_log_vol) return EINVAL;

    lb_size = udf_log_vol->lb_size;
    if (!udf_confirm_freespace(udf_log_vol, UDF_C_NODE, lb_size))
        return ENOSPC;

    if (udf_open_logvol(dir_node->udf_log_vol))
        return EROFS;

    error = udf_create_directory_entry(dir_node, componentname, UDF_ICB_FILETYPE_RANDOMACCESS, 0, NULL, stat, new_node);
    if ((!error) && (*new_node)) {
        udf_node = *new_node;
        /* update sizes */
        udf_node->stat.st_size    = 0;
        udf_node->stat.st_blksize = dir_node->udf_log_vol->lb_size;
        udf_node->stat.st_blocks  = 0;		/* not 1? */

        udf_node->udf_log_vol->num_files++;

        udf_node_mark_dirty(udf_node);
    }
    return error;
}


/* VOP_MKDIR */
int udf_create_directory(struct udf_node *dir_node, char *componentname, struct stat *stat, struct udf_node **new_node) {
    struct udf_log_vol *udf_log_vol;
    struct udf_node *udf_node, *dummy_node;
    uint32_t lb_size;
    int error;

    if (!dir_node) return EINVAL;

    udf_log_vol = dir_node->udf_log_vol;
    if (!udf_log_vol) return EINVAL;

    lb_size = udf_log_vol->lb_size;
    if (!udf_confirm_freespace(udf_log_vol, UDF_C_NODE, 2*lb_size))
        return ENOSPC;

    if (udf_open_logvol(dir_node->udf_log_vol))
        return EROFS;

    stat->st_mode |= S_IFDIR;
    error = udf_create_directory_entry(dir_node, componentname, UDF_ICB_FILETYPE_DIRECTORY, UDF_FILE_CHAR_DIR, NULL, stat, new_node);
    if ((!error) && (*new_node)) {
        udf_node = *new_node;
        /* update sizes */
        udf_node->stat.st_size    = 0;
        udf_node->stat.st_blksize = dir_node->udf_log_vol->lb_size;
        udf_node->stat.st_blocks  = 0;		/* not 1? */

        udf_node->udf_log_vol->num_directories++;

        udf_node_mark_dirty(udf_node);

        /* create `..' directory entry */
        error = udf_create_directory_entry(udf_node, "..", UDF_ICB_FILETYPE_DIRECTORY, UDF_FILE_CHAR_DIR | UDF_FILE_CHAR_PAR, dir_node, stat, &dummy_node);
        if (error) {
            /* use of _prim for dir counting might not go well due to aborted creation */
            error = udf_remove_directory_prim(dir_node, udf_node, componentname);
        }
    }
    return error;
}



/* really deletes all space referenced to this udf node including descriptor spaces and removes it from the administration */
int udf_unlink_node(struct udf_node *udf_node) {
    struct udf_allocentry *alloc_entry;
    uint32_t lbnum, len;
    uint16_t vpart;
    int error;

    /* just in case its called from outside */
    if (udf_open_logvol(udf_node->udf_log_vol))
        return EROFS;

    /* unlinking changes metadata modification */
    udf_set_timespec_now(&udf_node->stat.st_ctimespec);

    udf_node->link_cnt--;
    udf_node_mark_dirty(udf_node);
    if (udf_node->link_cnt > 0)
        return 0;

    /* trunc node */
    udf_truncate_node(udf_node, (uint64_t) 0);	/* get rid of file contents	*/

    /* free descriptors from dscr_allocs queue */
    TAILQ_FOREACH(alloc_entry, &udf_node->dscr_allocs, next_alloc) {
        vpart = alloc_entry->vpart_num;
        lbnum = alloc_entry->lb_num;
        /* flags = alloc_entry->flags; */
        len   = alloc_entry->len;

        error = udf_release_lbs(udf_node->udf_log_vol, vpart, lbnum, len);
        /* what if an error occures? */
        assert(error == 0);
    }

    /* delete from administration */
    udf_dispose_udf_node(udf_node);

    return 0;
}


/* NOTE: Dont use the EXTENT erased part; its for non sequential WORM only */
/*       UDF 2.3.10.1, ECMA 4/48.14.1.1 */
/*       fid->icb.impl.im_used.flags = udf_rw16(UDF_ADIMP_FLAGS_EXTENT_ERASED); */

static int udf_remove_directory_entry(struct udf_node *dir_node, struct udf_node *udf_node, char *name) {
    struct dirhash       *dirh;
    struct dirhash_entry *dirh_ep;
    struct fileid_desc *fid;
    struct dirent *dirent;
    uint64_t diroffset;
    uint32_t lb_size, namelen, fidsize;
    int      hit, found;
    int      error;

    assert(dir_node);
    assert(udf_node);
    assert(udf_node->udf_log_vol);
    assert(name);

    namelen = strlen(name);
    if (strncmp(name, "..", 3) == 0) {
        printf("Asked to remove `..' parent directory identifier; not allowed!\n");
        return ENOENT;
    }

    if (strncmp(name, ".", 2) == 0) {
        printf("Asked to remove `.' current directory identifier; not allowed!\n");
        return ENOENT;
    }

    /* only lookup in directories XXX DT_COMP also possible XXX */
    if ((dir_node->stat.st_mode & S_IFDIR) == 0)
        return ENOTDIR;

    /* get our dirhash and make sure its read in */
    dirhash_get(&dir_node->dir_hash);
    error = dirhash_fill(dir_node);
    if (error) {
        dirhash_put(dir_node->dir_hash);
        return error;
    }
    dirh = dir_node->dir_hash;

    /* allocate temporary space for fid */
    lb_size = udf_node->udf_log_vol->lb_size;
    fid     = malloc(lb_size);
    dirent  = malloc(sizeof(struct dirent));
    if (!fid || !dirent) {
        error = ENOMEM;
        goto error_out;
    }

    /* search our dirhash hits */
    found = 0;
    dirh_ep = NULL;
    for (;;) {
        hit = dirhash_lookup(dirh, name, namelen, &dirh_ep);
        /* if no hit, abort the search */
        if (!hit)
            break;

        /* check this hit */
        diroffset = dirh_ep->offset;

        /* transfer a new fid/dirent */
        error = udf_read_fid_stream(dir_node, &diroffset, fid, dirent);
        if (error)
            break;

        /* see if its our entry */
        assert(DIRENT_NAMLEN(dirent) == namelen);
        if (strncmp(dirent->d_name, name, namelen) == 0) {
            found = 1;
            break;
        }
    }

    if (!found)
        error = ENOENT;
    if (error)
        goto error_out;

    /* get size of fid and compensate for the read_fid_stream advance */
    fidsize    = udf_fidsize(fid);
    diroffset -= fidsize;

    /* mark fid as deleted */
    fid->file_char |= UDF_FILE_CHAR_DEL;
    bzero(&fid->icb, sizeof(struct long_ad));
    udf_validate_tag_and_crc_sums((union dscrptr *) fid);
    udf_writeout_fid_info(dir_node, fid, diroffset, fidsize);

    /* remove from the dirhash */
    dirhash_mark_freed(dirh, dirh_ep, dirent);

    /* delete node and its administration if refcount indicates so */
    udf_unlink_node(udf_node);

error_out:
    if (fid)
        free(fid);
    if (dirent)
        free(dirent);

    dirhash_put(dir_node->dir_hash);

    return error;
}



/* VOP_REMOVE */
int udf_remove_file(struct udf_node *dir_node, struct udf_node *udf_node, char *componentname) {
    int error;

    if (udf_open_logvol(dir_node->udf_log_vol))
        return EROFS;

    if (udf_node->stat.st_mode & S_IFDIR) {
        /* only remove files with this call */
        return EISDIR;
    }

    error = udf_remove_directory_entry(dir_node, udf_node, componentname);
    if (!error) {
        dir_node->udf_log_vol->num_files--;
    }	/* else? */

    return error;
}


static int udf_remove_directory_prim(struct udf_node *dir_node, struct udf_node *udf_node, char *componentname) {
    int error;

    if (udf_open_logvol(dir_node->udf_log_vol))
        return EROFS;

    /* remove the entry */
    error = udf_remove_directory_entry(dir_node, udf_node, componentname);
    if (!error) {
        dir_node->link_cnt--;
        udf_node_mark_dirty(dir_node);
        dir_node->udf_log_vol->num_directories--;
    } else {
        /* whoah! something went wrong, mark the .. as present again */
        printf("UDF warning: filesystem might by in compromised state\n");
        assert(udf_node);
        udf_node->link_cnt++;
    }

    return error;
}


/* VOP_RMDIR */
int udf_remove_directory(struct udf_node *dir_node, struct udf_node *udf_node, char *componentname) {
    uint32_t num_nodes;
    int error;

    if (!(udf_node->stat.st_mode & S_IFDIR)) {
        /* only remove directories with this call */
        return ENOTDIR;
    }

    error = udf_count_direntries(udf_node, 0, &num_nodes);
    if (error) return error;

    if (num_nodes != 0) return ENOTEMPTY;

    error = udf_remove_directory_prim(dir_node, udf_node, componentname);

    return error;
}


/* end of udf.c */

