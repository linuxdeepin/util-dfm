/* $NetBSD$ */

/*
 * File "udf_discinfo.c" is part of the UDFclient toolkit.
 * File $Id: udf_discop.c,v 1.82 2022/02/06 14:30:49 reinoud Exp $ $Name:  $
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


/* XXX strip this XXX */
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <limits.h>

#include "uscsilib.h"


/* for locals */
#include "uscsilib.h"
#include "udf_discop.h"


#ifndef MAX
#	define MAX(a,b) ((a)>(b)?(a):(b))
#	define MIN(a,b) ((a)<(b)?(a):(b))
#endif


/* #define DEBUG(a) { a; } */
#define DEBUG(a) if (0) { a; }


/* globals */


/******************************************************************************************
 *
 * Tables and helper functions section
 *
 ******************************************************************************************/

int read_cd_hex2(int val) {
	int nl, nh;

	nl = val & 15;
	nh = val >> 4;
	if (nl >= 'A') nl -= 'A' + 10;
	if (nh >= 'A') nh -= 'A' + 10;

	return (nh*16) + nl;
}


int read_cd_bcd(int val) {
	int nl, nh;

	nl = (val & 15) - '0';
	nh = (val >> 4) - '0';
	if ((nl < 0 || nl > 9) || (nh < 0 || nh > 9)) return val;

	return nh*10 + nl;
}


int32_t cd_msf2lba(int h, int m, int s, int f) {
	return 270000*h + 4500*m + 75*s + f - 150;
}


/******************************************************************************************
 *
 * Disc level operations 
 *
 ******************************************************************************************/

int udf_discinfo_is_cd_or_dvd(struct udf_discinfo *disc) {
	/* check device type */
	switch (disc->devdrv_class & UDF_DEVDRV_CLASS) {
		case UDF_DEVDRV_CLASS_FILE :
		case UDF_DEVDRV_CLASS_DISC :
			/* not nessisary */
			return 0;
		case UDF_DEVDRV_CLASS_CD   :
		case UDF_DEVDRV_CLASS_DVD  :
			/* it really is */
			return 1;
		default :
			break;
	}
	return ENODEV;
}


int udf_discinfo_check_disc_ready(struct udf_discinfo *disc) {
	scsicmd cmd;
	uint8_t buf[36];
	int error;

	if (!udf_discinfo_is_cd_or_dvd(disc)) return 1;

	bzero(cmd, SCSI_CMD_LEN);
	cmd[0] = 0;				/* test unit ready */
	error = uscsi_command(SCSI_READCMD, disc->dev, cmd, 6, buf, 0, 30000, NULL);

	return (error == 0);
}


#define blk_len 10000
int udf_discinfo_set_recording_parameters(struct udf_discinfo *discinfo, int testwriting) {
	scsicmd  cmd;
	uint8_t  res[blk_len];
	uint8_t *pos;
	uint32_t blockingnr;
	int      val_len, packet;
	int      error;

	if (!udf_discinfo_is_cd_or_dvd(discinfo)) return 0;

	/* Set up CD/DVD recording parameters */
	if (!discinfo->recordable) return 0;

	blockingnr = discinfo->blockingnr;

	DEBUG(printf("Setting device's recording parameters\n"));
	packet = discinfo->packet;

	val_len = 0x32+2+8;
	bzero(res, val_len);

	pos = res + 8;

	bzero(cmd, SCSI_CMD_LEN);
	if (!packet) {
		pos[ 0] = 0x05;			/* page code 5 : cd writing					*/
		pos[ 1] = 0x32;			/* length in bytes						*/
		pos[ 2] = 64 + 0;		/* BUFE + write type 1 : track at once				*/
		if (testwriting) pos[ 2] += 16;
		pos[ 3] = (3<<6) | 5;		/* next session OK, data packet, rec. incr. var packets		*/
		pos[ 4] = 8;			/* ISO mode 1							*/
		pos[ 8] = 0;			/* normal CD-DA/CD-ROM or data disc				*/
		DEBUG(printf("\tsetting up for sequential writing\n"));
	} else {
		pos[ 0] = 0x05;			/* page code 5 : cd writing					*/
		pos[ 1] = 0x32;			/* length in bytes						*/
		pos[ 2] = 0;			/* write type 0 : packet/incremental				*/
		if (testwriting) pos[ 2] += 16;
		pos[ 3] = (3<<6) | 32 | 5;	/* next session OK, data packet, rec. incr. fixed packets	*/
		pos[ 4] = 10;			/* ISO mode 2; XA form 1					*/
		pos[ 8] = 0x20;			/* CD-ROM XA disc or DDCD disc */
		pos[10] = (blockingnr >> 24) & 0xff;	/* MSB packet size 		*/
		pos[11] = (blockingnr >> 16) & 0xff;
		pos[12] = (blockingnr >>  8) & 0xff;
		pos[13] = (blockingnr      ) & 0xff;	/* LSB packet size in SECTORS	*/
		DEBUG(printf("\tsetting up for packet writing with packet size %d\n", blockingnr));
	}

	bzero(cmd, SCSI_CMD_LEN);
	cmd[0] = 0x55;			/* MODE SELECT (10)		*/
	cmd[1] = 16;			/* PF format			*/
	cmd[7] = val_len >> 8;		/* length of blob		*/
	cmd[8] = val_len & 0xff;
	cmd[9] = 0;			/* control			*/

	error = uscsi_command(SCSI_WRITECMD, discinfo->dev, cmd, 10, res, val_len, 3000, NULL);
	if (error) {
		perror("While WRTITING parameter page 5");
		return error;
	}

#if 0
	/* Set CD/DVD speed to 'optimal' for it doesnt seem to do it automatically */
	bzero(cmd, SCSI_CMD_LEN);
	cmd[ 0] = 0xBB;				/* Set CD speed */
	cmd[ 1] = 1;				/* select CAV (1) or CLV (0) recording */
	cmd[ 2] = 0xff;
	cmd[ 3] = 0xff;				/* max read performance speed */
	cmd[ 4] = 0xff;
	cmd[ 5] = 0xff;				/* max write performance speed; applic? */
	cmd[11] = 0;				/* control */
	error = scsi_call(SCSI_WRITECMD, discinfo, cmd, 12, NULL, 0, NULL);
	if (error) {
		/* CAV not possible? then go for CLV */
		cmd[ 1] = 0;				/* select CAV (1) or CLV (0) recording */
		error = scsi_call(SCSI_WRITECMD, discinfo, cmd, 12, NULL, 0, NULL);
		if (error) {
			perror("While setting speed");
			return error;
		}
	}
#endif

	/* flag OK */
	return 0;
}
#undef blk_len


int udf_discinfo_synchronise_caches(struct udf_discinfo *discinfo) {
	scsicmd cmd;
	int	error;

	/* bail out when we're in sequential emulation */
	if (!udf_discinfo_is_cd_or_dvd(discinfo))
		return 0;

	bzero(cmd, SCSI_CMD_LEN);
	cmd[ 0] = 0x35;				/* Synchronise cache	*/
	cmd[ 9] = 0;				/* control		*/
	error = uscsi_command(SCSI_WRITECMD, discinfo->dev, cmd, 10, NULL, 0, 30000, NULL);
	if (error) {
		perror("While synchronising write cache");
	}

	return error;
}


/*
 * important: it must be called after write operations before read operations
 * are allowed again. When its allready finished with writing this call has no
 * effect and can be called at start to make sure the device knows that we're
 * going to read.
 */
int udf_discinfo_finish_writing(struct udf_discinfo *discinfo) {
	int error;

	if (!udf_discinfo_is_cd_or_dvd(discinfo))
		return 0;

	error = udf_discinfo_synchronise_caches(discinfo);
	return error;
}


int udf_discinfo_reserve_track_in_logic_units(struct udf_discinfo *discinfo, uint32_t logic_units) {
	scsicmd cmd;
	int	error;

	if (!udf_discinfo_is_cd_or_dvd(discinfo)) return ENODEV;

	bzero(cmd, SCSI_CMD_LEN);
	cmd[ 0] = 0x53;				/* reserve track	*/
	cmd[ 5] = (logic_units  >> 24) & 0xff;	/* size MSB		*/
	cmd[ 6] = (logic_units  >> 16) & 0xff;
	cmd[ 7] = (logic_units  >>  8) & 0xff;
	cmd[ 8] = (logic_units       ) & 0xff;	/* size LSB		*/
	cmd[ 9] = 0;				/* control		*/
	error = uscsi_command(SCSI_WRITECMD, discinfo->dev, cmd, 10, NULL, 0, 30000, NULL);

	return error;
}


int udf_discinfo_close_track(struct udf_discinfo *discinfo, uint16_t trackno) {
	scsicmd cmd;
	int	error;

	if (!udf_discinfo_is_cd_or_dvd(discinfo)) return ENODEV;

	bzero(cmd, SCSI_CMD_LEN);
	cmd[ 0] = 0x5B;				/* close session/track	*/
	cmd[ 2] = 1;				/* track 		*/
	cmd[ 4] = (trackno      >>  8) & 0xff;	/* specify trackno MSB	*/
	cmd[ 5] = (trackno           ) & 0xff;	/*         trackno LSB	*/
	cmd[ 9] = 0;				/* control		*/
	error = uscsi_command(SCSI_WRITECMD, discinfo->dev, cmd, 10, NULL, 0, 30000, NULL);

	return error;
}


/* can only close last session */
int udf_discinfo_close_session(struct udf_discinfo *discinfo) {
	scsicmd cmd;
	int error;

	if (!udf_discinfo_is_cd_or_dvd(discinfo)) return ENODEV;

	bzero(cmd, SCSI_CMD_LEN);
	cmd[ 0] = 0x5B;				/* close session/track	*/
	cmd[ 2] = 2;				/* session 		*/
	cmd[ 9] = 0;				/* control		*/
	error = uscsi_command(SCSI_WRITECMD, discinfo->dev, cmd, 10, NULL, 0, 30000, NULL);

	return error;
}


/*
 * Repair a damaged track when suspected. It'll try to make it writable again.
 * A track can be broken when sudden stops are made and the track end is left
 * in a inconsistent state in the ATIP/PMA/TOC.
 */
int udf_discinfo_repair_track(struct udf_discinfo *discinfo, uint16_t trackno) {
	scsicmd cmd;
	int	error;

	if (!udf_discinfo_is_cd_or_dvd(discinfo)) return ENODEV;

	bzero(cmd, SCSI_CMD_LEN);
	cmd[ 0] = 0x58;				/* repair track		*/
	cmd[ 4] = (trackno      >>  8) & 0xff;	/* specify trackno MSB	*/
	cmd[ 5] = (trackno           ) & 0xff;	/*         trackno LSB	*/
	cmd[ 9] = 0;				/* control		*/
	error = uscsi_command(SCSI_WRITECMD, discinfo->dev, cmd, 10, NULL, 0, 30000, NULL);

	return error;
}


/*
 * This routine 'get disc type' tries to get operational information from the
 * disc/drive combination and its capabilities.  Fills in devdrv_class, MMC
 * profile and the various flags; no track info.
 */

int udf_discinfo_get_disc_type(struct udf_discinfo *disc) {
	struct stat stat;
	scsicmd	    cmd;
	const int   feat_tbl_len = 0x200;
	uint8_t	    buf[256];
	uint8_t	    features[feat_tbl_len], *rpos, *fpos;
	uint32_t    pos, features_len, val_len;
	uint32_t    feature, last_feature;
	uint32_t    feature_ver, feature_pers, feature_cur, feature_len;
	int         error;

	/* assume generic CD-ROM with no known MMC profile */
	disc->devdrv_class = UDF_DEVDRV_CLASS_CD;
	disc->mmc_profile  = 0;

	/* check if its a regular file */
	fstat(disc->dev->fhandle, &stat);
	if (S_ISREG(stat.st_mode)) {
		UDF_VERBOSE(printf("UDF device %s is a regular file\n", disc->dev->dev_name));
		disc->devdrv_class     = UDF_DEVDRV_CLASS_FILE;
		disc->sequential       = 0;	/* full r/w		*/
		disc->recordable       = 1;	/* assuming rw access	*/
		disc->blankable        = 0;	/* not applicable       */
		disc->rewritable       = 1;
		disc->packet           = 0;
		disc->blockingnr       = 1;
		disc->strict_overwrite = 0;
		disc->sector_size      = DISC_SECTOR_SIZE;
		return 0;
	}

	/* check if its a ATAPI/SCSI device */
	error = uscsi_check_for_scsi(disc->dev);
	if (error) {
		/* obviously no ATIPI/SCSI device -> has to be IDE disc or other but no file either */
		disc->devdrv_class     = UDF_DEVDRV_CLASS_DISC;
		disc->sequential       = 0;	/* full r/w		*/
		disc->recordable       = 1;	/* assuming rw access	*/
		disc->blankable        = 0;	/* not applicable       */
		disc->rewritable       = 1;
		disc->packet           = 0;
		disc->blockingnr       = 1;
		disc->strict_overwrite = 0;
		disc->sector_size      = DISC_SECTOR_SIZE;
		UDF_VERBOSE(printf("Got error executing SCSI command, assuming IDE disc\n"));
		return 0;
	}

	/* check if its a SCSI disc -> if so, do NOT issue mmc profile check */
	bzero(cmd, SCSI_CMD_LEN);
	cmd[0] = 0x12;	/* INQUIRY */
	cmd[1] = 0;	/* basic inquiry */
	cmd[2] = 0;	/* no page or operation code */
	cmd[3] = 0;	/* reserved/MSB result */
	cmd[4] = 96;	/* all but vendor specific */
	cmd[5] = 0;	/* control */
	error = uscsi_command(SCSI_READCMD, disc->dev, cmd, 6, buf, 96, 30000, NULL);
	if (error) {
		fprintf(stderr, "Device claims to be SCSI but does NOT respond to inquiry!\n");
		return ENOENT;
	}
	disc->scsi_device_type = buf[0] & 0x1f;

	switch (disc->scsi_device_type) {
		case 0x05 : break; /* MMC */
		case 0x00 : /* Direct access device */
		case 0x0E : /* Simplified direct access device */
			/* read-write possible */
			disc->devdrv_class     = UDF_DEVDRV_CLASS_DISC;
			disc->sequential       = 0;	/* full r/w		*/
			disc->recordable       = 1;	/* assuming rw access	*/
			disc->blankable        = 0;	/* not applicable       */
			disc->rewritable       = 1;
			disc->packet           = 0;
			disc->blockingnr       = 1;
			disc->strict_overwrite = 0;
			disc->sector_size      = DISC_SECTOR_SIZE;
			return 0;
		case 0x04 :
		case 0x07 :
			/* Non MMC read only optical media */
			disc->devdrv_class     = UDF_DEVDRV_CLASS_DISC;
			disc->sequential       = 0;	/* 			*/
			disc->recordable       = 0;	/* assuming ro access	*/
			disc->blankable        = 0;	/* not applicable       */
			disc->rewritable       = 0;
			disc->packet           = 0;
			disc->blockingnr       = 1;
			disc->strict_overwrite = 0;
			disc->sector_size      = DISC_SECTOR_SIZE;
			return 0;
		default:
			fprintf(stderr, "Device type 0x%02x not suitable for mass storage\n", disc->scsi_device_type);
			return ENOENT;
	}

	/* get MMC profile */
	bzero(cmd, SCSI_CMD_LEN);
	cmd[ 0] = 0x46;				/* Get configuration */
	cmd[ 8] = 32;				/* just a small buffer size */
	cmd[ 9] = 0;				/* control */
	error = uscsi_command(SCSI_READCMD, disc->dev, cmd, 10, buf, 32, 30000, NULL);
	if (!error) {
		disc->mmc_profile = buf[7] | (buf[6] << 8);
	} else {
		disc->mmc_profile = 0;	/* mark unknown MMC profile */
	}
	UDF_VERBOSE_MAX(printf("Device has MMC profile 0x%02x\n", disc->mmc_profile));

	/* determine CD sector size */
	bzero(buf, 8);
	bzero(cmd, SCSI_CMD_LEN);
	cmd[0] = 0x25;		/* CD READ RECORDED CAPACITY */
	error = uscsi_command(SCSI_READCMD, disc->dev, cmd, 10, buf, 8, 30000, NULL);
	if (error) {
		fprintf(stderr, "Can't read CD recorded capacity; assuming sector size %d : %s", CD_SECTOR_SIZE, strerror(error));
		disc->sector_size = CD_SECTOR_SIZE;
	} else {
		disc->sector_size = buf[7] | (buf[6]<<8) | (buf[5]<<16) | (buf[4]<<24);
		/* packet size is recorded for each track seperately */
	}

	/*
	 * Read in features to determine device flags. First take some initial
	 * values.
	 */
	disc->sequential       = 0;
	disc->recordable       = 0;
	disc->erasable         = 0;
	disc->blankable        = 0;
	disc->formattable      = 0;
	disc->rewritable       = 0;
	disc->mrw              = 0;
	disc->packet           = 0;
	disc->strict_overwrite = 0;
	disc->blockingnr       = 1;	/* not relevant if non packet write */

	last_feature = feature = 0;
	do {
		bzero(cmd, SCSI_CMD_LEN);
		cmd[0] = 0x46;			/* Get configuration */
		cmd[1] = 0;			/* RT=0 -> all independent of current setting */
		cmd[2] = (last_feature) >> 8;	/* MSB feature number */
		cmd[3] = (last_feature) & 0xff;	/* LSB feature number */
		cmd[7] = (feat_tbl_len) >> 8;	/* MSB buffersize */
		cmd[8] = (feat_tbl_len) & 0xff;	/* LSB buffersize */
		cmd[9] = 0;			/* control */
		error = uscsi_command(SCSI_READCMD, disc->dev, cmd, 10, features, feat_tbl_len, 30000, NULL);
		if (error) {
			fprintf(stderr, "While reading feature table : %s\n", strerror(error));
			return EIO;
		}

		features_len      = features[3] | (features[2]<<8) | (features[1]<<16) | (features[0]<<24);
		disc->mmc_profile = features[7] | (features[6]<<8);

		pos = 8;
		while (pos < features_len) {
			fpos = &features[pos];

			feature     =  fpos[1] | (fpos[0] << 8);
			feature_ver = (fpos[2] >> 2) & 15;
			feature_cur = (fpos[2] & 1);
			feature_pers= (fpos[2] & 2);
			feature_len =  fpos[3];

			if (feature_cur == 1) {
				rpos = &fpos[4];
				switch (feature) {
					case 0x0010 :	/* random readable feature */
						disc->sector_size = rpos[3] | (rpos[2] << 8) | (rpos[1] << 16) | (rpos[0] << 24);
						disc->blockingnr  = rpos[5] | (rpos[4] << 8);
						/* RW error page */
						break;
					case 0x0020 :	/* random writable feature */
						disc->recordable = 1;
						disc->rewritable = 1;
						break;
					case 0x0021 :	/* incremental streaming write feature */
						disc->recordable = 1;
						disc->sequential = 1;
						disc->link_size  = rpos[7];
						if (rpos[6] & 1)
							disc->link_size = 0;
						break;
					case 0x0022 : 	/* (obsolete) erase support feature */
						disc->recordable = 1;
						disc->erasable = 1;
						break;
					case 0x0023 :	/* formatting media support feature */
						disc->recordable = 1;
						disc->formattable = 1;
						break;
					case 0x0024 :	/* hardware assisted defect management feature */
						/* XXX set mrw ? */
						break;
					case 0x0025 :	/* write once */
						disc->recordable = 1;
						break;
					case 0x0026 :	/* restricted overwrite feature */
						disc->recordable = 1;
						disc->rewritable = 1;
						disc->strict_overwrite = 1;
						break;
					case 0x0028 :	/* MRW formatted media support feature */
						disc->mrw = 1;
						break;
					case 0x002b :	/* read/write DVD+R formatted media */
						disc->sequential = 1;
						if (rpos[0] & 1) /* write support */
							disc->recordable = 1;
						break;
					case 0x002c :	/* regid restricted overwrite feature */
						disc->recordable = 1;
						disc->rewritable = 1;
						disc->strict_overwrite = 1;
						if (rpos[0] & 1) /* blank bit */
							disc->blankable = 1;
						break;
					case 0x002d :	/* track at once recording feature */
						disc->recordable = 1;
						disc->sequential = 1;
						break;
					case 0x002f :	/* DVD-R/-RW write feature */
						disc->recordable = 1;
						if (rpos[0] & 2) /* DVD-RW bit */
							disc->blankable = 1;
						break;
					case 0x0038 :	/* pseuro overwritable */
						break;
					default :
						/* ignore */
						break;
				}
			}

			last_feature = MAX(last_feature, feature);
			if (feature_len & 3) {
				UDF_VERBOSE(printf("Drive returned feature %d %swith bad length %d\n",
					feature, (feature_cur == 1?  "(current) ":""), feature_len));
				feature_len = (feature_len + 3) & ~3;
			}
			pos += 4 + feature_len;
		}
	} while (features_len >= 0xffff);

       /*
	* fixup DVD and CD-RW drives that are on crack.
	*/
        if (disc->mmc_profile == 0x0a) {
		/* some forget to mention strict overwrite when not sequential and forget the blocking size */
		if (!disc->sequential) {
			disc->strict_overwrite = 1;
			disc->blockingnr = 32;		/* fixed for CD-RW */
		}
		/* some say they are strict overwrite but also sequential */
		if (disc->strict_overwrite)
			disc->sequential = 0;
		/* some forget that if they are mrw they can't the be other */
		if (disc->mrw) {
			disc->sequential = 0;
			disc->strict_overwrite = 0;
		}
	}
	/* some think a CD-R is rewritable */
	if (disc->mmc_profile == 0x09) {
		disc->rewritable = 0;
	}

	/* derivatives */
	if (disc->blockingnr > 1)
		disc->packet = 1;

	switch (disc->mmc_profile) {
		case 0x01 :
		case 0x02 :
			/* SCSI discs class; treat like normal discs */
			disc->devdrv_class   = UDF_DEVDRV_CLASS_DISC;
			UDF_VERBOSE(printf("SCSI disc detected; treating like normal disc device\n"));
			return 0;
		case 0x03 :	/* Magneto Optical with sector erase */
		case 0x04 :	/* Magneto Optical write once */
		case 0x05 :	/* Advance Storage Magneto Optical */
			disc->devdrv_class   = UDF_DEVDRV_CLASS_MO;
			break;
		/* 0x00, 0x08, 0x09, 0x0a : different types of CD-ROM devices like CD-R/RW etc. */
		case 0x00 :
			disc->devdrv_class   = UDF_DEVDRV_CLASS_CD;	/* not allways clear */
			break;
		case 0x08 :	/* CD-ROM */
		case 0x09 :	/* CD-R */
		case 0x0a :	/* CD-RW */
			disc->devdrv_class   = UDF_DEVDRV_CLASS_CD;
			break;
		/* 0x10...0x14 DVD-ROM and DVD- devices */
		case 0x10 :	/* DVD-ROM */
		case 0x11 :	/* DVD-R   */
		case 0x12 :	/* DVD-RAM */
		case 0x13 :	/* DVD-RW (restricted overwrite) */
		case 0x14 :	/* DVD-RW (sequential)           */
		/* 0x1a..0x1b DVD+ devices */
		case 0x1a :	/* DVD+RW */
		case 0x1b :	/* DVD+R */
		case 0x2b :	/* DVD+R double layer */
			disc->devdrv_class   = UDF_DEVDRV_CLASS_DVD;
			break;
		case 0x40 :	/* BD-ROM */
		case 0x41 :	/* BD-R Sequential Recording (SRM) */
		case 0x42 :	/* BD-R Random Recording (RRM) */
		case 0x43 :	/* BD-RE */
			disc->devdrv_class   = UDF_DEVDRV_CLASS_BD;
			break;
		default :
			fprintf(stderr, "Not recognized MMC profile %d encountered, marking readonly\n", disc->mmc_profile);
			disc->devdrv_class   = UDF_DEVDRV_CLASS_CD;	/* go for the `dummy' */
			break;
	}

	/*
	 * ok, if we're on an CD-MRW or DVD+MRW, we ought to have switched
	 * automatically to DMA space. However, this drive at times just
	 * refuses with all messy things around.
	 */


	/* Select `Defect managed area' LBA space */
	if (!disc->mrw)
		return 0;

	DEBUG(printf("Setting DMA LBA space"));

	val_len = 6   + 8 + 2;			/* 2 for 4 byte alignment	*/
	rpos    = buf + 8;
	bzero(buf, val_len);

	rpos[ 0] = 0x03;			/* GAA/DMA space select		*/
	rpos[ 1] = 6;				/* page length         		*/
	rpos[ 3] = 0;				/* select GAA bit in bit 0	*/

	bzero(cmd, SCSI_CMD_LEN);
	cmd[0] = 0x55;			/* MODE SELECT (10)		*/
	cmd[1] = 16;			/* PF format			*/
	cmd[7] = val_len >> 8;		/* length of blob		*/
	cmd[8] = val_len & 0xff;
	cmd[9] = 0;			/* control			*/

	error = uscsi_command(SCSI_WRITECMD, disc->dev, cmd, 10, buf, val_len, 30000, NULL);
	if (error) {
		perror("While WRTITING parameter page 3");
		return error;
	}

	/* shutup gcc */
	feature_pers = feature_pers; feature_ver = feature_ver;

	return 0;
}

#if defined(__NetBSD__)
#include "partutil.h"
#include "partutil.c"

int udf_get_partition_info(struct udf_discinfo *disc) {
	struct disk_geom	geo;
	struct dkwedge_info	dkw;
	int error;

	/* get our disc info */
	error = getdiskinfo("name", disc->dev->fhandle, NULL, &geo, &dkw);
	if (error) {
		warn("retrieving disc info failed");
		return EIO;
	}
	disc->sector_size      = geo.dg_secsize;
	disc->session_start[0] = 0;
	disc->session_end  [0] = dkw.dkw_size -1;

	return 0;
}

#elif defined(__OpenBSD__)
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>

int udf_get_partition_info(struct udf_discinfo *disc) {
	struct disklabel  disklab;
	struct partition *dp;
	struct stat st;
	int partnr;

	/* read disklabel partition */
	if (ioctl(disc->dev->fhandle, DIOCGDINFO, &disklab) == -1) {
		/* failed to get disclabel! */
		perror("disklabel");
		return errno;
	}

        /* get disk partition it refers to */
	fstat(disc->dev->fhandle, &st);
	partnr = DISKPART(st.st_rdev);
	dp = &disklab.d_partitions[partnr];

	if (dp->p_size == 0) {
		perror("faulty disklabel partition returned, check label\n");
		return EIO;
	}

	disc->sector_size        = disklab.d_secsize;
	disc->session_start [0]  = 0;
	disc->session_end   [0]  = dp->p_size - 1;

	return 0;
}

#elif defined(__linux__)
#include <linux/fs.h>

int udf_get_partition_info(struct udf_discinfo *disc) {
	long p_size, p_size512;
	int  secsize;

	/* get device length and sector size */
	if (ioctl(disc->dev->fhandle, BLKSSZGET, &secsize) == -1) {
		perror("Can't read my sector size\n");
		return errno;
	}
	if (ioctl(disc->dev->fhandle, BLKGETSIZE, &p_size512) == -1) {
		perror("Can't read my partition size\n");
		return errno;
	}

	p_size = p_size512 * (secsize / 512);

	disc->sector_size        = secsize;
	disc->session_start [0]  = 0;
	disc->session_end   [0]  = p_size - 1;

	return 0;
}

#else

int udf_get_partition_info(struct udf_discinfo *disc) {
	perror(" UDF: no explicit support for disc devices yet for this operating system.\n");
	perror("Trying readonly access...\n");

	disc->recordable = disc->rewritable = 0;

	return 0;
}

#endif


/* 10000 is arbitrary */
/* TODO split up one day for its updating values unnessisarily */
#define res_len 10000
int udf_get_disc_info(struct udf_discinfo *disc) {
	scsicmd		 cmd;
	struct stat	 stat;
	uint32_t	 val_len;
	uint32_t	 first_track, last_track;
	uint32_t	 first_session, last_session;
	uint32_t	 next_writable_addr, packet_size;
	uint32_t	 cntrl, addr, tno, point, min, sec, frame, pmin, psec, pframe;
	uint32_t	 data_length, pos;
	uint8_t		 res[res_len];
	int		 first_track_last_session, last_track_last_session;
	int		 track, session, sector_size;
	int		 nwa_valid;
	off_t		 track_start, track_end, track_size, disc_size, free_blocks;
	int		 error;

	if (disc->devdrv_class == UDF_DEVDRV_CLASS_FILE) {
		sector_size = disc->alt_sector_size ? disc->alt_sector_size : disc->sector_size;
		fstat(disc->dev->fhandle, &stat);
		disc->link_size		 = 0;					/* no link lossage	*/
		disc->disc_state         = DISC_STATE_NOT_SERIAL;
		disc->last_session_state = SESSION_STATE_INCOMPLETE;
		disc->num_sessions       = 1;
		disc->session_start [0]  = 0;
		disc->session_end   [0]  = (stat.st_size    / sector_size);	/* inclusive */
		disc->next_writable [0]  = (stat.st_size    / sector_size) + 1;
		disc->packet_size   [0]  =  stat.st_blksize / sector_size;
		return 0;
	}

	if (disc->devdrv_class == UDF_DEVDRV_CLASS_DISC) {
		disc->link_size		 = 0;					/* no link lossage	*/
		disc->disc_state         = DISC_STATE_NOT_SERIAL;
		disc->last_session_state = SESSION_STATE_COMPLETE;
		disc->num_sessions       = 1;

		disc->session_start [0]  = 0;
		disc->session_end   [0]  = 0;

		error = udf_get_partition_info(disc);
		if (error)
			return error;

		fprintf(stderr, "UDF: warning... reading/writing on 'disc' device\n");
		return 0;
	}

	/* classes CD and DVD remain; we can be on a DVD/CD recordable or on a legacy CD-ROM */

	/* Get track information */
	val_len = 12;
	bzero(res, val_len);
	bzero(cmd, SCSI_CMD_LEN);

	cmd[0] = 0x51;				/* Read disc information */
	cmd[7] = val_len >> 8;			/* MSB allocation length */
	cmd[8] = val_len & 0xff;		/* LSB allocation length */
	cmd[9] = 0;				/* control */
	error = uscsi_command(SCSI_READCMD, disc->dev, cmd, 10, res, val_len, 30000, NULL);

	if (!error) {
		/* we are a MMC3+ device! */
		disc->disc_state		 =  res[2]       &  3;		/* just 'happends' to be the same as we use */
		disc->last_session_state	 = (res[2] >> 2) &  3;		/* ditto */
		disc->blankable			 =  res[2]       & 16;		/* blankable -> update possibility */
		disc->num_sessions		 =  res[4] | (res[ 9] << 8);
		first_track			 =  res[3];
		first_track_last_session	 =  res[5] | (res[10] << 8);	/* to build up the last 'session' */
		last_track_last_session  	 =  res[6] | (res[11] << 8);

		/* Initialise the sessions to be taken as overspanning tracks */
		for (session = 0; session < disc->num_sessions; session++) {
			disc->session_start[session] = INT_MAX;
			disc->session_end  [session] = 0;
		}

		for (track = first_track; track <= last_track_last_session; track++) {
			/* each track record is 36 bytes long */
			val_len = 36;
			bzero(res, val_len);

			bzero(cmd, SCSI_CMD_LEN);
			cmd[0] = 0x52;				/* Read track information */
			cmd[1] = 1;				/* indexed on track       */
			cmd[4] = track >> 8;			/* track number 0-0xff ?  */
			cmd[5] = track & 0xff;
			cmd[7] = val_len >> 8;			/* MSB length resultbuf	  */
			cmd[8] = val_len & 0xff;		/* LSB ,,		  */
			cmd[9] = 0;
			error = uscsi_command(SCSI_READCMD, disc->dev, cmd, 10, res, val_len, 30000, NULL);
			if (error) {
				perror("While reading track info");
				break;
			}
#if 0
			data_length		= res[1] | (res[0]  << 8);
			// track_number		= res[2] | (res[32] << 8);	  /* why? */
			session			= res[3] | (res[33] << 8);
			// is_track_mode	= res[5] & 15;
			// is_copy		= res[5] & 16;
			// is_damage		= res[5] & 32;
			// is_fixed_packet	= res[6] & 16;
			// is_packet_or_inc	= res[6] & 32;
			// is_blank		= res[6] & 64;
			// is_reserved		= res[6] & 128;
			// is_data_mode		= res[6] & 15;
			nwa_valid		= res[7] & 1;
			// lra_valid		= res[7] & 2;
#endif
			data_length		= res[1] | (res[0]  << 8);
			session			= res[3] | (res[33] << 8);
			nwa_valid		= res[7] & 1;

			track_start		= res[11] | (res[10]<<8) | (res[ 9]<<16) | (res[ 8]<<24);
			next_writable_addr	= res[15] | (res[14]<<8) | (res[13]<<16) | (res[12]<<24);
			free_blocks		= res[19] | (res[18]<<8) | (res[17]<<16) | (res[16]<<24);
			packet_size		= res[23] | (res[22]<<8) | (res[21]<<16) | (res[20]<<24);
			track_size		= res[27] | (res[26]<<8) | (res[25]<<16) | (res[24]<<24);
			/* last_recorded_addr	= res[32] | (res[30]<<8) | (res[29]<<16) | (res[28]<<24); */
			track_end = track_start + track_size;

			if (data_length <= 30) session &= 0xff;

			disc->session_start[session-1] = MIN(disc->session_start[session-1], track_start);
			disc->session_end  [session-1] = MAX(disc->session_end  [session-1], track_end);
			disc->free_blocks  [session-1] = free_blocks;
			disc->packet_size  [session-1] = packet_size;
			if (nwa_valid) disc->next_writable[session-1] = next_writable_addr;
		}
		if (disc->session_start[disc->num_sessions-1] == INT_MAX) {
			if (disc->disc_state == DISC_STATE_FULL) {
				if (disc->last_session_state == SESSION_STATE_COMPLETE) disc->num_sessions--;
			}
		}

		/* XXX
		 * initialise default link size to zero; only set DEFAULT
		 * link lossage for CD-R we are using `Burn Free' for writing
		 * but if it fails its good to know the penalty for recovery
		 * XXX
		 */
		disc->link_size = 0;
		if (disc->mmc_profile == 0x09) {
			disc->link_size = 7;
		}
		
		return 0;
	}

	/* Start with legacy CD-ROM .... trying to get as much info from it as possible */
	disc->sequential	 = 0;
	disc->recordable	 = 0;
	disc->blankable		 = 0;
	disc->packet		 = 0;
	disc->blockingnr	 = 32;
	disc->strict_overwrite	 = 0;
	disc->disc_state         = DISC_STATE_FULL;
	disc->last_session_state = SESSION_STATE_COMPLETE;
	disc->link_size          = 7;	/* DEFAULT link lossage for CDs */


	/* Read session range */
	val_len = 4;			/* only head */
	bzero(res, val_len);
	bzero(cmd, SCSI_CMD_LEN);
	cmd[0] = 0x43;			/* READ TOC/PMA/ATIP INFO	*/
	cmd[1] = 2;			/* no LBA's are defined         */
	cmd[2] = 2;			/* format 2 also this time	*/
	cmd[6] = 0;			/* obligatory zero		*/
	cmd[7] = val_len >> 8;
	cmd[8] = val_len & 0xff;
	cmd[9] = 0;
	error = uscsi_command(SCSI_READCMD, disc->dev, cmd, 10, res, val_len, 30000, NULL);
	if (error) {
		perror("TOC reading of sessions failed");
		return error;
	}
	first_session = read_cd_hex2(res[2]);
	last_session  = read_cd_hex2(res[3]);

	disc->num_sessions = last_session - first_session + 1;

	/* Initialise the sessions to be taken as overspanning tracks */
	for(session = 0; session <= disc->num_sessions; session++) {
		disc->session_start[session] = INT_MAX;
		disc->session_end  [session] = 0;
	}

	/* calculate how big the result buffer ought to be to get the whole TOC */
	/* NOTE: don't count the 2 length bytes */
	val_len = res[1] + (res[0] << 8);

	/* fix length for ATAPI drives */
	if (val_len & 1)
		val_len++;

	assert(val_len < res_len);

	/* Read the complete TOC and extract information */
	bzero(res, val_len);
	bzero(cmd, SCSI_CMD_LEN);
	cmd[0] = 0x43;			/* READ TOC/PMA/ATIP INFO	*/
	cmd[1] = 2;			/* LBA's are not defined 	*/
	cmd[2] = 2;			/* format 2; full TOC		*/
	cmd[6] = first_session;		/* start at first session	*/
	cmd[7] = val_len >> 8;
	cmd[8] = val_len & 0xff;
	cmd[9] = 0;
	error = uscsi_command(SCSI_READCMD, disc->dev, cmd, 10, res, val_len, 30000, NULL);
	if (error) {
		perror("TOC reading of full TOC failed");
		return error;
	}

	data_length =res[1] | (res[0] << 8);
	if (data_length != val_len) {
		fprintf(stderr, "Warning: device didn't return all data requested when reading full TOC\nn");
		fprintf(stderr, "\treturned %d bytes instead of %d\n", data_length, val_len);
	}
	pos = 4;
	track_start = INT_MAX; track_end = 0;

	while (pos < data_length + 2) {
		session = read_cd_bcd(res[pos+ 0]);
		cntrl   = res[pos+1] & 15;
		addr    = res[pos+1] >> 4;
		tno     = read_cd_bcd(res[pos+ 2]);
		point   = read_cd_bcd(res[pos+ 3]);
		min     = read_cd_bcd(res[pos+ 4]);
		sec     = read_cd_bcd(res[pos+ 5]);
		frame   = read_cd_bcd(res[pos+ 6]);
		pmin    = read_cd_bcd(res[pos+ 8]);
		psec    = read_cd_bcd(res[pos+ 9]);
		pframe  = read_cd_bcd(res[pos+10]);

		/* extract information; explicit writeout. See SCSI docs */
		if (tno == 0 && session && addr == 1) {
			switch (point) {
				case 0xa0 : first_track = pmin; break;
				case 0xa1 : last_track  = pmin; break;
				case 0xa2 :
					track_end   = cd_msf2lba(0, pmin, psec, pframe);
					disc->session_end  [session-1] = MAX(disc->session_end  [session-1], track_end);
					break;
				default   :
					track_start = cd_msf2lba(0, pmin, psec, pframe);
					disc->session_start[session-1] = MIN(disc->session_start[session-1], track_start);
					break;
			}
		}
		if (tno == 0 && session && addr == 5) {
			if (point == 0xb0) {
				next_writable_addr = cd_msf2lba(0, min, sec, frame);
				disc_size = cd_msf2lba(0, pmin, psec, pframe);
				/* TODO use nwa & size */
				printf("UDF: ignoring B0 Q channel : next writable address; pre MMC3 device; fix me\n");
			}

		}
		pos += 11;
	}

	/* Last session information is notoriously flawed in TOC format so update it */
	bzero(res, 8);
	bzero(cmd, SCSI_CMD_LEN);
	cmd[0] = 0x25;		/* CD READ RECORDED CAPACITY */
	error = uscsi_command(SCSI_READCMD, disc->dev, cmd, 10, res, 8, 30000, NULL);
	if (error) {
		fprintf(stderr, "Can't read CD recorded capacity, last session end might not be OK : %s\n", strerror(error));
		return 0;
	}

	session = disc->num_sessions-1;
	disc->session_end[session] = res[3] | (res[2]<<8) | (res[1]<<16) | (res[0]<<24);

	/* shut up gcc */
	disc_size = disc_size; first_track_last_session = first_track_last_session;
	cntrl = cntrl; last_track = last_track;

	return 0;
}
#undef res_len


int udf_open_disc(char *dev_name, int discop_flags, struct udf_discinfo **discptr) {
	struct udf_discinfo *disc;

	if (!discptr) return EINVAL;
	*discptr = NULL;

	/* determine what kind of file/device we are dealing with */
	disc = calloc(1, sizeof(struct udf_discinfo));
	if (!disc) return ENOMEM;

	disc->dev = calloc(1, sizeof(struct uscsi_dev));
	if (!disc->dev) {
		free(disc);
		return ENOMEM;
	}

	/* fill in the name */
	disc->dev->dev_name = strdup(dev_name);

	if (uscsi_open(disc->dev) != 0) {
		perror("Failure to open device or file");
		free(disc->dev);
		free(disc);
		return ENODEV;
	}

	/* determine disc type */
	if (udf_discinfo_get_disc_type(disc)) {
		perror("Error during classification of disc; skipping disc\n");
		uscsi_close(disc->dev);
		free(disc->dev);
		free(disc);
		return ENODEV;
	}

	/* get disc info */
	if (udf_get_disc_info(disc)) {
		fprintf(stderr, "Can't get disc info");
		uscsi_close(disc->dev);
		free(disc->dev);
		free(disc);
		return ENODEV;
	}

	/* process discop_flags */
	if (discop_flags & UDF_DISCOP_BSWAP)
		disc->bswap_sectors = 1;

	/* return the pointer */
	*discptr = disc;

	/* set recording parameters */
	udf_discinfo_set_recording_parameters(disc, 0);	/* no testwrite */

	return 0;
}


int udf_close_disc(struct udf_discinfo *disc) {
	if (!disc) return 0;

/*	udf_stop_disc_thread(disc); */
	uscsi_close(disc->dev);

	printf("Disc access statistics\n");
	printf("\tsector reads   %8"PRIu64"  (%"PRIu64" Kbyte)\n", disc->sectors_read,    ((uint64_t) (disc->sectors_read)    * disc->sector_size) / 1024);
	printf("\tsector written %8"PRIu64"  (%"PRIu64" Kbyte)\n", disc->sectors_written, ((uint64_t) (disc->sectors_written) * disc->sector_size) / 1024);
	printf("\tswitches       %8d\n", disc->switchings);

	return 0;
}



int udf_discinfo_alter_perception(struct udf_discinfo *disc, uint32_t sec_size, uint32_t num_sectors) {
	struct stat stat;

	assert(disc);
	if ((disc->devdrv_class & UDF_DEVDRV_CLASS) != UDF_DEVDRV_CLASS_FILE) {
		return EINVAL;
	}

	fstat(disc->dev->fhandle, &stat);
	if (sec_size == 0)    sec_size    = disc->sector_size;
	if (num_sectors == 0) num_sectors = stat.st_size / sec_size;

	if (((sec_size % 512) != 0) || (sec_size == 0)) {
		fprintf(stderr, "Size of blocks need to be a multiple of 512\n");
		return EINVAL;
	}

	if (stat.st_size / sec_size >= (uint32_t) -1)
	{
		fprintf(stderr, "Disc needs to many logical sectors, please increase blocksize\n");
		return EINVAL;
	}

	if (num_sectors < 300) {
		fprintf(stderr, "Disc size too small to put an UDF filingsystem on\n");
		return EINVAL;
	}

	if (stat.st_size != (off_t) sec_size * num_sectors) {
		fprintf(stderr, "Size of image file is not equal to specified size parameters\n");
		return EINVAL;
	}

	disc->sequential         = 0;					/* full r/w		*/
	disc->recordable         = 1;					/* assuming rw access	*/
	disc->rewritable         = 1;
	disc->sector_size	 = sec_size;
	disc->alt_sector_size	 = sec_size;				/* altered value	*/
	disc->link_size		 = 0;					/* no link lossage	*/
	disc->disc_state         = DISC_STATE_NOT_SERIAL;
	disc->last_session_state = SESSION_STATE_INCOMPLETE;
	disc->num_sessions       = 1;
	disc->session_start [0]  = 0;
	disc->session_end   [0]  = num_sectors;
	disc->next_writable [0]  = num_sectors + 1;
	disc->packet_size   [0]  = stat.st_blksize / sec_size;		/* best blocking size	*/

	return 0;
}


/******************************************************************************************
 *
 * Sector readers and writers
 *
 ******************************************************************************************/


/* read an extent of sectors in the `result' buffer */
int udf_read_physical_sectors(struct udf_discinfo *disc, off_t sector, uint32_t num_sectors, char *what, uint8_t *result) {
	struct uscsi_sense sense;
	scsicmd		cmd;
	int		size, size_read, chunk;
	uint32_t	session, skipped, sector_size;
	int		pos, lb, hb, error;

	/* protect us */
	if (((long) result) & 3) {
		printf("Unaligned read of sector : possible panic() on some systems avoided\n");
		return EIO;
	}

	sector_size = disc->sector_size;

	/* read one UDF sector at a physical address specified in UDF_SECTOR size units */
	size = num_sectors * disc->sector_size;
	size_read = 0;
	bzero(result, size);	/* just in case */		/* has to go? */

	assert(sector_size);
	assert(num_sectors <= 0xffff);

	/* statistics and cache control */
	if (disc->am_writing) {
		disc->switchings++;
		/* XXX how about pseudo-overwrite? is this nessisary then too ? XXX */
		if (disc->sequential) {
			/*
			 * we need to synchronise the write caches before we
			 * are allowed to read from the disc again.
			 */
			error = udf_discinfo_synchronise_caches(disc);
			while (error) {
				printf("udf_discinfo: failed to sync caches, retrying\n");
				error = udf_discinfo_synchronise_caches(disc);
			}
			/* we need to update our NWA's after the sync on sequentials */
			udf_get_disc_info(disc);
		}
		/* mark reading access only */
		disc->am_writing = 0;
	}

	error = 0;
	DEBUG(printf("\r%08d : %s; read %d bytes\n", (int) sector, what, size));
	while (num_sectors) {
		switch (disc->devdrv_class & UDF_DEVDRV_CLASS) {
			case UDF_DEVDRV_CLASS_CD  :
			case UDF_DEVDRV_CLASS_DVD :
				/*
				 * Use a SCSI command to read it so we can go past the last session;
				 * allthough READ (12) is available, some older CD-ROM
				 * devices only want to do READ (10).
				 */

				/* limited by MAXPHYS, taking 64kb as limitation */
				chunk     = MIN(64*1024 / sector_size, num_sectors);
				size_read = chunk * sector_size;		/* definition */
				skipped = session = 0;

				bzero(cmd, SCSI_CMD_LEN);
				cmd[0] = 0x28;				/* READ (10) command		*/
				cmd[1] = 0;				/* normal access		*/
				cmd[2] = (sector >> 24) & 0xff;
				cmd[3] = (sector >> 16) & 0xff;
				cmd[4] = (sector >>  8) & 0xff;
				cmd[5] = (sector      ) & 0xff;
				cmd[6] = 0;				/* reserved */
				cmd[7] = (chunk >>  8) & 0xff;		/* MSB transfer */
				cmd[8] = (chunk      ) & 0xff;		/* number of logical block(s) */
				cmd[9] = 0;				/* control */
				do {
					error = uscsi_command(SCSI_READCMD, disc->dev, cmd, 10, result, size_read - skipped, 30000, &sense);
					/* TODO: if busy, ask drive how long it'll take to be available again and wait */
					if (sense.asc == 4)
						usleep(5000);
				} while (sense.asc == 4);

				if (error) return ENOENT;
				break;
			default :
				/* XXX first and only reference to {p}read() XXX */
				if (sector>=0)
					size_read = pread(disc->dev->fhandle, result, (uint64_t) num_sectors * sector_size, sector * sector_size);
				break;
		}
		/* statistics */
		disc->sectors_read += size_read / sector_size;

		/* swap space if requested */
		if (disc->bswap_sectors) {
			for (pos = 0; pos < size_read;) {
				lb = result[pos  ];
				hb = result[pos+1];
				result[pos  ] = hb;
				result[pos+1] = lb;
				pos += 2;
			}
		}

		/* advance */
		num_sectors -= size_read / sector_size;
		sector      += size_read / sector_size;
		result      += size_read;
		if (size_read <= 0) {
			UDF_VERBOSE_MAX(
				if (what) {
					printf("Can't read sectors %d+%d for %s\n", (int) sector, num_sectors, what);
				}
			);
			if (size_read == 0) return ENOENT;
			return error;
		}
	}

	return 0; /* flag ok */
}


/* write an extent of sectors to disc */
int udf_write_physical_sectors(struct udf_discinfo *disc, off_t sector, uint32_t num_sectors, char *what, uint8_t *source) {
	struct uscsi_sense sense;
	scsicmd    cmd;
	int        trans_length, size, size_written, chunk;
	uint32_t   sector_size;
	uint8_t   *buffer;
	int        lb, hb, pos, error;

/*	if (!disc->udf_recording) return ENODEV; */

	/* protect us */
	if (((long) source) & 3) {
		printf("Unaligned write of sector : possible panic() on some systems avoided\n");
		return EIO;
	}

	sector_size = disc->sector_size;

	/* XXX clean up XXX */
	assert(sector_size);
	assert(num_sectors <= 0xffff);					/* compatible with WRITE (10)? */

	DEBUG(printf("\r%08d : %s ;WRITE %d bytes\n", (int) sector, what, num_sectors * sector_size));

	/* swap space if requested */
	buffer = source;
	if (disc->bswap_sectors) {
		size = num_sectors * sector_size;
		buffer = malloc(num_sectors * sector_size);
		for (pos = 0; pos < size;) {
			lb = source[pos  ];
			hb = source[pos+1];
			buffer[pos  ] = hb;
			buffer[pos+1] = lb;
			pos += 2;
		}
	}

	error = 0;
	while (num_sectors) {
                size_written = 0;

		switch (disc->devdrv_class & UDF_DEVDRV_CLASS) {
			case UDF_DEVDRV_CLASS_CD  :
			case UDF_DEVDRV_CLASS_DVD :
				/*
				 * Use WRITE (12) command to write to the disc; we
				 * might have to downgrade later to using WRITE (10)
				 * on older discs though :-/
				 */

				/* limited by MAXPHYS, taking 64kb as limitation */
				chunk        = MIN(64*1024 / sector_size, num_sectors);		/* in sectors */
				trans_length = chunk;						/* in sectors */
				bzero(cmd, SCSI_CMD_LEN);
				cmd[ 0] = 0xAA;				/* WRITE (12)	*/
				cmd[ 1] = 0;				/* no force unit access */
				cmd[ 2] = (sector       >> 24) & 0xff;
				cmd[ 3] = (sector       >> 16) & 0xff;
				cmd[ 4] = (sector       >>  8) & 0xff;
				cmd[ 5] = (sector            ) & 0xff;
				cmd[ 6] = (trans_length >> 24) & 0xff;	/* MSB */
				cmd[ 7] = (trans_length >> 16) & 0xff;
				cmd[ 8] = (trans_length >>  8) & 0xff;
				cmd[ 9] = (trans_length      ) & 0xff;	/* LSB  */
				cmd[10] = 0;				/* no streaming */
				cmd[11] = 0;				/* control */
				do {
					error = uscsi_command(SCSI_WRITECMD, disc->dev, cmd, 12, buffer, trans_length * sector_size, 30000, &sense);
					/* TODO: if busy, ask drive how long it'll take to be available again and wait */
					if (sense.asc == 4)
						usleep(5000);
				} while (sense.asc == 4);

		                /* write one UDF sector at a physical address specified in UDF_SECTOR size units */
                		size = trans_length * sector_size;
				size_written = error ? 0 : size;
				break;
			default :
				/* XXX first and only reference to {p}write() XXX */
				DEBUG(printf("udf_discop: pwrite %"PRIu64" + %d\n", (uint64_t) sector * sector_size, (int) num_sectors * sector_size));
		                /* write one UDF sector at a physical address specified in UDF_SECTOR size units */
                		size = num_sectors * sector_size;
				size_written = pwrite(disc->dev->fhandle, buffer, (uint64_t) num_sectors * sector_size, sector * sector_size);
				if (size_written < 0)
					size_written = 0;
				break;
		}
	
		/* free our copy if we created one */
		if (buffer != source)
			free(buffer);
	
		/* statistics */
		disc->sectors_written += size_written / sector_size;
		if (!disc->am_writing) disc->switchings++;
		disc->am_writing = 1;

		num_sectors -= size_written / sector_size;
		sector      += size_written / sector_size;
		buffer      += size_written;
		if ((size_written < size) || error) {
			DEBUG(if (error) printf("Writing %s at sectors %d+%d failed\n", what, (int) sector, num_sectors));
			return EIO;
		}
	}

	return 0;
}


/* End of udf_discinfo.c */

