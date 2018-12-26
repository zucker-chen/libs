/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */

/* Definitions of physical drive number for each drive */
#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */

#define DEV_NAME    "/dev/sdd"//"/dev/mmcblk0p1""./sd.bin" //
static int dev_fd = 0;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	return (dev_fd == 0) ? STA_NOINIT: RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
    printf("%s(%d)\n", __FUNCTION__, __LINE__);
	DRESULT stat = STA_NOINIT;

    if (dev_fd > 0) {
        return RES_OK;
    }
    
    dev_fd = open(DEV_NAME, O_RDWR);
    if (dev_fd < 0) {
        printf("%s(%d): open(%s) error: %s\n", __FUNCTION__, __LINE__, DEV_NAME, strerror(errno));
        stat = STA_NOINIT;
    } else {
        stat = RES_OK;
    }
        
    //struct stat st;
    //fstat(dev_fd, &st);
    //printf("%s(%d) st.st_size = %ld, st.st_blksize=%ld, st.st_blocks = %ld\n", __FUNCTION__, __LINE__, st.st_size, st.st_blksize, st.st_blocks);
        
	return stat;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
    //printf("%s(%d)\n", __FUNCTION__, __LINE__);
	DRESULT res = RES_PARERR;
    BYTE i = 0;

        lseek(dev_fd, (long)(sector*FF_MAX_SS), SEEK_SET);
        for (i = 0; i < count; i++) {
            res = read(dev_fd, buff + (i*FF_MAX_SS), FF_MAX_SS);
            if (res > 0 && res < FF_MAX_SS) {
                if ( read(dev_fd, buff + (i*FF_MAX_SS) + res, FF_MAX_SS - res) != FF_MAX_SS - res) {
                    res = RES_PARERR;
                }
            } else if (res < 0) {
                res = RES_PARERR;
            } else {
                res = RES_OK;
        }
        }

	return res;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
    //printf("%s(%d) sector = %d, count = %d\n", __FUNCTION__, __LINE__, sector, count);
	DRESULT res = RES_PARERR;
    BYTE i = 0;

    lseek(dev_fd, (long)(sector*FF_MAX_SS), SEEK_SET);
    for (i = 0; i < count; i++) {
        res = write(dev_fd, buff + (i*FF_MAX_SS), FF_MAX_SS);
        if (res > 0 && res < FF_MAX_SS) {
            if (write(dev_fd, buff + (i*FF_MAX_SS) + res, FF_MAX_SS - res) != FF_MAX_SS - res) {
                res = RES_PARERR;
            }
        } else if (res < 0) {
            res = RES_PARERR;
        } else {
            res = RES_OK;
        }
    }

	return res;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
    //printf("%s(%d)\n", __FUNCTION__, __LINE__);
	DRESULT res = RES_PARERR;

	switch (cmd) {
        case CTRL_SYNC:
            printf("%s(%d) CTRL_SYNC\n", __FUNCTION__, __LINE__);
            syncfs(dev_fd);
            res = RES_OK;
            break;

        case GET_SECTOR_COUNT:  /* Get number of sectors on the drive */
            printf("%s(%d) GET_SECTOR_COUNT\n", __FUNCTION__, __LINE__);
            *(DWORD*)buff = 268435456;  /* f_fdisk need it */
            res = RES_OK;
            break;

        case GET_SECTOR_SIZE:   /* Get size of sector for generic read/write */
            printf("%s(%d) GET_SECTOR_SIZE\n", __FUNCTION__, __LINE__);
            *(WORD*)buff = 512;
            res = RES_OK;
            break;

        case GET_BLOCK_SIZE:    /* Get internal block size in unit of sector */
            printf("%s(%d) GET_BLOCK_SIZE\n", __FUNCTION__, __LINE__);
            *(DWORD*)buff = 1;
            res = RES_OK;
            break;
	}
        
        
	return res;
}

/*-----------------------------------------------------------------------*/
/* FAT TIME                                                              */
/* bit31:25 year (0~127)(start 1980)                                     */
/* bit24:21 month (1~12)                                                 */
/* bit20:16 day (1~31)                                                   */
/* bit15:11 hour (0~23)                                                  */
/* bit10:5 min (0~59)                                                    */
/* bit4:0 sec (0~29)                                                     */
/*-----------------------------------------------------------------------*/
DWORD get_fattime (void)
{
    printf("%s(%d)\n", __FUNCTION__, __LINE__);
    DWORD result;
    time_t t;
    struct tm lt;
    
    time(&t);
    localtime_r(&t, &lt);
    result = ((DWORD)(lt.tm_year+1900-1980) << 25 | (DWORD)(lt.tm_mon+1) << 21 | (DWORD)(lt.tm_mday) << 16 | (DWORD)(lt.tm_hour) << 11 | (DWORD)(lt.tm_min) << 5 | (DWORD)lt.tm_hour);
    
    return result;
}

