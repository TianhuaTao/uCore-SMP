#include <arch/riscv.h>
#include <file/file.h>
#include <fs/buf.h>
#include <fs/fs.h>
#include <proc/proc.h>
#include <ucore/defs.h>
#include <ucore/types.h>
// there should be one superblock per disk device, but we run with
// only one device
struct superblock sb;

// Read the super block.
static void read_superblock(int dev, struct superblock *sb) {
    struct buf *bp;
    bp = acquire_buf_and_read(dev, 1);
    memmove(sb, bp->data, sizeof(*sb));
    release_buf(bp);
}

// Init fs
void fsinit()
{
    printf("[ucore] Initialize File System ...\n");
    int dev = ROOTDEV;
    read_superblock(dev, &sb);
    if (sb.magic != FSMAGIC)
    {
        panic("invalid file system");
    }
    printf("[ucore] File System Initialized\n");
}


int namecmp(const char *s, const char *t)
{
    return strncmp(s, t, DIRSIZ);
}
// Look for a directory entry in a directory.
// If found, set *poff to byte offset of entry.
// struct inode *
// dirlookup(struct inode *dp, char *name, uint *poff)
// {
//     uint off, inum;
//     // struct dirent de;

//     if (dp->type != T_DIR)
//         panic("dirlookup not DIR");

//     for (off = 0; off < dp->size; off += sizeof(de))
//     {
//         if (readi(dp, 0, &de, off, sizeof(de)) != sizeof(de))
//             panic("dirlookup read");
//         if (de.inum == 0)
//             continue;
//         if (namecmp(name, de.name) == 0)
//         {
//             // entry matches path element
//             if (poff)
//                 *poff = off;
//             inum = de.inum;
//             return iget(dp->dev);
//         }
//     }

//     return 0;

// }

struct inode *
dirlookup(struct inode *dp,char*path){

    struct inode* ip=iget(dp->dev);
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;

    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                i = strlen(path);
                sprintf(&path[i], "/%s", fno.fname);
                res = scan_files(path);                    /* Enter the directory */
                if (res != FR_OK) break;
                path[i] = 0;
            } else {                                       /* It is a file. */
                printf("%s/%s\n", path, fno.fname);
            }
        }
        f_closedir(&dir);
    }
    ip->DIRECTORY=&dir;
    return ip;
}


// Write a new directory entry (name, inum) into the directory dp.
int dirlink(struct inode *dp, char *name, uint inum)
{
    // int off;
    // struct dirent de;
    // struct inode *ip;
    // // Check that name is not present.
    // if ((ip = dirlookup(dp, name, 0)) != 0)
    // {
    //     iput(ip);
    //     return -1;
    // }

    // // Look for an empty dirent.
    // for (off = 0; off < dp->size; off += sizeof(de))
    // {
    //     if (readi(dp, FALSE, &de, off, sizeof(de)) != sizeof(de))
    //         panic("dirlink read");
    //     if (de.inum == 0)
    //         break;
    // }
    // strncpy(de.name, name, DIRSIZ);
    // de.inum = inum;
    // if (writei(dp, FALSE, &de, off, sizeof(de)) != sizeof(de))
    //     panic("dirlink");
    // return 0;
}

struct inode *root_dir()
{
    debugcore("root_dir");
    struct inode *r = iget(ROOTDEV);
    f_opendir(r->DIRECTORY, "/");	
    r->type=T_DIR;
    ivalid(r);
    return r;
}


// Copy stat information from inode.
// Caller must hold ip->lock.
void stati(struct inode *ip, struct stat *st)
{
    st->dev = ip->dev;
    st->ino = ip->inum;
    st->type = ip->type;
    st->nlink = ip->num_link;
    st->size = ip->size;
}
// Is the directory dp empty except for "." and ".." ?
int isdirempty(struct inode *dp)
{
    int off;
    struct dirent de;

    for (off = 2 * sizeof(de); off < dp->size; off += sizeof(de))
    {
        if (readi(dp, 0, (void *)&de, off, sizeof(de)) != sizeof(de))
            panic("isdirempty: readi");
        if (de.inum != 0)
            return 0;
    }
    return 1;
}