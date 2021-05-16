// #define USE_RAMDISK

#include <ucore/ucore.h>
#include "ramdisk.h"

void init_abstract_disk(){
    #ifdef USE_RAMDISK
        init_ram_disk();

    #else
        virtio_disk_init();

    #endif
}

void abstract_disk_rw(struct buf *b, int write){
    #ifdef USE_RAMDISK
        ram_disk_rw(b, write);
    #else
        virtio_disk_rw(b, write);
    #endif
}