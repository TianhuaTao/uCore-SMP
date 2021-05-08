#include <arch/riscv.h>
#include <ucore/defs.h>
#include <mem/memory_layout.h>
#include <utils/log.h>
#include <lock/lock.h>
void freerange(void *pa_start, void *pa_end);

extern char ekernel[];

struct linklist
{
    struct linklist *next;
};

struct
{
    struct spinlock lock;
    struct linklist *freelist;
    uint64 free_page_count;
} kmem;

/**
 * Kernel init
 */
void kinit()
{
    init_spin_lock_with_name(&kmem.lock, "kmem.lock");
    freerange(ekernel, (void *)PHYSTOP);
    kmem.free_page_count = 0;
}

void freerange(void *pa_start, void *pa_end)
{
    char *p;
    p = (char *)PGROUNDUP((uint64)pa_start);
    for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
        recycle_physical_page(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to alloc_physical_page().  (The exception is when
// initializing the allocator; see kinit above.)
void recycle_physical_page(void *pa)
{
    struct linklist *l;
    if (((uint64)pa % PGSIZE) != 0 || (char *)pa < ekernel || (uint64)pa >= PHYSTOP)
        panic("recycle_physical_page");
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);
    l = (struct linklist *)pa;

    acquire(&kmem.lock);

    l->next = kmem.freelist;

    kmem.freelist = l;
    kmem.free_page_count++;
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
alloc_physical_page(void)
{
    struct linklist *l;
    acquire(&kmem.lock);
    l = kmem.freelist;
    if (l)
    {
        kmem.freelist = l->next;
        kmem.free_page_count--;
    }
    release(&kmem.lock);
    if(l)
        memset((char *)l, 5, PGSIZE); // fill with junk
    else 
        warnf("Out of memory");
    return (void *)l;
}