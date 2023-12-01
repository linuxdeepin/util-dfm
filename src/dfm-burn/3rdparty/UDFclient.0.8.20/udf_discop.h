/* $NetBSD$ */

/*
 * File "udf_discop.h" is part of the UDFclient toolkit.
 * File $Id: udf_discop.h,v 1.30 2011/02/01 20:43:41 reinoud Exp $ $Name:  $
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


#ifndef _UDF_DISCOP_H_
#define _UDF_DISCOP_H_


#include <pthread.h>
#include "defs.h"
#include "uscsilib.h"
#include "queue.h"

/* setup constants */
#define UDF_DISCOP_BSWAP	   1

/* Implementation constants */
#define MAX_SESSIONS		 100
#define CD_SECTOR_SIZE		2048
#define DISC_SECTOR_SIZE	 512


/* State and driver constants */
#define DISC_STATE_EMPTY		0	/* nothing on the disc	 */
#define DISC_STATE_INCOMPLETE		1	/* can be appended	 */
#define DISC_STATE_FULL			2	/* no appending possible */
#define DISC_STATE_NOT_SERIAL		3	/* not serial		 */

#define SESSION_STATE_EMPTY		0	/* still empty		 */
#define SESSION_STATE_INCOMPLETE	1	/* session is open	 */
#define SESSION_STATE_CLOSED		2	/* we need a new session */
#define SESSION_STATE_COMPLETE		3	/* no appending possible */

#define UDF_DEVDRV_CLASS		0xFF
#define UDF_DEVDRV_CLASS_FILE		0x00
#define UDF_DEVDRV_CLASS_DISC		0x01
#define UDF_DEVDRV_CLASS_CD		0x02
#define UDF_DEVDRV_CLASS_DVD		0x04
#define UDF_DEVDRV_CLASS_MO		0x08
#define UDF_DEVDRV_CLASS_BD		0x10


/* will go? */
struct udf_session;


/*
 * The complete disc, all cd sessions and how to access the drive
 */
struct udf_discinfo {
	struct uscsi_dev *dev;			/* uscsi device associated			*/

	int	 scsi_device_type;		/* in case of SCSI */
	int	 mmc_profile;			/* in case of SCSI */
	int	 devdrv_class;

	int	 sequential;			/* media is sequential writable only		*/
	int	 recordable;			/* media is record-able; i.e. not static	*/
	int	 erasable;			/* drive can erase sectors 			*/
	int	 blankable;			/* media can be blanked by the drive		*/
	int	 formattable;			/* media can be formatted by the drive		*/
	int	 rewritable;			/* media can be rewritten			*/
	int	 mrw;				/* media is identified as being MRW formatted	*/
	int	 packet;			/* media is using packet recording		*/
	int	 strict_overwrite;		/* media can only be written a packet at a time	*/
	int	 blockingnr;			/* blocking size for ECC/modification		*/

	int	 sector_size;			/* sector size in bytes				*/
	int	 alt_sector_size;		/* if non zero, override normal sectorsize	*/
	int	 link_size;			/* link size lossage when having underruns	*/
	int	 disc_state;			/* empty - incomplete - full   - not_serial	*/
	int	 last_session_state;		/* empty - incomplete - closed - complete	*/

	/* flags that alter udf_discop's behaviour */
	int	 udf_recording;			/* flags if a recording is initialised		*/
	int	 bswap_sectors;			/* swap all bytes in a sector			*/

	/* statistics */
	int		 am_writing;
	uint64_t	 sectors_read;
	uint64_t	 sectors_written;
	uint32_t	 switchings;

	/* sessions */
	int	 num_sessions;
	int	 num_udf_sessions;
	int	 session_is_UDF[MAX_SESSIONS];
	int	 session_quirks[MAX_SESSIONS];

	off_t	 session_start [MAX_SESSIONS];
	off_t	 session_end   [MAX_SESSIONS];

	uint32_t next_writable [MAX_SESSIONS];
	uint32_t free_blocks   [MAX_SESSIONS];
	uint32_t packet_size   [MAX_SESSIONS];

	STAILQ_HEAD(udf_sessions, udf_session) sessions;

	SLIST_ENTRY(udf_discinfo) next_disc;
};


/* exported functions */
extern int  udf_open_disc(char *dev_name, int discop_flags, struct udf_discinfo **discptr);
extern int  udf_close_disc(struct udf_discinfo *disc);

extern int  udf_discinfo_get_type(struct udf_discinfo *disc);
extern int  udf_discinfo_check_disc_ready(struct udf_discinfo *disc);
extern int  udf_discinfo_is_cd_or_dvd(struct udf_discinfo *disc);
extern int  udf_discinfo_set_recording_parameters(struct udf_discinfo *discinfo, int testwriting);
extern int  udf_discinfo_alter_perception(struct udf_discinfo *disc, uint32_t sec_size, uint32_t num_sectors);
extern int  udf_discinfo_finish_writing(struct udf_discinfo *discinfo);
extern int  udf_discinfo_reserve_track_in_logic_units(struct udf_discinfo *discinfo, uint32_t logic_units);
extern int  udf_discinfo_close_track(struct udf_discinfo *discinfo, uint16_t trackno);
extern int  udf_discinfo_repair_track(struct udf_discinfo *discinfo, uint16_t trackno);

extern int  udf_read_physical_sectors(struct udf_discinfo *disc, off_t sector, uint32_t num_sectors, char *what, uint8_t *result);
extern int  udf_write_physical_sectors(struct udf_discinfo *disc, off_t sector, uint32_t num_sectors, char *what, uint8_t *source);

#endif	/* _UDF_DISCOP_H_ */

