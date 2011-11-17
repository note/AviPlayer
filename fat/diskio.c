/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */
/* Note that Tiny-FatFs supports only single drive and always            */
/* accesses drive number 0.                                              */


//user's include:
#include "common.h"
#include "sd.h"

static DSTATUS diskStatus = STA_NOINIT;

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */

DSTATUS disk_initialize (
	BYTE drv				/* Physical drive nmuber (0..) */
)
{
	if(sdInit()==OK)
	{
		debug_msg("disk_initialize: sdCard initialized OK");
		diskStatus = 0;
	}
	else
	{
		debug_msg("disk_initialize: sdCard initialization FAILED");
		diskStatus = STA_NOINIT;
	}
	return(diskStatus);
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{
	return diskStatus;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{
	sdReadBlocks(buff,sector,count);
	return(0);
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..255) */
)
{
	sdWriteBlocks((void*)buff,sector,count);
	return(0);
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	static uint32_t maxLba;
	static uint16_t blockSize;
	switch(ctrl)
	{
		case GET_SECTOR_COUNT:
			sdGetSizeInfo(&maxLba,&blockSize);
			buff = (void*)&maxLba;
			return 0;
		case GET_BLOCK_SIZE:
			sdGetSizeInfo(&maxLba,&blockSize);
			buff = (void*)&blockSize;
			return 0;
		case CTRL_SYNC:
			return 0;
		default:
			return STA_NOINIT;
	}
}

