#include "defs.h"
#include "memory_layout.h"
#include "riscv.h"
#include "log.h"
pagetable_t kernel_pagetable;

extern char e_text[]; // kernel.ld sets this to end of kernel code.
extern char trampoline[];

// Make a direct-map page table for the kernel.
pagetable_t kvmmake(void)
{
    pagetable_t kpgtbl;

    kpgtbl = (pagetable_t)kalloc();
    memset(kpgtbl, 0, PGSIZE);

    // map kernel text executable and read-only.
    debugf("kernel text va=%p -> [%p, %p]", KERNBASE, KERNBASE, (uint64)e_text);
    kvmmap(kpgtbl, KERNBASE, KERNBASE, (uint64)e_text - KERNBASE, PTE_R | PTE_X);
    debugf("");

    // map kernel data and the physical RAM we'll make use of.
    debugf("kernel data va=%p -> [%p, %p]", e_text, e_text, PHYSTOP);
    kvmmap(kpgtbl, (uint64)e_text, (uint64)e_text, PHYSTOP - (uint64)e_text, PTE_R | PTE_W);
    debugf("");

    // map trampoline
    debugf("trampoline va=%p -> [%p, %p]", TRAMPOLINE, trampoline, trampoline + PGSIZE);
    kvmmap(kpgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);
    debugf("");

    return kpgtbl;
}

// Initialize the one kernel_pagetable
// Switch h/w page table register to the kernel's page table,
// and enable paging.
void kvminit(void)
{
    // generate the kernel page table
    kernel_pagetable = kvmmake();
}

// Switch h/w page table register to the kernel's page table,
// and enable paging.
void kvminithart()
{
    // enable virtual memory
    w_satp(MAKE_SATP(kernel_pagetable));

    // update TLB
    sfence_vma();
    infof("enable pageing at %p", r_satp());
}

// Return the address of the PTE in page table pagetable
// that corresponds to virtual address va.  If alloc!=0,
// create any required page-table pages.
//
// The risc-v Sv39 scheme has three levels of page-table
// pages. A page-table page contains 512 64-bit PTEs.
// A 64-bit virtual address is split into five fields:
//   39..63 -- must be zero.
//   30..38 -- 9 bits of level-2 index.
//   21..29 -- 9 bits of level-1 index.
//   12..20 -- 9 bits of level-0 index.
//    0..11 -- 12 bits of byte offset within the page.
pte_t *
walk(pagetable_t pagetable, uint64 va, int alloc)
{
    if (va >= MAXVA)
        panic("walk");
    // debug("va=%p\n", va);
    for (int level = 2; level > 0; level--)
    {
        // debug("px=%p\n", PX(level, va));
        // debug("pxshift=%p, level=%d\n", PXSHIFT(level), level);
        pte_t *pte = &pagetable[PX(level, va)];
        if (*pte & PTE_V)
        {
            pagetable = (pagetable_t)PTE2PA(*pte);
        }
        else
        {
            if (!alloc || (pagetable = (pde_t *)kalloc()) == 0)
                return 0;
            // debug("new pagetable %p\n", pagetable);

            memset(pagetable, 0, PGSIZE);
            *pte = PA2PTE(pagetable) | PTE_V;
        }
    }
    return &pagetable[PX(0, va)];
}

// Look up a virtual address, return the physical address,
// or 0 if not mapped.
// Can only be used to look up user pages.
uint64
walkaddr(pagetable_t pagetable, uint64 va)
{
    pte_t *pte;
    uint64 pa;

    if (va >= MAXVA)
        return 0;

    pte = walk(pagetable, va, 0);
    if (pte == 0)
        return 0;
    if ((*pte & PTE_V) == 0)
        return 0;
    if ((*pte & PTE_U) == 0)
        return 0;
    pa = PTE2PA(*pte);
    return pa;
}

// Look up a virtual address, return the physical address,
uint64 useraddr(pagetable_t pagetable, uint64 va)
{
    uint64 page = walkaddr(pagetable, va);
    if (page == 0)
        return 0;
    return page | (va & 0xFFFULL);
}

// add a mapping to the kernel page table.
// only used when booting.
// does not flush TLB or enable paging.
void kvmmap(pagetable_t kpgtbl, uint64 va, uint64 pa, uint64 sz, int perm)
{
    if (mappages(kpgtbl, va, sz, pa, perm) != 0)
        panic("kvmmap");
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned. Returns 0 on success, -1 if walk() couldn't
// allocate a needed page-table page.
int mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
{
    uint64 a, last;
    pte_t *pte;

    // debugf("va=%p->pa=%p, size=%p, UXWR=%d%d%d%d", va, pa, size,
    //        HAS_BIT(perm, PTE_U),
    //        HAS_BIT(perm, PTE_X),
    //        HAS_BIT(perm, PTE_W),
    //        HAS_BIT(perm, PTE_R));
    a = PGROUNDDOWN(va);
    last = PGROUNDDOWN(va + size - 1);
    // int cnt =0 ;
    for (;;)
    {
        // cnt+=1;
        if ((pte = walk(pagetable, a, 1)) == 0)
            return -1;
        if (*pte & PTE_V)
            panic("remap");
        *pte = PA2PTE(pa) | perm | PTE_V;
        if (a == last)
            break;
        a += PGSIZE;
        pa += PGSIZE;
    }
    // debugf("map page cnt=%d", cnt);
    return 0;
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned. Returns 0 on success, -1 if walk() couldn't
// allocate a needed page-table page.
int map1page(pagetable_t pagetable, uint64 va, uint64 pa, int perm)
{
    uint64 a;
    pte_t *pte;
    uint64 size = PGSIZE;
    // debugf("va=%p->pa=%p, size=%p, UXWR=%d%d%d%d", va, pa, size,
    //        HAS_BIT(perm, PTE_U),
    //        HAS_BIT(perm, PTE_X),
    //        HAS_BIT(perm, PTE_W),
    //        HAS_BIT(perm, PTE_R));
    a = PGROUNDDOWN(va);

    if ((pte = walk(pagetable, a, 1)) == 0)
        return -1;
    if (*pte & PTE_V)
    {
        infof("map1page: remap\n");
        return -2;
    }
    *pte = PA2PTE(pa) | perm | PTE_V;

    return size;
}

// Remove npages of mappings starting from va. va must be
// page-aligned. The mappings must exist.
// Optionally free the physical memory.
void uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
    uint64 a;
    pte_t *pte;

    if ((va % PGSIZE) != 0)
        panic("uvmunmap: not aligned");

    for (a = va; a < va + npages * PGSIZE; a += PGSIZE)
    {
        if ((pte = walk(pagetable, a, 0)) == 0)
            panic("uvmunmap: walk");
        if ((*pte & PTE_V) == 0)
            panic("uvmunmap: not mapped");
        if (PTE_FLAGS(*pte) == PTE_V)
            panic("uvmunmap: not a leaf");
        if (do_free)
        {
            uint64 pa = PTE2PA(*pte);
            kfree((void *)pa);
        }
        *pte = 0;
    }
}

// create an empty user page table.
// map trampoline page
// returns 0 if out of memory.
pagetable_t
uvmcreate()
{
    pagetable_t pagetable;
    pagetable = (pagetable_t)kalloc();
    if (pagetable == 0)
        return 0;
    memset(pagetable, 0, PGSIZE);

    // if (mappages(pagetable, TRAMPOLINE, PGSIZE,
    //              (uint64)trampoline, PTE_R | PTE_X) < 0)
    // {
    //     uvmfree(pagetable, 0);
    //     return 0;
    // }
    // debugf("map user trampoline\n");
    return pagetable;
}

// Allocate PTEs and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
uint64
uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
    char *mem;
    uint64 a;

    if (newsz < oldsz)
        return oldsz;

    oldsz = PGROUNDUP(oldsz);
    for (a = oldsz; a < newsz; a += PGSIZE)
    {
        mem = kalloc();
        if (mem == 0)
        {
            uvmdealloc(pagetable, a, oldsz);
            return 0;
        }
        memset(mem, 0, PGSIZE);
        if (mappages(pagetable, a, PGSIZE, (uint64)mem, PTE_W | PTE_X | PTE_R | PTE_U) != 0)
        {
            kfree(mem);
            uvmdealloc(pagetable, a, oldsz);
            return 0;
        }
    }
    return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
uint64
uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
    if (newsz >= oldsz)
        return oldsz;

    if (PGROUNDUP(newsz) < PGROUNDUP(oldsz))
    {
        int npages = (PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
        uvmunmap(pagetable, PGROUNDUP(newsz), npages, 1);
    }

    return newsz;
}

// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void debugwalk(pagetable_t pagetable, int depth) {
    // there are 2^9 = 512 PTEs in a page table.
    for (int i = 0; i < 512; i++) {
        pte_t pte = pagetable[i];
        if(pte != 0)
            infof("{%d} pg[%d] = %p\n", depth, i, pte);
        if ((pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X)) == 0) {
            // this PTE points to a lower-level page table.
            uint64 child = PTE2PA(pte);
            debugwalk((pagetable_t) child, depth + 1);
        }
    }
}

// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void freewalk(pagetable_t pagetable)
{
    // there are 2^9 = 512 PTEs in a page table.
    for (int i = 0; i < 512; i++)
    {
        pte_t pte = pagetable[i];
        if ((pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X)) == 0)
        {
            // this PTE points to a lower-level page table.
            uint64 child = PTE2PA(pte);
            freewalk((pagetable_t)child);
            pagetable[i] = 0;
        }
        // TODO: enable this
        // else if (pte & PTE_V)
        // {
        //     panic("freewalk: leaf");
        // }
    }
    kfree((void *)pagetable);
}

// Free user memory pages,
// then free page-table pages.
void uvmfree(pagetable_t pagetable, uint64 sz)
{
    if (sz > 0)
        uvmunmap(pagetable, 0, PGROUNDUP(sz) / PGSIZE, 1);
    freewalk(pagetable);
}

// mark a PTE invalid for user access.
// used by exec for the user stack guard page.
void uvmclear(pagetable_t pagetable, uint64 va)
{
    pte_t *pte;

    pte = walk(pagetable, va, 0);
    if (pte == 0)
        panic("uvmclear");
    *pte &= ~PTE_U;
}

int uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
    pte_t *pte;
    uint64 pa, i;
    uint flags;
    char *mem;

    for(i = 0; i < sz; i += PGSIZE){
        if((pte = walk(old, i, 0)) == 0)
            panic("uvmcopy: pte should exist");
        if((*pte & PTE_V) == 0)
            panic("uvmcopy: page not present");
        pa = PTE2PA(*pte);
        flags = PTE_FLAGS(*pte);
        if((mem = kalloc()) == 0)
            goto err;
        memmove(mem, (char*)pa, PGSIZE);
        if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0){
            kfree(mem);
            goto err;
        }
    }
    return 0;

    err:
    uvmunmap(new, 0, i / PGSIZE, 1);
    return -1;
}

// Copy from kernel to user.
// Copy len bytes from src to virtual address dstva in a given page table.
// Return 0 on success, -1 on error.
int copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
    uint64 n, va0, pa0;

    while (len > 0)
    {
        va0 = PGROUNDDOWN(dstva);
        pa0 = walkaddr(pagetable, va0);
        if (pa0 == 0)
            return -1;
        n = PGSIZE - (dstva - va0);
        if (n > len)
            n = len;
        memmove((void *)(pa0 + (dstva - va0)), src, n);

        len -= n;
        src += n;
        dstva = va0 + PGSIZE;
    }
    return 0;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
    uint64 n, va0, pa0;

    while (len > 0)
    {
        va0 = PGROUNDDOWN(srcva);
        pa0 = walkaddr(pagetable, va0);
        if (pa0 == 0)
            return -1;
        n = PGSIZE - (srcva - va0);
        if (n > len)
            n = len;
        memmove(dst, (void *)(pa0 + (srcva - va0)), n);

        len -= n;
        dst += n;
        srcva = va0 + PGSIZE;
    }
    return 0;
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
    uint64 n, va0, pa0;
    int got_null = 0, len = 0;

    while (got_null == 0 && max > 0)
    {
        va0 = PGROUNDDOWN(srcva);
        pa0 = walkaddr(pagetable, va0);
        // debug("srcva=%p ,va0=%p, pa0=%p\n",srcva, va0 ,pa0);
        if (pa0 == 0)
            return -1;
        n = PGSIZE - (srcva - va0);
        if (n > max)
            n = max;

        char *p = (char *)(pa0 + (srcva - va0));
        while (n > 0)
        {
            if (*p == '\0')
            {
                *dst = '\0';
                got_null = 1;
                break;
            }
            else
            {
                *dst = *p;
            }
            --n;
            --max;
            p++;
            dst++;
            len++;
        }

        srcva = va0 + PGSIZE;
    }
    return len;
}
