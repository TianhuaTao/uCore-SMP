#if !defined(ABSTRACT_DISK_H)
#define ABSTRACT_DISK_H

#include <fs/buf.h>
void init_abstract_disk();
void abstract_disk_rw(struct buf *b, int write);
void disk_intr(void);

#endif // ABSTRACT_DISK_H
