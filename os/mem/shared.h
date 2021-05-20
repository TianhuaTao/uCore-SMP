#if !defined(SHARED_H)
#define SHARED_H

#include <ucore/ucore.h>

#define MAX_SHARED_NAME (64)    
#define MAX_SHARED_MEM_INSTANCE (256) // system level
#define MAX_SHARED_MEM_SIZE (1*1024*1024) // 1MB
#define MAX_SHARED_MEM_PAGE (MAX_SHARED_MEM_SIZE)/PGSIZE // 256 pages

struct shared_mem{
    char name[MAX_SHARED_NAME];
    int used;
    int ref;
    int page_cnt;
    void* mem_pages[MAX_SHARED_MEM_PAGE];
};

extern struct shared_mem shared_mem_pool[MAX_SHARED_MEM_INSTANCE];
struct shared_mem* get_shared_mem_by_name(char* name,int page_cnt);
void drop_shared_mem(struct shared_mem* shmem);
struct shared_mem* dup_shared_mem (struct shared_mem* shmem);
void* map_shared_mem(struct shared_mem *shmem);
void init_shared_mem();


#endif // SHARED_H
