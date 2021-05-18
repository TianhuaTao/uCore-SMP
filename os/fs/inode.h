#if !defined(INODE_H)
#define INODE_H
#include <ucore/ucore.h>

struct inode *
iget(uint dev, uint inum);

#endif // INODE_H
