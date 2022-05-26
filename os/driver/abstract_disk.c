// #define USE_RAMDISK
// #define USE_MMC
#include <ucore/ucore.h>
#include "ramdisk.h"

#include <fs/buf.h>
void init_abstract_disk(){
    #ifdef USE_RAMDISK
        init_ram_disk();
    #elif defined(USE_MMC)
	    sdcard_init();
    #else
        virtio_disk_init();
    #endif
}

void abstract_disk_rw(struct buf *b, int write){
    #ifdef USE_RAMDISK
        ram_disk_rw(b, write);
    #elif defined(USE_MMC)
	    if (write){
	        sdcard_write_sector(b->data, b->blockno);
        }else{
	        sdcard_read_sector(b->data, b->blockno);
        }
    #else
        virtio_disk_rw(b, write);
        
    #endif
}

void disk_intr(void)
{
    #ifdef QEMU
    virtio_disk_intr();
    #elif defined(USE_MMC)
    dmac_intr(DMAC_CHANNEL0);
    #else 

    #endif
}
