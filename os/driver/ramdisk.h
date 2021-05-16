#if !defined(RAMDISK_H)
#define RAMDISK_H

struct buf;

void init_ram_disk();
void ram_disk_rw(struct buf *b, int write);

#endif // RAMDISK_H
