/* $NetBSD$ */

/*
 * File "udf_verbose.c" is part of the UDFclient toolkit.
 * File $Id: udf_verbose.c,v 1.121 2022/04/22 15:24:24 reinoud Exp $ $Name:  $
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>

#include "udf.h"
#include "udf_bswap.h"


/* globals */
void udf_dump_id(char *prefix, int len, char *id, struct charspec *chsp);
void udf_dump_long_ad(char *prefix, struct long_ad *adr);
void udf_dump_descriptor(union dscrptr *dscrpt);
void udf_dump_vat_table(struct udf_part_mapping *udf_part_mapping);


void udf_dump_unimpl(union dscrptr *dscrpt, char *name) {
	dscrpt = dscrpt;
	printf("\t\t(unimplemented dump of `%s` tag)\n", name);
}


void udf_dump_desc(struct desc_tag *tag) {
	printf("\tTAG: descriptor %d, serial_num %d at sector %d, crc length %d bytes\n",
			udf_rw16(tag->id), udf_rw16(tag->serial_num), udf_rw32(tag->tag_loc), udf_rw16(tag->desc_crc_len));
}


void udf_dump_anchor(struct anchor_vdp *vdp) {
	printf("\t\tAnchor\n");
	printf("\t\t\tMain    volume descriptor set at %d for %d bytes\n",
			udf_rw32(vdp->main_vds_ex.loc),    udf_rw32(vdp->main_vds_ex.len));
	printf("\t\t\tReserve volume descriptor set at %d for %d bytes\n",
			udf_rw32(vdp->reserve_vds_ex.loc), udf_rw32(vdp->reserve_vds_ex.len));
}


void udf_dump_disc_anchors(struct udf_discinfo *disc) {
	int session;

	printf("\nUDF Dump of disc in device %s\n", disc->dev->dev_name);
	printf("UDF sessions : ");
	for (session = 0; session < disc->num_sessions; session++) {
		if (disc->session_is_UDF[session]) {
			printf("Yes");
#if 0
			if (disc->session_quirks[session] & CD_SESS_QUIRK_SESSION_LOCAL) {
				printf("(local)");
			}
#endif
			printf(" ");
		} else {
			printf("No ");
		}
	}
	printf("\n\n");
	UDF_VERBOSE_TABLES(
		struct udf_session *udf_session;
		/* Dump anchors */
		STAILQ_FOREACH(udf_session, &disc->sessions, next_session) {
			printf("UDF session %d (lba %d + %d sectors) anchor dump : \n", udf_session->session_num,
					(uint32_t) disc->session_start[udf_session->session_num], (uint32_t) udf_session->session_length);
			udf_dump_descriptor((union dscrptr *) &udf_session->anchor);
		}
	)
}


char *udf_messy_unicode_conv(char *buf) {
	static char out_buf[1024];
	uint16_t *uni_pos, uni_char;
	char *pos;

	pos = out_buf;
	uni_pos = (uint16_t *) buf;

	while ((uni_char = *uni_pos++)) {
		if (uni_char & 0xff00) uni_char='_';
		*pos++ = uni_char;
	}

	return out_buf;
}


char *udf_get_osname(int os_class, int os_id) {
	static char buffer[40];

	switch (os_class) {
		case 0 : return "undefined OS";
		case 1 : return "DOS/Windows 3.x";
		case 2 : return "OS/2";
		case 3 : return "MacOS";
		case 4 :
			switch (os_id) {
				case 0 : return "UNIX";
				case 1 : return "IBM AIX";
				case 2 : return "SunOS/Solaris";
				case 3 : return "HP/UX";
				case 4 : return "Silicon Graphics Irix";
				case 5 : return "Linux";
				case 6 : return "MKLinux";
				case 7 : return "FreeBSD";
				case 8 : return "NetBSD";
				default :
					 sprintf(buffer, "unknown UNIX (%d)", os_id);
					 return buffer;
			}
		case 5 : return "MS Windows 9x";
		case 6 : return "MS Windows NT";
		case 7 : return "OS/400";
		case 8 : return "BeOS";
		case 9 : return "MS Windows CE";
		default :
			break;
	}
	sprintf(buffer, "unknown OS (%d, %d)", os_class, os_id);
	return buffer;
}


void udf_dump_regid(char *prefix, struct regid *id, int regid_type) {
	char    buffer[UDF_REGID_ID_SIZE+1];
	int     cnt, version;
	uint8_t *pos;

	memcpy(buffer, id->id, UDF_REGID_ID_SIZE);
	buffer[UDF_REGID_ID_SIZE] = 0;

	printf("%s `%s`", prefix, buffer);
	if (regid_type == UDF_REGID_NAME) {
		printf("\n");
		return;
	}
	printf(" (");
	pos = id->id_suffix;
	switch (regid_type) {
		case UDF_REGID_DOMAIN :
			version = udf_rw16(*((uint16_t *) pos));
			printf("UDFv %x; ", version);
			if ((pos[2]) & UDF_DOMAIN_FLAG_HARD_WRITE_PROTECT) printf("HARD ");
			if ((pos[2]) & UDF_DOMAIN_FLAG_SOFT_WRITE_PROTECT) printf("SOFT");
			if (((pos[2]) & 3) == 0) printf("no");
			printf(" write protect ");
			if ((pos[2]) & ~3) printf("; also undefined flags 0x%d", pos[2] & ~3);
			break;
		case UDF_REGID_UDF :
			version = udf_rw16(*((uint16_t *) pos));
			printf("UDFv %x; ", version);
			printf("%s ", udf_get_osname(pos[2], pos[3]));
			break;
		case UDF_REGID_IMPLEMENTATION :
			printf("%s [", udf_get_osname(pos[0], pos[1]));
			for(cnt=2; cnt < 8; cnt++) {
				printf("%02x ", *pos++);
			}
			printf("]");
			break;
		case UDF_REGID_NAME :
			break;
		case UDF_REGID_APPLICATION :
		default :
			printf("[");
			for(cnt=0; cnt < 8; cnt++) {
				printf("%02x ", *pos++);
			}
			printf("]");
			break;
	}
	printf(") (flags=%d)\n", id->flags);
}


void udf_dump_timestamp(char *prefix, struct timestamp *t) {
	printf("%s (%4d %02d %02d at %02d:%02d:%02d.%02d.%02d.%02d)\n", prefix, udf_rw16(t->year), t->month, t->day,
			t->hour, t->minute, t->second, t->centisec, t->hund_usec, t->usec);
}


#if 0
void udf_dump_charspec(char *prefix, struct charspec *chsp) {
	int cnt, ch;

	printf("%s type CS%d (", prefix, chsp->type);
	for (cnt=0; cnt<63; cnt++) {
		ch = chsp->inf[cnt];
		if (ch < 32 || ch > 126) {
			printf(".");
		} else {
			printf("%c", ch);
		}
	}
	printf(")\n");
}
#endif


void udf_dump_sparing_table(struct udf_sparing_table *spt) {
	struct spare_map_entry *sp_entry;
	uint32_t entry, entries;

	printf("\t\tSparing table descriptor\n");
	udf_dump_regid("\t\t\tSparing table Id ", &spt->id, UDF_REGID_UDF);
	printf("\t\t\tRelocation table entries          %d\n", udf_rw16(spt->rt_l));
	printf("\t\t\tSequence number                   %d\n", udf_rw32(spt->seq_num));
	printf("\t\t\tMappings :");

	entries = udf_rw16(spt->rt_l);
	for(entry = 0; entry < entries; entry++) {
		if (entry % 4 == 0) printf("\n\t\t\t\t");
		sp_entry = &spt->entries[entry];
		printf("[%08x -> %08x]   ", udf_rw32(sp_entry->org), udf_rw32(sp_entry->map));
	}
	printf("\n");
}


void udf_dump_pri_vol(struct pri_vol_desc *pvd) {
	struct extent_ad *ext;

	printf("\t\tPrimary volume descriptor\n");
	printf("\t\t\tVolume descriptor sequence number %d\n", udf_rw32(pvd->seq_num));
	printf("\t\t\tPrimary volume descriptor number  %d\n", udf_rw32(pvd->pvd_num));
	udf_dump_id("\t\t\tVolume Id     ", 32, pvd->vol_id, &pvd->desc_charset);
	printf("\t\t\tVolume sequence number            %d\n", udf_rw16(pvd->vds_num));
	printf("\t\t\tMaximum volume sequence number    %d\n", udf_rw16(pvd->max_vol_seq));
	printf("\t\t\tInterchange level                 %d\n", udf_rw16(pvd->ichg_lvl));
	printf("\t\t\tMaximum interchange level         %d\n", udf_rw16(pvd->max_ichg_lvl));
	udf_dump_id("\t\t\tVolume set Id ", 128, pvd->volset_id, &pvd->desc_charset);
	/* udf_dump_charspec("\t\t\tCharspec for this descriptor     ", &pvd->desc_charset); */
	/* udf_dump_charspec("\t\t\tCharspec for the explaination    ", &pvd->explanatory_charset); */
	ext = &pvd->vol_abstract;
	printf("\t\t\tVolume abstract  at %d for %d bytes\n", udf_rw32(ext->loc), udf_rw32(ext->len));
	ext = &pvd->vol_copyright;
	printf("\t\t\tVolume copyright at %d for %d bytes\n", udf_rw32(ext->loc), udf_rw32(ext->len));
	udf_dump_regid("\t\t\tApplication   id", &pvd->app_id, UDF_REGID_APPLICATION);
	udf_dump_timestamp("\t\t\tTimestamp", &pvd->time);
	udf_dump_regid("\t\t\tImplementator id", &pvd->imp_id, UDF_REGID_IMPLEMENTATION);
	printf("\t\t\tPrevious volume descriptor sequence locator at sector %d\n", udf_rw32(pvd->prev_vds_loc));
	printf("\t\t\tFlags %d\n", udf_rw16(pvd->flags));
}


void udf_dump_implementation_volume(struct impvol_desc *ivd) {
	struct charspec *charspec;

	printf("\t\tImplementation use volume descriptor\n");
	printf("\t\t\tVolume descriptor sequence number %d\n", udf_rw32(ivd->seq_num));
	udf_dump_regid("\t\t\tImplementator identifier", &ivd->impl_id, UDF_REGID_UDF);

	/* check on UDF implementation info ... */
	if (strcmp((char *) ivd->impl_id.id, "*UDF LV Info") == 0) {
		charspec = &ivd->_impl_use.lv_info.lvi_charset;
		/* udf_dump_charspec("\t\t\tLV info charspec                 ", charspec); */
	        udf_dump_id("\t\t\tLogical volume identifier         ", 128, ivd->_impl_use.lv_info.logvol_id, charspec);
	        udf_dump_id("\t\t\tLV info 1                         ",  36, ivd->_impl_use.lv_info.lvinfo1,   charspec);
	        udf_dump_id("\t\t\tLV info 2                         ",  36, ivd->_impl_use.lv_info.lvinfo2,   charspec);
	        udf_dump_id("\t\t\tLV info 3                         ",  36, ivd->_impl_use.lv_info.lvinfo3,   charspec);
		udf_dump_regid("\t\t\tImplementation identifier", &ivd->_impl_use.lv_info.impl_id, UDF_REGID_IMPLEMENTATION);
	}
}


char *udf_dump_partition_access_type(int type) {
	switch (type) {
		case UDF_ACCESSTYPE_PSEUDO_OVERWITE : return "Pseudo overwiteable";
		case UDF_ACCESSTYPE_READ_ONLY       : return "Read only";
		case UDF_ACCESSTYPE_WRITE_ONCE      : return "Write once";
		case UDF_ACCESSTYPE_REWRITEABLE     : return "Rewritable (blocked or with erase)";
		case UDF_ACCESSTYPE_OVERWRITABLE    : return "Overwritable";
	}
	return "Unknown partion access type";
}


void udf_dump_part(struct part_desc *pd) {
	struct part_hdr_desc *part_hdr_desc;

	printf("\t\tPartition descriptor\n");
	printf("\t\t\tVolume descriptor sequence number %d\n", udf_rw32(pd->seq_num));
	printf("\t\t\tFlags                             %d\n", udf_rw16(pd->flags));
	printf("\t\t\tPartition number                  %d\n", udf_rw16(pd->part_num));
	udf_dump_regid("\t\t\tContents", &pd->contents, UDF_REGID_APPLICATION);
	printf("\t\t\tAccessType                        %s\n", udf_dump_partition_access_type(udf_rw32(pd->access_type)));
	printf("\t\t\tPartition starts at sector %u for %u sectors\n", udf_rw32(pd->start_loc), udf_rw32(pd->part_len));
	udf_dump_regid("\t\t\tImplementator id", &pd->imp_id, UDF_REGID_IMPLEMENTATION);

	printf("\t\t\tPartition contents use (file) descriptors:\n");
	if (strncmp((char *) pd->contents.id, "+NSR0", 5) == 0) {
		part_hdr_desc = &pd->pd_part_hdr;
		printf("\t\t\t\tUnallocated space table       at logic block %u for %u bytes\n",
				udf_rw32(part_hdr_desc->unalloc_space_table.lb_num),
				udf_rw32(part_hdr_desc->unalloc_space_table.len)
			);
		printf("\t\t\t\tUnallocated space bitmap      at logic block %u for %u bytes\n",
				udf_rw32(part_hdr_desc->unalloc_space_bitmap.lb_num),
				udf_rw32(part_hdr_desc->unalloc_space_bitmap.len)
			);
		printf("\t\t\t\tPartition integrity table     at logic block %u for %u bytes\n",
				udf_rw32(part_hdr_desc->part_integrity_table.lb_num),
				udf_rw32(part_hdr_desc->part_integrity_table.len)
			);
		printf("\t\t\t\tReusable (freed) space table  at logic block %u for %u bytes\n",
				udf_rw32(part_hdr_desc->freed_space_table.lb_num),
				udf_rw32(part_hdr_desc->freed_space_table.len)
			);
		printf("\t\t\t\tReusable (freed) space bitmap at logic block %u for %u bytes\n",
				udf_rw32(part_hdr_desc->freed_space_bitmap.lb_num),
				udf_rw32(part_hdr_desc->freed_space_bitmap.len)
			);
	} else {
		printf("\t\t\t\tWARNING: Unknown or unused contents\n");
	}
}


void udf_dump_log_vol(struct logvol_desc *lvd) {
	union udf_pmap *pmap;
	uint8_t pmap_type, pmap_size;
	uint8_t *pmap_pos;
	uint32_t map, sparing_table;
	uint32_t lb_size, packet_len;

	lb_size = udf_rw32(lvd->lb_size);

	printf("\t\tLogical volume descriptor\n");
	printf("\t\t\tVolume descriptor sequence number %d\n", udf_rw32(lvd->seq_num));
	udf_dump_id("\t\t\tLogical volume id                ",  128, lvd->logvol_id, &lvd->desc_charset);
	printf("\t\t\tLogical block size                %d\n", udf_rw32(lvd->lb_size));
	udf_dump_regid("\t\t\tDomainId", &lvd->domain_id, UDF_REGID_DOMAIN);
	udf_dump_long_ad("\t\t\tFileset descriptor at", &lvd->_lvd_use.fsd_loc);
	printf("\t\t\tMap table length                  %d\n", udf_rw32(lvd->mt_l));
	printf("\t\t\tNumber of part maps               %d\n", udf_rw32(lvd->n_pm));
	udf_dump_regid("\t\t\tImplementation id", &lvd->imp_id, UDF_REGID_IMPLEMENTATION);
	printf("\t\t\tIntegrety sequence at %d for %d bytes\n",
			udf_rw32(lvd->integrity_seq_loc.loc), udf_rw32(lvd->integrity_seq_loc.len));
	printf("\t\t\tPartion maps follow\n");

	pmap_pos = &lvd->maps[0];
	for (map = 0; map < udf_rw32(lvd->n_pm); map++) {
		pmap = (union udf_pmap *) pmap_pos;
		pmap_type = pmap->data[0];
		pmap_size = pmap->data[1];

		printf("\t\t\t\tPartion map type %d length %d \n", pmap_type, pmap_size);
		/* only pmap types 1 and pmap types 2 are to be used */
		printf("\t\t\t\t\tLogical %d maps to ", map);
		switch (pmap_type) {
			case 1 :
				printf("partition %d on volume seq. number %d directly\n",
						udf_rw16(pmap->pm1.part_num), udf_rw16(pmap->pm1.vol_seq_num));
				break;
			case 2 :
				printf("partition %d on volume seq. number %d using\n",
						udf_rw16(pmap->pm2.part_num), udf_rw16(pmap->pm2.vol_seq_num));
				udf_dump_regid("\t\t\t\t\tmapping type", &pmap->pm2.part_id, UDF_REGID_UDF);
				if (strncmp((char *) pmap->pm2.part_id.id, "*UDF Virtual Partition", UDF_REGID_ID_SIZE) == 0) {
					/* nothing to print... */
				}
				if (strncmp((char *) pmap->pm2.part_id.id, "*UDF Sparable Partition", UDF_REGID_ID_SIZE) == 0) {
					packet_len = udf_rw16(pmap->pms.packet_len);
					printf("\t\t\t\t\t\tPacket length                %d sectors (%d bytes)\n",
							packet_len, packet_len * lb_size);
					printf("\t\t\t\t\t\tNumber of sparing tables     %d\n", pmap->pms.n_st);
					printf("\t\t\t\t\t\tSize of each sparing table   %d\n", udf_rw32(pmap->pms.st_size));
					if (pmap->pms.n_st) {
						printf("\t\t\t\t\t\tSparing tables at sectors    ");
						for (sparing_table = 0; sparing_table < pmap->pms.n_st; sparing_table++) {
							printf("%d ", udf_rw32(pmap->pms.st_loc[sparing_table]));
						}
						printf("\n");
					}
				}
				if (strncmp((char *) pmap->pm2.part_id.id, "*UDF Metadata Partition", UDF_REGID_ID_SIZE) == 0) {
					printf("\t\t\t\t\t\tMetadata is %sduplicated on disc\n", pmap->pmm.flags & METADATA_DUPLICATED ? "":"NOT ");
					printf("\t\t\t\t\t\tAllocation unit size                  %d sectors\n", udf_rw32(pmap->pmm.alloc_unit_size));
					printf("\t\t\t\t\t\tAlignment  unit size                  %d sectors\n", udf_rw32(pmap->pmm.alignment_unit_size));
					printf("\t\t\t\t\t\tMetadata file at part. sector         %d\n", udf_rw32(pmap->pmm.meta_file_lbn));
					if (udf_rw32(pmap->pmm.meta_mirror_file_lbn) != (uint32_t) -1) 
						printf("\t\t\t\t\t\tMetadata mirror file at part. sector  %d\n", udf_rw32(pmap->pmm.meta_mirror_file_lbn));
					if (udf_rw32(pmap->pmm.meta_bitmap_file_lbn) != (uint32_t) -1) 
						printf("\t\t\t\t\t\tMetadata bitmap file at part. sector  %d\n", udf_rw32(pmap->pmm.meta_bitmap_file_lbn));
				}
				break;
			default :
				break;
		}
		pmap_pos += pmap_size;
	}
}


void udf_dump_unalloc_space(struct unalloc_sp_desc *usd) {
	struct extent_ad *alloc_desc;
	uint32_t desc_num;

	printf("\t\tUnallocated space descriptor\n");
	printf("\t\t\tVolume descriptor sequence number %d\n", udf_rw32(usd->seq_num));
	printf("\t\t\tNumber of free space slots        %d\n", udf_rw32(usd->alloc_desc_num));
	if (udf_rw32(usd->alloc_desc_num)) {
		printf("\t\t\tFree space at : ");
		for (desc_num = 0; desc_num < udf_rw32(usd->alloc_desc_num); desc_num++) {
			alloc_desc = &usd->alloc_desc[desc_num];
			printf("[%d %d] ", udf_rw32(alloc_desc->loc), udf_rw32(alloc_desc->loc)+udf_rw32(alloc_desc->len));
		}
		printf("\n");
	}
}


void udf_dump_terminating_desc(union dscrptr *desc) {
	desc = desc;

	printf("\t\tTerminating descriptor\n");
}


void udf_dump_logvol_integrity(struct logvol_int_desc *lvid) {
	struct udf_logvol_info *impl;
	uint32_t  part, num_part;
	uint32_t *pos1, *pos2;
	uint32_t  free, size, rest_bytes;
	uint32_t  version;
	const char *inttp;

	printf("\t\tLogical volume integrity descriptor\n");
	udf_dump_timestamp("\t\t\tTimestamp                           ", &lvid->time);

	inttp = "UNKNOWN/INVALID";
	if (udf_rw32(lvid->integrity_type) == UDF_INTEGRITY_CLOSED)
		inttp = "closed";
	if (udf_rw32(lvid->integrity_type) == UDF_INTEGRITY_OPEN)
		inttp = "closed";
	printf("\t\t\tIntegrity type                       %s\n", inttp);
	printf("\t\t\tNext integrity sequence at %d for %d bytes\n",
			udf_rw32(lvid->next_extent.loc), udf_rw32(lvid->next_extent.len));
	printf("\t\t\tNext free unique file ID             %d\n", (uint32_t) udf_rw64(lvid->lvint_next_unique_id));
	printf("\t\t\tLength of implementation use area    %d bytes\n", udf_rw32(lvid->l_iu));

	num_part = udf_rw32(lvid->num_part);
	printf("\t\t\tNumber of partitions                 %d\n", num_part);
	for (part=0; part < num_part; part++) {
		pos1 = &lvid->tables[0] + part;
		pos2 = &lvid->tables[0] + num_part + part;
		free = udf_rw32(*pos1);
		size = udf_rw32(*pos2);
		printf("\t\t\tPartition %d : %u blocks free space out of %u blocks\n", part, free, size);
	}

	/* printout the implementation use field */
	impl = (struct udf_logvol_info *) (lvid->tables + 2*num_part);

	udf_dump_regid("\t\t\tImplemenator Id", &impl->impl_id, UDF_REGID_IMPLEMENTATION );
	printf("\t\t\tNumber of files                      %d\n", udf_rw32(impl->num_files));
	printf("\t\t\tNumber of directories                %d\n", udf_rw32(impl->num_directories));
	version = udf_rw16(impl->min_udf_readver);
	printf("\t\t\tMinimum readversion                  UDFv %x\n", version);
	version = udf_rw16(impl->min_udf_writever);
	printf("\t\t\tMinimum writeversion                 UDFv %x\n", version);
	version = udf_rw16(impl->max_udf_writever);
	printf("\t\t\tMaximum writeversion                 UDFv %x\n", version);
	rest_bytes = udf_rw32(lvid->l_iu)-sizeof(struct udf_logvol_info);
	if (rest_bytes > 0) printf("\t\t\t<%d bytes of undumped extra implementation use area>", rest_bytes);
	printf("\n");
}


void udf_dump_fileset_desc(struct fileset_desc *fsd) {
	printf("\t\tFileset descriptor\n");
	udf_dump_timestamp("\t\t\tTimestamp                         ", &fsd->time);
	printf("\t\t\tInterchange level                  %d\n", udf_rw16(fsd->ichg_lvl));
	printf("\t\t\tMax interchange level              %d\n", udf_rw16(fsd->max_ichg_lvl));
	printf("\t\t\tCharset lists                      %d\n", udf_rw32(fsd->charset_list));
	printf("\t\t\tMax charset lists                  %d\n", udf_rw32(fsd->max_charset_list));
	printf("\t\t\tFileset number                     %d\n", udf_rw32(fsd->fileset_num));
	printf("\t\t\tFileset descriptor number          %d\n", udf_rw32(fsd->fileset_desc_num));
	/* udf_dump_charspec("\t\t\tLogical volume id charspec        ", &fsd->logvol_id_charset); */
	/* udf_dump_charspec("\t\t\tFileset id charspec               ", &fsd->fileset_charset); */
	udf_dump_id("\t\t\tLogical volume id                 ", 128, fsd->logvol_id,  &fsd->logvol_id_charset);
	udf_dump_id("\t\t\tFileset id                        ",  32, fsd->fileset_id, &fsd->fileset_charset);
	udf_dump_id("\t\t\tCopyright file id                 ",  32, fsd->copyright_file_id, &fsd->fileset_charset);
	udf_dump_id("\t\t\tAbstract file id                  ",  32, fsd->abstract_file_id,  &fsd->fileset_charset);
	udf_dump_regid("\t\t\tDomainId", &fsd->domain_id, UDF_REGID_DOMAIN);
	udf_dump_long_ad("\t\t\tRootdir ICB found       ", &fsd->rootdir_icb);
	udf_dump_long_ad("\t\t\tNext extend for fileset ", &fsd->next_ex);
	udf_dump_long_ad("\t\t\tStreamdir ICB found     ", &fsd->streamdir_icb); 
}


void udf_dump_fileid_in_charspec(struct fileid_desc *fid, struct charspec *chsp) {
	char *pos, file_char;

	printf("\tFile id entry\n");
	printf("\t\tFile version number                  %d\n", udf_rw16(fid->file_version_num));
	file_char = fid->file_char;
	printf("\t\tFile characteristics %d :\t", file_char);
	if (file_char & UDF_FILE_CHAR_VIS)  printf("hidden ");
	if (file_char & UDF_FILE_CHAR_DEL)  printf("deleted ");
	if (file_char & UDF_FILE_CHAR_PAR)  printf("parent(..) ");
	if (file_char & UDF_FILE_CHAR_DIR)  printf("directory ");
	if (file_char & UDF_FILE_CHAR_META) printf("METADATA ");
	printf("\n");
	udf_dump_long_ad("\t\tFile ICB", &fid->icb);
	printf("\t\tLength of file identifier area       %d\n", fid->l_fi);
	printf("\t\tOSTA UDF Unique ID                   %d\n", udf_rw32(fid->icb.impl.im_used.unique_id));
	printf("\t\tOSTA UDF fileflags                   %d\n", udf_rw16(fid->icb.impl.im_used.flags));
	printf("\t\tImplementation use length            %d\n", udf_rw16(fid->l_iu));

	if (udf_rw16(fid->l_iu)) {
		/* Ecma 1/7.4 demands a (padded if wanted) implementation identifier */
		if (udf_rw16(fid->l_iu) >= sizeof(struct regid)) {
			udf_dump_regid("\t\t\tModified by", (struct regid *) &fid->data, UDF_REGID_IMPLEMENTATION);
		} else {
			printf("\t\t\tBROKEN fid, expected at least enough space for implementation regid\n");
		}
	}

	pos = (char *) fid->data + udf_rw16(fid->l_iu);
	if (file_char & UDF_FILE_CHAR_PAR) {
		printf("\t\tParent directory ..\n");
	} else {
		udf_dump_id("\t\tFilename", fid->l_fi, pos, chsp);
	}
}


void udf_dump_fileid(struct fileid_desc *fid) {
	struct charspec chsp;

	/* prolly OSTA compressed unicode anyway */
	chsp.type = 0;
	strcpy((char *) chsp.inf, "OSTA Compressed Unicode");

	udf_dump_fileid_in_charspec(fid, &chsp);
}


void udf_dump_icb_tag(struct icb_tag *icb_tag) {
	uint32_t flags, strat_param16;

	flags = udf_rw16(icb_tag->flags);
	strat_param16 = udf_rw16(icb_tag->strat_param16);
	printf("\t\tICB Prior direct entries recorded (excl.)   %d\n", udf_rw32(icb_tag->prev_num_dirs));
	printf("\t\tICB Strategy type                           %d\n", udf_rw16(icb_tag->strat_type));
	printf("\t\tICB Strategy type flags                     %d %d\n", icb_tag->strat_param[0], icb_tag->strat_param[1]);
	printf("\t\tICB Maximum number of entries (non strat 4) %d\n", udf_rw16(icb_tag->max_num_entries));
	printf("\t\tICB     indirect entries/depth              %d\n", strat_param16);
	printf("\t\tICB File type                               %d\n", icb_tag->file_type);
	printf("\t\tICB Parent ICB in logical block %d of mapped partition %d\n",
		udf_rw32(icb_tag->parent_icb.lb_num), udf_rw16(icb_tag->parent_icb.part_num));
	printf("\t\tICB Flags                                   %d\n", udf_rw16(icb_tag->flags));
	printf("\t\t\tFile/directory information using : ");
	switch (flags & UDF_ICB_TAG_FLAGS_ALLOC_MASK) {
		case UDF_ICB_SHORT_ALLOC :
			printf("short allocation descriptor\n");
			break;
		case UDF_ICB_LONG_ALLOC :
			printf("long allocation descriptor\n");
			break;
		case UDF_ICB_EXT_ALLOC :
			printf("extended allocation descriptor (out of specs)\n");
			break;
		case UDF_ICB_INTERN_ALLOC :
			printf("internal in the ICB\n");
			break;
	}
	if (icb_tag->file_type == UDF_ICB_FILETYPE_DIRECTORY)
		if (flags & UDF_ICB_TAG_FLAGS_DIRORDERED)
			printf("\t\t\tOrdered directory\n");
	if (flags & UDF_ICB_TAG_FLAGS_NONRELOC) printf("\t\t\tNot relocatable\n");
	printf("\t\t\tFile flags :");
		if (flags & UDF_ICB_TAG_FLAGS_SETUID) printf("setuid() ");
		if (flags & UDF_ICB_TAG_FLAGS_SETGID) printf("setgid() ");
		if (flags & UDF_ICB_TAG_FLAGS_STICKY) printf("sticky ");
	printf("\n");
	if (flags & UDF_ICB_TAG_FLAGS_CONTIGUES)
		printf("\t\t\tFile is contigues i.e. in one piece effectively \n");
	if (flags & UDF_ICB_TAG_FLAGS_MULTIPLEVERS)
		printf("\t\t\tExpect multiple versions of a file in this directory\n");
}


void udf_dump_indirect_entry(struct indirect_entry *inde) {
	printf("\tIndirect (ICB) entry\n");
	udf_dump_icb_tag(&inde->icbtag);
	udf_dump_long_ad("\t\tPointing at", &inde->indirect_icb);
	printf("\n");
}


void udf_dump_allocation_entries(uint8_t addr_type, uint8_t *pos, uint32_t data_length) {
	union icb	*icb;
	uint32_t	 size, piece_length, piece_flags;
	uint32_t	 entry;

	entry = 0;
	size  = 0;
	while (data_length) {
		if (entry % 1 == 0) printf("\t\t\t");
		printf(" [ ");
		printf("blob at ");
		/* what to do with strat type == 3 ? or is all set up ok then ? */
		icb = (union icb *) pos;
		switch (addr_type) {
			case UDF_ICB_SHORT_ALLOC  :
				piece_length = udf_rw32(icb->s_ad.len) & (((uint32_t) 1<<30)-1);
				piece_flags  = udf_rw32(icb->s_ad.len) >> 30;		/* XXX ecma167 48.14.1.1 XXX */
				printf("sector %8u for %8d bytes", udf_rw32(icb->s_ad.lb_num), piece_length);
				if (piece_flags) printf(" flags %d", piece_flags);
				size = sizeof(struct short_ad);
				if (piece_length == 0) size = data_length;
				break;
			case UDF_ICB_LONG_ALLOC   :
				piece_length = udf_rw32(icb->l_ad.len) & (((uint32_t) 1<<30)-1);
				piece_flags  = udf_rw32(icb->l_ad.len) >> 30;		/* XXX ecma167 48.14.1.1 XXX */
				printf("sector %8d for %8d bytes in logical partion %d", udf_rw32(icb->l_ad.loc.lb_num), piece_length, 
						udf_rw16(icb->l_ad.loc.part_num));
				if (piece_flags) printf(" flags %d", piece_flags);
				size = sizeof(struct long_ad);
				if (piece_length == 0) size = data_length;
				break;
			case UDF_ICB_EXT_ALLOC    :
				printf("extended alloc (help)");
				size = sizeof(struct ext_ad);
				break;
			case UDF_ICB_INTERN_ALLOC :
				printf("internal blob here for %d bytes", data_length);
				size = data_length;
				break;
		}
		printf(" ] \n");
		entry++;
		pos += size;
		data_length -=size;
	}
	printf("\n");

}


/* TODO create a read-in/insert/cleanup etc. for extra attributes */
void udf_dump_extattrseq(uint8_t *start, uint32_t offset, uint32_t impl_offset, uint32_t appl_offset, uint32_t length) {
	struct impl_extattr_entry	*impl_extattr;
	struct appl_extattr_entry	*appl_extattr;
	struct filetimes_extattr_entry	*filetimes_extattr;
	struct device_extattr_entry	*device_extattr;
	struct vatlvext_extattr_entry	*vatlvext_extattr;
	struct extattr_entry	*extattr;
	struct timestamp	*timestamp;
	struct charspec  chsp;
	uint32_t  extattr_len, au_l, iu_l, d_l;
	uint32_t  type, subtype, chksum, attr_space, print_attr_space;
	uint32_t  existence;
	uint8_t  *pos;
	char     *type_txt, what[256];
	int       is_free_ea_space, is_free_app_ea_space, is_vatlvext_space, bit;

	/* if used its OSTA compressed unicode anyway */
	chsp.type = 0;
	strcpy((char *) chsp.inf, "OSTA Compressed Unicode");

	/* if one of the offsets is `-1' (0xffffffff), it indicates that its not present; God i hate magic values */
	if (impl_offset == UDF_IMPL_ATTR_LOC_NOT_PRESENT)
		printf("\t\tNOTE: indicated no implementation related attributes are recorded in this extent\n");
	if (appl_offset == UDF_IMPL_ATTR_LOC_NOT_PRESENT)
		printf("\t\tNOTE: indicated no application related attributes are recorded in this extent\n");

	pos    = start;
	attr_space  = UDF_REGID_UDF;	/* really? */
	while (length > 0) {
		extattr = (struct extattr_entry *) pos;
		extattr_len = udf_rw32(extattr->a_l);
		type        = udf_rw32(extattr->type);
		subtype     = extattr->subtype;

		if (pos    == start)       printf("\t\tStart of extended file related attributes area\n");
		if (offset == impl_offset) printf("\t\tStart of implementation related attributes area\n");
		if (offset == appl_offset) printf("\t\tStart of application related attributes area\n");

		if (pos    == start)       attr_space = UDF_REGID_UDF;
		if (offset == impl_offset) attr_space = UDF_REGID_IMPLEMENTATION;
		if (offset == appl_offset) attr_space = UDF_REGID_APPLICATION;

		if (subtype != 1) printf("\t\t\tWARNING: unknown subtype %d\n", subtype);

		print_attr_space = attr_space;
		switch (type) {
			case 65536 :	/* [4/48.10.8] application use extended attributes */
				appl_extattr = (struct appl_extattr_entry *) pos;
				au_l = udf_rw32(appl_extattr->au_l);
				printf("\t\t\tApplication use extended attribute\n");
				if (attr_space != UDF_REGID_APPLICATION)
					printf("\t\t\t\t*** application use extended attribute found in non application use area ***\n");
				printf("\t\t\t\tLength of application use space     %d\n", au_l);
				udf_dump_regid("\t\t\t\tApplication use Id", &appl_extattr->appl_id, attr_space);
				break;
			case  2048 :	/* [4/48.10.9] implementation use extended attributes */
				impl_extattr = (struct impl_extattr_entry *) pos;
				iu_l = udf_rw32(impl_extattr->iu_l);
				chksum = udf_rw16(impl_extattr->data16);

				printf("\t\t\tImplementation use extended attribute\n");
				if (chksum != udf_ea_cksum(pos))
					printf("\t\t\t\t*** header checksum failed (%d should be %d) ***\n", chksum, udf_ea_cksum(pos));
				if (attr_space != UDF_REGID_IMPLEMENTATION)
					printf("\t\t\t\t*** implementation use extended attribute found in non implementation use area ***\n");

				if (strncmp((char *) impl_extattr->imp_id.id, "*UDF", 4) == 0)
					print_attr_space = UDF_REGID_UDF;
				printf("\t\t\t\tLength of implementation use space     %d\n", iu_l);
				udf_dump_regid("\t\t\t\tImplementation use Id", &impl_extattr->imp_id, print_attr_space);
				is_free_ea_space     = (strcmp((char *) impl_extattr->imp_id.id, "*UDF FreeEASpace")    == 0);
				is_free_app_ea_space = (strcmp((char *) impl_extattr->imp_id.id, "*UDF FreeAppEASpace") == 0);
				is_vatlvext_space    = (strcmp((char *) impl_extattr->imp_id.id, "*UDF VAT LVExtension") == 0);
				if (is_free_ea_space || is_free_app_ea_space) {
					printf("\t\t\t\tFree space for new extended attributes (%d bytes total)\n", extattr_len);
				} else if (is_vatlvext_space) {
					vatlvext_extattr = (struct vatlvext_extattr_entry *) (impl_extattr->data + iu_l);
					printf("\t\t\t\t\tUniqueID check            %"PRIu64"\n", udf_rw64(vatlvext_extattr->unique_id_chk));
					printf("\t\t\t\t\tNumber of files           %d\n", udf_rw32(vatlvext_extattr->num_files));
					printf("\t\t\t\t\tNumber of directories     %d\n", udf_rw32(vatlvext_extattr->num_directories));
					udf_dump_id("\t\t\t\t\tLogical volume id        ", 128, vatlvext_extattr->logvol_id,  &chsp);
				} else {
					printf("\t\t\t\t<Undumped %d bytes of implementation use data>\n", iu_l);
				}
				break;
			case 1 :	/* [4/48.10.3] : Character set information; UDF does allow/disallow explicitly */
				printf("\t\t\tCharacter set information attribute\n");
				printf("\t\t\t\t<Undumped %d bytes attribute>\n", extattr_len);
				break;
			case 3 :	/* [4/48.10.4] : Alternate permissions; UDF 3.3.4.2: not to be recorded */
				printf("\t\t\tAlternate permission attribute\n");
				printf("\t\t\t\t<Undumped %d bytes attribute>\n", extattr_len);
				break;
			case 5 :	/* [4/48.10.5] : File Times Extended Attribute */
			case 6 :	/* [4/48.10.6] : Information Times Extended Attribute; recorded in UDF ? */
				/* ASSUMPTION : bit fields are not exlusive */
				filetimes_extattr = (struct filetimes_extattr_entry *) pos;
				d_l = udf_rw32(filetimes_extattr->d_l);
				existence = udf_rw32(filetimes_extattr->existence);
				type_txt = "File";
				if (type == 6) type_txt = "File information";

				printf("\t\t\t%s times extended attribute\n", type_txt);
				timestamp = &filetimes_extattr->times[0];
				for (bit = 0; bit < 32; bit++) {
					if (d_l == 0) break;
					if ((existence & (1 << bit)) == 0)
						continue;
					switch (bit) {
						case 0 : /* File Creation Date and Time: the date and time of the day at which the file was created. */
							sprintf(what, "\t\t\t\t%s created at            ", type_txt);
							break;
						case 1 : /* Information Last Modification Date and Time: the date and time of the day at which the information in the file was last modified. */
							sprintf(what, "\t\t\t\t%s last modified at      ", type_txt);
							break;
						case 2 : /* File Deletion Date and Time: the date and time of the day after which the file may be deleted. */
							sprintf(what, "\t\t\t\t%s may be deleted after  ", type_txt);
							break;
						case 3 : /* File Effective Date and Time: the date and time of the day after which the file may be used. */
							sprintf(what, "\t\t\t\t%s may only be used after ", type_txt);
							break;
						case 5 : /* File Last Backup Date and Time: the date and time of the day at which the file was last backed up. */
							sprintf(what, "\t\t\t\t%s last backuped at       ", type_txt);
							break;
						default : /* unspec */
							sprintf(what, "\t\t\t\tUndefined meaning for %s time stamp ", type_txt);
							break;
					}
					udf_dump_timestamp(what, timestamp);
					d_l -= sizeof(struct timestamp);
					timestamp++;	/* advance */
				}
				break;
			case 12 :	/* [4/48.10.7] : Device Specification Extended Attribute */
				device_extattr = (struct device_extattr_entry *) pos;
				iu_l = udf_rw32(device_extattr->iu_l);
				printf("\t\t\tDevice node extended attribute\n");
				printf("\t\t\t\tMajor    %d\n", udf_rw32(device_extattr->major));
				printf("\t\t\t\tMinor    %d\n", udf_rw32(device_extattr->minor));
				if (iu_l >= sizeof(struct regid)) {
					udf_dump_regid("\t\t\t\tImplementator", (struct regid *) (device_extattr->data), UDF_REGID_IMPLEMENTATION);
				}
				break;
			default :
				printf("\t\t\tUndumped extended attribute type       %d\n", type);
				printf("\t\t\t\tSubtype                        %d\n", subtype);
				printf("\t\t\t\tLength                         %d\n", extattr_len);
				break;
		}
		if (extattr_len == 0) {
			printf("\t\t\tABORTing dump\n");
			break;
		}
		pos    += extattr_len;
		offset += extattr_len;
		length -= extattr_len;
	}
	printf("\n");
}


void udf_dump_extattr_hdr(struct extattrhdr_desc *eahd, uint32_t length) {
	uint32_t  hdr_len, impl_attr_loc, appl_attr_loc;
	uint8_t	 *pos;

	hdr_len = (uint32_t) sizeof(struct extattrhdr_desc);
	impl_attr_loc = udf_rw32(eahd->impl_attr_loc);
	appl_attr_loc = udf_rw32(eahd->appl_attr_loc);

	printf("\t\tExtended attributes header:\n");
	printf("\t\t\tLength                                    %d bytes\n", length);
	printf("\t\t\tImplementation attributes at offset       %d\n", impl_attr_loc);
	printf("\t\t\tApplication attributes at offset          %d\n", appl_attr_loc);
	printf("\t\t\tBytes remaining after header              %d\n", length - hdr_len);

	/* determine length of file related attributes space */
	pos     = (uint8_t *) eahd;
	pos    += hdr_len;
	length -= hdr_len;

	udf_dump_extattrseq(pos, hdr_len, impl_attr_loc, appl_attr_loc, length);
}


void udf_dump_file_entry(struct file_entry *fe) {
	uint8_t		*pos;
	uint32_t	 length;
	uint8_t		 addr_type;

	/* direct_entries = udf_rw32(fe->icbtag.prev_num_dirs); */
	//strat_param16  = udf_rw16(* (uint16_t *) (fe->icbtag.strat_param));
	//entries        = udf_rw16(fe->icbtag.max_num_entries);
	//strategy       = udf_rw16(fe->icbtag.strat_type);
	addr_type      = udf_rw16(fe->icbtag.flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK;

	printf("\tFile entry\n");
	udf_dump_icb_tag(&fe->icbtag);
	printf("\t\tUid                                         %d\n", udf_rw32(fe->uid));
	printf("\t\tGid                                         %d\n", udf_rw32(fe->gid));
	printf("\t\tPermissions                                 %x\n", udf_rw32(fe->perm));
	printf("\t\tLink count                                  %d\n", udf_rw16(fe->link_cnt));
	printf("\t\tRecord format                               %d\n", fe->rec_format);
	printf("\t\tRecord display attributes                   %d\n", fe->rec_disp_attr);
	printf("\t\tRecord length                               %d\n", fe->rec_len);
	printf("\t\tInformation length                          %"PRIu64"\n", (uint64_t) udf_rw64(fe->inf_len));
	printf("\t\tLogical blocks recorded                     %"PRIu64"\n", (uint64_t) udf_rw64(fe->logblks_rec));
	udf_dump_timestamp("\t\tAccess time                                ", &fe->atime);
	udf_dump_timestamp("\t\tModification time                          ", &fe->mtime);
	udf_dump_timestamp("\t\tAttribute time                             ", &fe->attrtime);
	printf("\t\tCheckpoint                                  %d\n", udf_rw32(fe->ckpoint));
	udf_dump_long_ad("\t\tExtended attributes ICB at", &fe->ex_attr_icb);
	udf_dump_regid("\t\tImplementation", &fe->imp_id, UDF_REGID_IMPLEMENTATION);
	printf("\t\tUniqueID                                    %d\n", (uint32_t) udf_rw64(fe->unique_id));
	printf("\t\tLength of extended attribute area           %d\n", udf_rw32(fe->l_ea));
	printf("\t\tLength of allocation descriptors            %d\n", udf_rw32(fe->l_ad));

	if (udf_rw32(fe->l_ea)) {
		udf_dump_extattr_hdr((struct extattrhdr_desc *) &fe->data[0], udf_rw32(fe->l_ea));
	}
	if (udf_rw32(fe->ex_attr_icb.len)) {
		printf("\t\t<Undumped %d bytes of extended attributes descriptor\n", udf_rw32(fe->ex_attr_icb.len));
	}

	printf("\t\tAllocation descriptors : \n");

	pos            = &fe->data[0] + udf_rw32(fe->l_ea);
	length         = udf_rw32(fe->l_ad);

	udf_dump_allocation_entries(addr_type, pos, length);
}


void udf_dump_extfile_entry(struct extfile_entry *efe) {
	uint8_t		*pos;
	uint32_t	 length;
	uint8_t		 addr_type;

	/* direct_entries = udf_rw32(efe->icbtag.prev_num_dirs); */
	//strat_param16  = udf_rw16(* (uint16_t *) (efe->icbtag.strat_param));
	//entries        = udf_rw16(efe->icbtag.max_num_entries);
	//strategy       = udf_rw16(efe->icbtag.strat_type);
	addr_type      = udf_rw16(efe->icbtag.flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK;

	printf("\tExtended file entry\n");
	udf_dump_icb_tag(&efe->icbtag);
	printf("\t\tUid                                         %d\n", udf_rw32(efe->uid));
	printf("\t\tGid                                         %d\n", udf_rw32(efe->gid));
	printf("\t\tPermissions                                 %x\n", udf_rw32(efe->perm));
	printf("\t\tLink count                                  %d\n", udf_rw16(efe->link_cnt));
	printf("\t\tRecord format                               %d\n", efe->rec_format);
	printf("\t\tRecord display attributes                   %d\n", efe->rec_disp_attr);
	printf("\t\tRecord length                               %d\n", efe->rec_len);
	printf("\t\tInformation length                          %"PRIu64"\n", (uint64_t) udf_rw64(efe->inf_len));
	printf("\t\tObject size                                 %"PRIu64"\n", (uint64_t) udf_rw64(efe->obj_size));
	printf("\t\tLogical blocks recorded                     %"PRIu64"\n", (uint64_t) udf_rw64(efe->logblks_rec));
	udf_dump_timestamp("\t\tAccess time                                ", &efe->atime);
	udf_dump_timestamp("\t\tModification time                          ", &efe->mtime);
	udf_dump_timestamp("\t\tCreation time                              ", &efe->ctime);
	udf_dump_timestamp("\t\tAttribute time                             ", &efe->attrtime);
	printf("\t\tCheckpoint                                  %d\n", udf_rw32(efe->ckpoint));
	udf_dump_long_ad("\t\tExtended attributes ICB at", &efe->ex_attr_icb);
	udf_dump_long_ad("\t\tStreamdir ICB at", &efe->streamdir_icb);
	udf_dump_regid("\t\tImplementation", &efe->imp_id, UDF_REGID_IMPLEMENTATION);
	printf("\t\tUniqueID                                    %d\n", (uint32_t) udf_rw64(efe->unique_id));
	printf("\t\tLength of extended attribute area           %d\n", udf_rw32(efe->l_ea));
	printf("\t\tLength of allocation descriptors            %d\n", udf_rw32(efe->l_ad));

	if (udf_rw32(efe->l_ea)) {
		udf_dump_extattr_hdr((struct extattrhdr_desc *) &efe->data[0], udf_rw32(efe->l_ea));
	}
	if (udf_rw32(efe->ex_attr_icb.len)) {
		printf("\t\t<Undumped %d bytes of extended attributes descriptor\n", udf_rw32(efe->ex_attr_icb.len));
	}

	printf("\t\tAllocation descriptors : \n");
	
	pos            = &efe->data[0] + udf_rw32(efe->l_ea);
	length         = udf_rw32(efe->l_ad);

	udf_dump_allocation_entries(addr_type, pos, length);
}


/* dump allocation extention descriptor */
void udf_dump_alloc_extent(struct alloc_ext_entry *ext, int addr_type) {
	uint8_t		*pos;
	uint32_t	 length;
	int		 isshort, islong;

	/* note we DONT know if its filled with short_ad's or long_ad's! */
	printf("\tAllocation Extent descriptor\n");
	printf("\t\tPrevious entry                              %d\n", udf_rw32(ext->prev_entry));
	printf("\t\tLength of allocation descriptors            %d\n", udf_rw32(ext->l_ad));

	pos    =  &ext->data[0];
	length =  udf_rw32(ext->l_ad);

	if (addr_type < 0) {
		isshort = ((length % sizeof(struct short_ad)) == 0);
		islong  = ((length % sizeof(struct long_ad)) == 0);
		if (isshort)
			addr_type = UDF_ICB_SHORT_ALLOC;
		if (islong)
			addr_type = UDF_ICB_LONG_ALLOC;
		if (!(isshort ^ islong)) {
			printf("\t\tCan't determine if its filled with long_ad's or short_ad's !\n");
			return;
		}
	}

	udf_dump_allocation_entries(addr_type, pos, length);
}


/* dump a space table(entry) descriptor */
void udf_dump_space_entry(struct space_entry_desc *sed) {
	union icb *icb;
	uint32_t addr_type, size, bytes;
	uint32_t piece_sector, piece_length, piece_part;
	uint8_t *pos;

	printf("\tSpace entry table\n");
	udf_dump_icb_tag(&sed->icbtag);
	printf("\t\tSize in bytes                               %d\n", udf_rw32(sed->l_ad));

	pos   = &sed->entry[0];
	bytes = udf_rw32(sed->l_ad);

	addr_type = udf_rw16(sed->icbtag.flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK;
	while (bytes) {
		size = piece_length = piece_sector = piece_part = 0;
		icb = (union icb *) pos;
		switch (addr_type) {
			case UDF_ICB_SHORT_ALLOC :
				piece_length = udf_rw32(icb->s_ad.len) & (((uint32_t) 1<<31)-1);
				piece_sector = udf_rw32(icb->s_ad.lb_num);
				printf("[at sec %u for %d bytes] ", piece_sector, piece_length);
				size = sizeof(struct short_ad);
				break;
			case UDF_ICB_LONG_ALLOC :
				piece_length = udf_rw32(icb->l_ad.len) & (((uint32_t) 1<<31)-1);
				piece_sector = udf_rw32(icb->l_ad.loc.lb_num);
				piece_part   = udf_rw16(icb->l_ad.loc.part_num);
				size = sizeof(struct long_ad);
				printf("[at sec %u for %d bytes at partition %d] ", piece_sector, piece_length, piece_part);
				break;
			case UDF_ICB_EXT_ALLOC :
			case UDF_ICB_INTERN_ALLOC :
				printf("\t\t\tWARNING : an internal alloc in a space entry?\n");
				return;
		}
		bytes -= size;
	}
}


/* dump a space bitmap descriptor */
void udf_dump_space_bitmap(struct space_bitmap_desc *sbd) {
	uint32_t bits, from, now, cnt;
	uint8_t byte, bit, bitpos, state, *pos;

	printf("\t\tSpace bitmap\n");
	printf("\t\t\tNumber of bits                      %u\n", udf_rw32(sbd->num_bits));
	printf("\t\t\tNumber of bytes                     %u\n", udf_rw32(sbd->num_bytes));
	printf("\t\t\tMarked parts at :\n");

	pos = sbd->data;
	bits = udf_rw32(sbd->num_bits);

	/* shield */
	/* if (bits > 2000*8) bits = 2000*8; */

	printf("\t\t\t\t");
	cnt = 0; from = 0; now = 0; bitpos = 0; byte = *pos; state = byte & 1;
	while (now < bits) {
		if (bitpos == 0) {
			byte = *pos++;
		}
		bit = byte & 1;
		if (bit != state) {
			if (state) {
				printf("[%08u - %08u]", from, now-1);
				if (cnt % 4 == 3) printf("\n\t\t\t\t"); else printf("    ");
				cnt++;
			}
			from = now;
			state = bit;
		}
		byte >>= 1;
		bitpos = (bitpos+1) & 7;
		now++;
	}
	if (state) printf("[%08u - %08u]", from, now);
	if (bits < udf_rw32(sbd->num_bits)) printf(" .... <trimmed>\n");
}


/* main descriptor `dump' function */
void udf_dump_descriptor(union dscrptr *dscrpt) {
	struct desc_tag *tag = &dscrpt->tag;
	int error;

	if (!dscrpt)
		return;

	/* check if its a valid descritor */
	if (udf_rw16(tag->id == 0) && udf_rw16(tag->descriptor_ver) == 0) return;

	udf_dump_desc(tag);

	error = udf_check_tag(dscrpt);
	if (error) {
		printf("\tBAD TAG\n");
		return;
	}
	switch (udf_rw16(tag->id)) {
		case TAGID_SPARING_TABLE :
			udf_dump_sparing_table(&dscrpt->spt);
			break;
		case TAGID_PRI_VOL :
			udf_dump_pri_vol(&dscrpt->pvd);
			break;
		case TAGID_ANCHOR :
			udf_dump_anchor(&dscrpt->avdp);
			break;
		case TAGID_VOL :
			udf_dump_unimpl(dscrpt, "volume descriptor");
			break;
		case TAGID_IMP_VOL :
			udf_dump_implementation_volume(&dscrpt->ivd);
			break;
		case TAGID_PARTITION :
			udf_dump_part(&dscrpt->pd);
			break;
		case TAGID_LOGVOL :
			udf_dump_log_vol(&dscrpt->lvd);
			break;
		case TAGID_UNALLOC_SPACE :
			udf_dump_unalloc_space(&dscrpt->usd);
			break;
		case TAGID_TERM :
			udf_dump_terminating_desc(dscrpt);
			break;
		case TAGID_LOGVOL_INTEGRITY :
			udf_dump_logvol_integrity(&dscrpt->lvid);
			break;
		case TAGID_FSD :
			udf_dump_fileset_desc(&dscrpt->fsd);
			break;
		case TAGID_FID :
			udf_dump_fileid(&dscrpt->fid);
			break;
		case TAGID_ALLOCEXTENT :
			udf_dump_alloc_extent(&dscrpt->aee, -1);
			break;
		case TAGID_INDIRECTENTRY :
			udf_dump_indirect_entry(&dscrpt->inde);
			break;
		case TAGID_FENTRY :
			udf_dump_file_entry(&dscrpt->fe);
			break;
		case TAGID_EXTATTR_HDR :
			udf_dump_extattr_hdr(&dscrpt->eahd, sizeof(struct extattrhdr_desc));
			break;
		case TAGID_UNALL_SP_ENTRY :
			udf_dump_space_entry(&dscrpt->sed);
			break;
		case TAGID_SPACE_BITMAP :
			udf_dump_space_bitmap(&dscrpt->sbd);
			break;
		case TAGID_PART_INTEGRITY :
			udf_dump_unimpl(dscrpt, "partition integrity");
			break;
		case TAGID_EXTFENTRY :
			udf_dump_extfile_entry(&dscrpt->efe);
			break;
		default :
			break;
	}
	printf("\n");
}


/* this one is special since the VAT table has no tag but is a file */
void udf_dump_vat_table(struct udf_part_mapping *udf_part_mapping) {
	struct charspec  chsp;
	struct udf_vat  *vat;
	uint32_t	 previous_vat, entry, vat_entries, *vat_pos, version;

	/* prolly OSTA compressed unicode anyway */
	chsp.type = 0;
	strcpy((char *) chsp.inf, "OSTA Compressed Unicode");

	vat = udf_part_mapping->vat;
	printf("\tVAT table: ");
	printf("%s UDF 2.00 format\n", vat?"post":"pre");

	vat_entries = udf_part_mapping->vat_entries;
	vat_pos = (uint32_t *) udf_part_mapping->vat_translation;
	if (vat) {
		printf("\t\tHeader length                        %d\n", udf_rw16(vat->header_len));
		printf("\t\tImplementation use length            %d\n", udf_rw16(vat->impl_use_len));
		udf_dump_id("\t\tLogical volume id                   ", 128, vat->logvol_id,  &chsp);
		printf("\t\tNumber of files                      %d\n", udf_rw32(vat->num_files));
		printf("\t\tNumber of directories                %d\n", udf_rw32(vat->num_directories));
		version = udf_rw16(vat->min_udf_readver);
		printf("\t\tMinimum readversion                  UDFv %x\n", version);
		version = udf_rw16(vat->min_udf_writever);
		printf("\t\tMinimum writeversion                 UDFv %x\n", version);
		version = udf_rw16(vat->max_udf_writever);
		printf("\t\tMaximum writeversion                 UDFv %x\n", version);
		if (udf_rw16(vat->impl_use_len)) printf("\t\t<undumped implementation use area>");
		previous_vat = udf_rw32(vat->prev_vat);
	} else {
		udf_dump_regid("\t\tIdentifier id (can be wrong)        ", (struct regid *) (vat_pos+vat_entries), UDF_REGID_NAME);
		previous_vat = udf_rw32(*(vat_pos + vat_entries + 32/4));			/* definition */
	}
	if (previous_vat == 0xffffffff) {
		printf("\t\tNo previous VAT recorded\n");
	} else {
		printf("\t\tPrevious VAT recorded at offset      %d\n", previous_vat);
	}

	printf("\t\tNumber of VAT entries                %d\n", vat_entries);
	printf("\t\tVAT dump :");
	for (entry=0; entry < vat_entries; entry++) {
		if ((entry % 4) == 0) printf("\n\t");
		printf("[0x%08x -> 0x%08x] ", entry, *vat_pos++);
	}
	printf("\n");
}


void udf_dump_volumeset_info(struct udf_volumeset *udf_volumeset) {
	struct udf_pri_vol	*primary;
	struct udf_log_vol	*logical;
	struct udf_partition	*udf_partition;
	struct udf_part_mapping *udf_part_mapping;
	struct udf_discinfo	*disc;
	char			*name;
	int			 num_volumes, num_partitions;
	int			 subvolume, part_num, track_num;

	num_volumes = 0;	/* shut up gcc */
	if (udf_volumeset->obsolete) return;

	primary = STAILQ_FIRST(&udf_volumeset->primaries);
	if (primary) {
		num_volumes =  udf_rw16(primary->pri_vol->max_vol_seq);
		if (udf_volumeset->obsolete) printf("OBSOLETE\n");	/* XXX */

		printf("Volume set ");
		udf_dump_id(NULL, 32, primary->pri_vol->volset_id, &primary->pri_vol->desc_charset);
		printf(" (%d volume%s) ", num_volumes, num_volumes>1?"s":"");

		num_partitions = udf_volumeset->max_partnum;
		printf("with %d partition%s\n", num_partitions, (num_partitions!=1)?"s":"");

		/* better loop trough the partition numbers to display them in a defined order */
		SLIST_FOREACH(udf_partition, &udf_volumeset->parts, next_partition) {
			part_num = udf_rw16(udf_partition->partition->part_num);
			if (udf_partition) {
				/* there is information */
				assert(udf_partition->udf_session);
				assert(udf_partition->udf_session->disc);
				assert(udf_partition->partition);
				assert(part_num == udf_rw16(udf_partition->partition->part_num));
				track_num = udf_partition->udf_session->session_num;
				disc      = udf_partition->udf_session->disc;

				printf("\tPartition number %d at device `%s' session %d from sector %d(+%d) for %u sectors\n",
						part_num,
						disc->dev->dev_name,
						track_num,
						udf_rw32(udf_partition->partition->start_loc),
						udf_partition->udf_session->session_offset,
						udf_rw32(udf_partition->partition->part_len)
				      );
			} else {
				printf("\tUnknown partition %d [unknown]\n", part_num);
			}
		}
	}

	STAILQ_FOREACH(primary, &udf_volumeset->primaries, next_primary) {
		subvolume = udf_rw16(primary->pri_vol->vds_num);

		printf("\tPrimary volume ");
		udf_dump_id(NULL, 32, primary->pri_vol->vol_id, &primary->pri_vol->desc_charset);
		printf(" (part %d/%d) ", subvolume, num_volumes);

		printf("created by implementator `%s' ", primary->pri_vol->imp_id.id);
		if (*primary->pri_vol->app_id.id)
			printf("by/for application `%s' ",primary->pri_vol->app_id.id);
		printf("\n");

		SLIST_FOREACH(logical, &primary->log_vols, next_logvol) {
			name = logical->log_vol->logvol_id;
			udf_dump_id("\t\tcontains logical volume ", 128, name, &logical->log_vol->desc_charset);
			if (logical->broken) {
				printf("\t\t\tBROKEN\n");
				continue;
			}

			SLIST_FOREACH(udf_part_mapping, &logical->part_mappings, next_mapping) {
				printf("\t\t\tmapping %d on %d as ", udf_part_mapping->udf_virt_part_num,
						udf_part_mapping->udf_phys_part_num);
				switch (udf_part_mapping->udf_part_mapping_type) {
					case UDF_PART_MAPPING_ERROR :
						printf("bad partition");
						break;
					case UDF_PART_MAPPING_PHYSICAL :
						printf("direct");
						break;
					case UDF_PART_MAPPING_VIRTUAL :
						printf("virtual partition");
						break;
					case UDF_PART_MAPPING_SPARABLE :
						printf("sparable");
						break;
					case UDF_PART_MAPPING_META :
						printf("metadata only");
				}
				printf(" recording");
				if (udf_part_mapping->data_writable) printf(" data");
				if (udf_part_mapping->metadata_writable) printf(" metadata");
				if (!udf_part_mapping->data_writable && !udf_part_mapping->metadata_writable) printf(" nothing");
				printf("\n");
			}
		}
		printf("\n");
	}
}


void udf_dump_alive_sets(void) {
	struct udf_volumeset *udf_volumeset;

	printf("UDF volume sets marked alive :\n");
	SLIST_FOREACH(udf_volumeset, &udf_volumeset_list, next_volumeset) {
		udf_dump_volumeset_info(udf_volumeset);
	}
	printf("\n");
}


/*
 * extern defined read_logvol_descriptor breaks splitting rules but how
 * otherwise to provide a detailed description of the file entry udf_node
 */

#define DUMP_DIRBUFFER_SIZE (16*1024)
void udf_dump_file_entry_node(struct udf_node *udf_node, char *prefix) {
	struct long_ad   udf_icbptr;
	struct uio       dir_uio;
	struct iovec     dir_iovec;
	struct dirent   *dirent;
	struct fileid_desc *fid;
	struct udf_node *entry_node;
	uint32_t         pos, lb_size;
	uint8_t         *buffer;
	char             fullpath[1024];	/* XXX arbitrary length XXX */
	int              isdot, isdotdot, isdir, found, eof;
	int		 error;

	if (!udf_node)
		return;

	/* XXX could pass on dirent XXX */
	isdir  = (udf_node->udf_filetype == UDF_ICB_FILETYPE_DIRECTORY);
	isdir |= (udf_node->udf_filetype == UDF_ICB_FILETYPE_STREAMDIR);
	if (isdir) {
		buffer = malloc(DUMP_DIRBUFFER_SIZE);
		if (!buffer)
			return;
		lb_size = udf_node->udf_log_vol->lb_size;
		fid = malloc(lb_size);
		assert(fid);	/* or just return? */

		/* recurse into this directory */
		dir_uio.uio_offset = 0;			/* begin at start */
		do {
			dir_iovec.iov_base = buffer;
			dir_iovec.iov_len  = DUMP_DIRBUFFER_SIZE;
			dir_uio.uio_resid  = DUMP_DIRBUFFER_SIZE;
			dir_uio.uio_iovcnt = 1;
			dir_uio.uio_iov    = &dir_iovec;
			dir_uio.uio_rw     = UIO_WRITE;

			error = udf_readdir(udf_node, &dir_uio, &eof);
			if (error) {
				printf("While reading in dirbuffer for dumping file entry udf_node : %s\n", strerror(error));
				break;
			}
			pos = 0;
			while (pos < DUMP_DIRBUFFER_SIZE - dir_uio.uio_resid) {
				dirent = (struct dirent *) (buffer + pos);

				sprintf(fullpath, "%s/%s", prefix, dirent->d_name);

				/* looking for '.' or '..' ? */
				isdot    = (strncmp(dirent->d_name, ".",  DIRENT_NAMLEN(dirent)) == 0);
				isdotdot = (strncmp(dirent->d_name, "..", DIRENT_NAMLEN(dirent)) == 0);

				pos += sizeof(struct dirent);	/* XXX variable size dirents possible XXX */

				if (isdotdot)
					continue;

				if (isdot)
					continue;

				error = udf_lookup_name_in_dir(udf_node, dirent->d_name, DIRENT_NAMLEN(dirent), &udf_icbptr, fid, &found);
				if (!error) {
					error = ENOENT;
					if (found)
						error = udf_readin_udf_node(udf_node, &udf_icbptr, fid, &entry_node);
				}
				if (!error)
					udf_dump_file_entry_node(entry_node, fullpath);
			}
		} while (!eof);

		free(fid);
		free(buffer);
		return;
	}
	/* leaf udf_node */
	printf("%s\n", prefix);
}
#undef DUMP_DIRBUFFER_SIZE


void udf_dump_root_dir(struct udf_mountpoint *mountpoint) {
	printf("\n\nRoot dir dump\n");
	if (mountpoint->rootdir_node)   udf_dump_file_entry_node(mountpoint->rootdir_node, ":Rootdir");

	printf("\n\nStreamdir dump\n");
	if (mountpoint->streamdir_node) udf_dump_file_entry_node(mountpoint->streamdir_node, ":Streamdir");
}


/* XXX These should move to form one cd verbose file with cd_discect XXX */
static char *print_disc_state(int state) {
	switch (state) {
		case 0: return "empty disc";
		case 1: return "incomplete (appendable)";
		case 2: return "full (not appendable)";
		case 3: return "random writable";
	}
	return "unknown disc state";
}


static char *print_session_state(int state) {
	switch (state) {
		case 0 : return "empty";
		case 1 : return "incomplete";
		case 2 : return "reserved/damaged";
		case 3 : return "complete/closed disc";
	}
	return "unknown session_state";
}


static char *print_mmc_profile(int profile) {
	static char scrap[100];

	switch (profile) {
		case 0x00 : return "Unknown[0] profile";
		case 0x01 : return "Non removable disc";
		case 0x02 : return "Removable disc";
		case 0x03 : return "Magneto Optical with sector erase";
		case 0x04 : return "Magneto Optical write once";
		case 0x05 : return "Advance Storage Magneto Optical";
		case 0x08 : return "CD-ROM";
		case 0x09 : return "CD-R recordable";
		case 0x0a : return "CD-RW rewritable";
		case 0x10 : return "DVD-ROM";
		case 0x11 : return "DVD-R sequential";
		case 0x12 : return "DVD-RAM rewritable";
		case 0x13 : return "DVD-RW restricted overwrite";
		case 0x14 : return "DVD-RW sequential";
		case 0x1a : return "DVD+RW rewritable";
		case 0x1b : return "DVD+R recordable";
		case 0x20 : return "DDCD readonly";
		case 0x21 : return "DDCD-R recodable";
		case 0x22 : return "DDCD-RW rewritable";
		case 0x2b : return "DVD+R double layer";
		case 0x40 : return "BD-ROM";
		case 0x41 : return "BD-R Sequential Recording (SRM)";
		case 0x42 : return "BD-R Random Recording (RRM)";
		case 0x43 : return "BD-RE rewritable";
	}
	sprintf(scrap, "Reserved profile 0x%02x", profile);
	return scrap;
}


void udf_dump_discinfo(struct udf_discinfo *disc) {
	int session;

	printf("Disc info for disc in device %s\n", disc->dev->dev_name);
	printf("\tMMC profile        : %s\n", print_mmc_profile(disc->mmc_profile));
	printf("\tsequential         : %s\n", disc->sequential       ?"yes":" no");
	printf("\trecordable         : %s\n", disc->recordable       ?"yes":" no");
	printf("\terasable           : %s\n", disc->erasable         ?"yes":" no");
	printf("\tblankable          : %s\n", disc->blankable        ?"yes":" no");
	printf("\tformattable        : %s\n", disc->formattable      ?"yes":" no");
	printf("\trewritable         : %s\n", disc->rewritable       ?"yes":" no");
	printf("\tmount raineer      : %s\n", disc->mrw              ?"yes":" no");
	printf("\tpacket writing     : %s\n", disc->packet           ?"yes":" no");
	printf("\tstrict overwrite   : %s\n", disc->strict_overwrite ?"yes":" no");
	printf("\tblocking number    : %d\n", disc->blockingnr);
	printf("\tdisc state         : %s\n", print_disc_state(disc->disc_state));
	printf("\tlast session state : %s\n", print_session_state(disc->last_session_state));
	printf("\tsectorsize         : %d\n", disc->sector_size);
	printf("\tNumber of sessions     %d\n", disc->num_sessions);
	for (session = 0; session < disc->num_sessions; session++) {
		printf("\tSession %d\n", session);
		printf("\t\tstart  at         %u\n", (uint32_t) disc->session_start[session]);
		printf("\t\tends   at         %u\n", (uint32_t) disc->session_end[session]);
		printf("\t\tlength for        %u\n", (uint32_t) (disc->session_end[session] - disc->session_start[session]));
		printf("\t\tnext writable at  %u\n", disc->next_writable[session]);
		printf("\t\tfree blocks       %u\n", disc->free_blocks[session]);
		printf("\t\tpacket size       %u\n", disc->packet_size[session]);
		printf("\n");
	}
}

/* end of udf_verbose.c */

