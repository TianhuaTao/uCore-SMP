#include <mem/shared.h>
#include <lock/spinlock.h>
#include <proc/proc.h>
struct shared_mem shared_mem_pool[MAX_SHARED_MEM_INSTANCE];
struct spinlock shared_mem_pool_lock;

void init_shared_mem()
{
    init_spin_lock_with_name(&shared_mem_pool_lock, "shared_mem_pool_lock");
    for (int i = 0; i < MAX_SHARED_MEM_INSTANCE; i++)
    {
        shared_mem_pool[i].used = FALSE;
        shared_mem_pool[i].ref = 0;
        shared_mem_pool[i].page_cnt = 0;
        memset(shared_mem_pool[i].name, 0, MAX_SHARED_NAME);
        memset(shared_mem_pool[i].mem_pages, 0, sizeof(shared_mem_pool[i].mem_pages));
    }
}
struct shared_mem *dup_shared_mem(struct shared_mem *shmem)
{
    acquire(&shared_mem_pool_lock);
    shmem->ref++;
    release(&shared_mem_pool_lock);
    return shmem;
}

// find a shared mem struct with name
// if one exists, the page_cnt is ignored
// if none exists, will create one, allocate page_cnt pages
struct shared_mem *get_shared_mem_by_name(char *name, int page_cnt)
{
    acquire(&shared_mem_pool_lock);

    // find created ones
    for (int i = 0; i < MAX_SHARED_MEM_INSTANCE; i++)
    {
        if (shared_mem_pool[i].used && strncmp(name, shared_mem_pool[i].name, MAX_SHARED_NAME) == 0)
        {
            shared_mem_pool[i].ref++;
            release(&shared_mem_pool_lock);
            return &shared_mem_pool[i];
        }
    }

    // not found, create one

    for (int i = 0; i < MAX_SHARED_MEM_INSTANCE; i++)
    {
        if (shared_mem_pool[i].used == FALSE) // empty
        {
            shared_mem_pool[i].used = TRUE;
            shared_mem_pool[i].ref = 1;
            shared_mem_pool[i].page_cnt = 0;
            strncpy(shared_mem_pool[i].name, name, MAX_SHARED_NAME);
            memset(shared_mem_pool[i].mem_pages, 0, sizeof(shared_mem_pool[i].mem_pages));

            for (int j = 0; j < page_cnt; j++)
            {
                void *p = alloc_physical_page();
                if (p == NULL)
                {
                    // not enough mem, free all
                    for (int k = 0; k < j; k++)
                    {
                        recycle_physical_page(shared_mem_pool[i].mem_pages[k]);
                    }
                    // clear
                    shared_mem_pool[i].used = FALSE;
                    shared_mem_pool[i].ref = 0;
                    shared_mem_pool[i].page_cnt = 0;
                    memset(shared_mem_pool[i].name, 0, MAX_SHARED_NAME);
                    memset(shared_mem_pool[i].mem_pages, 0, sizeof(shared_mem_pool[i].mem_pages));
                    release(&shared_mem_pool_lock);
                    return NULL;
                }
                shared_mem_pool[i].mem_pages[j] = p;
            }
            shared_mem_pool[i].page_cnt = page_cnt;
            release(&shared_mem_pool_lock);
            return &shared_mem_pool[i];
        }
    }
    // all used
    release(&shared_mem_pool_lock);
    return NULL;
}

void drop_shared_mem(struct shared_mem *shmem)
{
    acquire(&shared_mem_pool_lock);
    shmem->ref--;

    if (shmem->ref == 0)
    {
        shmem->used = FALSE;
        memset(shmem->name, 0, MAX_SHARED_NAME);
        for (int j = 0; j < shmem->page_cnt; j++)
        {
            recycle_physical_page(shmem->mem_pages[j]);
            shmem->mem_pages[j] = NULL;
        }
        shmem->page_cnt = 0;
    }

    release(&shared_mem_pool_lock);
}

void *map_shared_mem(struct shared_mem *shmem)
{
    struct proc *p = curr_proc();

    void *start_addr_va = p->next_shmem_addr;

    for (int i = 0; i < shmem->page_cnt; i++)
    {
        int err = mappages(p->pagetable, (uint64)(start_addr_va + i * PGSIZE), PGSIZE, (uint64)shmem->mem_pages[i], PTE_R | PTE_W | PTE_X | PTE_U);
        if (err)
        {
            panic("map_shared_mem");
        }
    }

    p->next_shmem_addr = start_addr_va + shmem->page_cnt* PGSIZE + PGSIZE;  // one guard page
    return start_addr_va;
}