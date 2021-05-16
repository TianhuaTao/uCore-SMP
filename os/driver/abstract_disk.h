#if !defined(ABSTRACT_DISK_H)
#define ABSTRACT_DISK_H


void init_abstract_disk();
void abstract_disk_rw(struct buf *b, int write);

#endif // ABSTRACT_DISK_H
