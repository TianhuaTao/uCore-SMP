#include "types.h"
#include "file.h"
#include "proc.h"
#include "defs.h"

#define FILE_MAX (128*16)
struct file filepool[FILE_MAX];

void
fileclose(struct file *f)
{
    if(f->ref < 1)
        panic("fileclose");
    if(--f->ref > 0) {
        return;
    }

    if(f->type == FD_PIPE){
        pipeclose(f->pipe, f->writable);
    }
    f->off = 0;
    f->readable = 0;
    f->writable = 0;
    f->ref = 0;
    f->type = FD_NONE;
}

struct file* filealloc() {
    for(int i = 0; i < FILE_MAX; ++i) {
        if(filepool[i].ref == 0) {
            filepool[i].ref = 1;
            return &filepool[i];
        }
    }
    return 0;
}

int init_mailbox(struct mailbox* mb){
    void* buf_pa = kalloc();
    if(buf_pa == 0 ){
        return 0;
    }
    init_spin_lock(&mb->lock);
    mb->mailbuf  =buf_pa;
    for (int i = 0; i < MAX_MAIL_IN_BOX; i++)
    {
        mb->length[i] = 0;
        mb->valid[i] = 0;
    }
    mb->head = 0;
    return 1;
}

