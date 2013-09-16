/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include <Windows.h>
#include <time.h>


HANDLE hFile = NULL;

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber (0..) */
)
{
	DWORD dwTemp;

	if (hFile != NULL)
		CloseHandle(hFile);

	hFile = CreateFileW(L"Z:\\fat.dat", 
		GENERIC_WRITE|GENERIC_READ, 
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		0,
		NULL);

	DeviceIoControl(hFile,
		FSCTL_SET_SPARSE, 
		NULL,
		0,
		NULL,
		0,
		&dwTemp,
		NULL);


	return 0;
}



/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0..) */
)
{
	return 0;
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..128) */
)
{
	DWORD result;
	LARGE_INTEGER li;
	li.QuadPart = (LONGLONG)sector << 9;

	SetFilePointerEx(hFile, li, NULL, SEEK_SET);
	ReadFile(hFile, buff, (DWORD)count<<9, &result, NULL);

	return RES_OK;
	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..128) */
)
{
	DWORD result;
	LARGE_INTEGER li;
	BOOL b;
	li.QuadPart = (LONGLONG)sector << 9;

	SetFilePointerEx(hFile, li, NULL, SEEK_SET);
	b = WriteFile(hFile, buff, (DWORD)count<<9, &result, NULL);

	
	return RES_OK;
	return RES_PARERR;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	switch(cmd)
	{
	case GET_BLOCK_SIZE:
		if (buff)
			*(DWORD*)buff = 512;
		break;
	case GET_SECTOR_COUNT:
		if (buff)
			*(DWORD*)buff = 2048*2560;
		break;
	case CTRL_SYNC:
		break;
	default:
		return RES_PARERR;
	}
	
	return RES_OK;
}
#endif


DWORD get_fattime(void)
{
	return time(NULL);
}