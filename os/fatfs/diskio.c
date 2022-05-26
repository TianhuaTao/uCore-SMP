/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <sifive/platform.h>
#include <spi/spi.h>
#include <driver/sd.h>
#include <ucore/defs.h> // for virtio function declarations
#include <utils/log.h>
#include <utils/assert.h>
#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include <fs/buf.h>


/* Definitions of physical drive number for each drive */
#define DEV_MMC		0

#ifndef QEMU
static unsigned int peripheral_input_khz = 500000; // copy from fsbl\main.c:173
static spi_ctrl* spictrl = (spi_ctrl*) SPI2_CTRL_ADDR;
PARTITION VolToPart[FF_VOLUMES] = {
        {0, 2},     /* "0:" ==> 1st partition on the pd#0 */
        {0, 1},     /* "1:" ==> 2nd partition on the pd#0 */
};
#else
#define ROOTDEV 1
#endif

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	return pdrv == DEV_MMC ? RES_OK : RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	int result;

	if (pdrv != DEV_MMC){
        return RES_PARERR;
    }

#ifndef QEMU
    result = sd_init(spictrl, peripheral_input_khz, 0);
#else
    virtio_disk_init();
    result = 0;
#endif
    return result == 0 ? RES_OK : RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	int result;

    if (pdrv != DEV_MMC){
        return RES_PARERR;
    }

//    KERNEL_ASSERT(sector <= (uint64_t) UINT32_MAX, "sector must be 32 bits");
//    printf("read sector = %d begin, count = %d\n", sector, count);
#ifndef QEMU
    result = sd_read_blocks(spictrl, buff, sector, count);
#else
    for(int i = 0;i<count;i++) {
        struct buf *b;
        b = acquire_buf_and_read(ROOTDEV,sector + i * 512);
        memmove((void*)((size_t)buff+i * 512), b->data, 512);
        release_buf(b);
    }
    result = 0;
#endif
//    printf("read sector end, result = %d\n", result);

    return result == 0 ? RES_OK : RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	int result;

    if (pdrv != DEV_MMC){
        return RES_PARERR;
    }

//    KERNEL_ASSERT(sector <= (uint64_t) UINT32_MAX, "sector must be 32 bits");
//    printf("write sector = %d begin, count = %d\n", sector, count);
#ifndef QEMU
    result = sd_write_blocks(spictrl, buff, sector, count);
#else
    for(int i = 0;i<count;i++) {
        struct buf *b;
        b = acquire_buf_and_read(ROOTDEV,sector + i * 512);
        memmove(b->data, (void*)((size_t)buff+i * 512), 512);
        write_buf_to_disk(b);
        release_buf(b);
    }
    result = 0;
#endif
//    printf("write sector end, result = %d\n", result);
    return result == 0 ? RES_OK : RES_ERROR;
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
    DRESULT res;

    if (pdrv != DEV_MMC){
        return RES_PARERR;
    }

    switch (cmd) {
    case CTRL_SYNC:
        res = RES_OK;
        break;
    case GET_SECTOR_COUNT:
#ifndef QEMU
        *(DWORD*)buff = 13631488; //read from disk tool
#else
        *(DWORD*)buff = 1048576; //512M
#endif
        res = RES_OK;
        break;
    case GET_SECTOR_SIZE:
        *(WORD*)buff = 512;
        res = RES_OK;
        break;
    case GET_BLOCK_SIZE:
        *(WORD*)buff = 4096;
        res = RES_OK;
        break;
//    case CTRL_TRIM:
//        result = sd_trim(spictrl, (LBA_t)buff);
//        res = result == 0 ? RES_OK : RES_ERROR;
//        break;
    default:
        res = RES_PARERR;
        break;
    }

    return res;
}

