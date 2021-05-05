#if !defined(DEFS_H)
#define DEFS_H

#include "types.h"
struct context;
struct proc;
struct file;
struct pipe;
struct superblock;
struct inode;
struct buf;

// panic.c
void loop();
void panic(char *);

// sbi.c
void console_putchar(int);
int console_getchar();
void shutdown();
void set_timer(uint64 stime);


// printf.c
void printf(char *, ...);
void printf_k(char *fmt, ...);
void
printfinit(void);

// trap.c
void trapinit();
void usertrapret();
void set_usertrap();
void set_kerneltrap();

// string.c
int memcmp(const void *, const void *, uint);
void *memmove(void *, const void *, uint);
void *memset(void *, int, uint);
char *safestrcpy(char *, const char *, int);
int strlen(const char *);
int strncmp(const char *, const char *, uint);
char *strncpy(char *, const char *, int);

// syscall.c
void syscall();

// swtch.S
void swtch(struct context *, struct context *);

// loader.c
// int finished();
void batchinit();
int run_all_app();
int get_id_by_name(char* name);
void loader(int, void*);

// proc.c
struct proc *curr_proc();
void exit(int);
void procinit();
void scheduler(); // __attribute__((noreturn));
void sched();
void yield();
int fork(void);
int exec(char*);
int wait(int, int*);
struct proc* allocproc();
void init_scheduler();
int fdalloc(struct file *);


// kalloc.c
void *alloc_physical_page(void);
void recycle_physical_page(void *);
void kinit(void);

// vm.c
void kvminit(void);
void kvmmap(pagetable_t, uint64, uint64, uint64, int);
int mappages(pagetable_t, uint64, uint64, uint64, int);
pagetable_t create_empty_user_pagetable(void);
void uvminit(pagetable_t, uchar *, uint);
uint64 uvmalloc(pagetable_t, uint64, uint64);
uint64 uvmdealloc(pagetable_t, uint64, uint64);
int uvmcopy(pagetable_t, pagetable_t, uint64);
void uvmfree(pagetable_t, uint64);
void uvmunmap(pagetable_t, uint64, uint64, int);
void uvmclear(pagetable_t, uint64);
uint64 walkaddr(pagetable_t, uint64);
uint64 virt_addr_to_physical(pagetable_t, uint64);
int copyout(pagetable_t, uint64, char *, uint64);
int copyin(pagetable_t, char *, uint64, uint64);
int copyinstr(pagetable_t, char *, uint64, uint64);
int map1page(pagetable_t pagetable, uint64 va, uint64 pa, int perm);
pte_t *walk(pagetable_t pagetable, uint64 va, int alloc);
void
kvminithart();

int either_copyout(int user_dst, uint64 dst, char* src, uint64 len);
int either_copyin(int user_src, uint64 src, char* dst, uint64 len);

// timer.c
uint64 get_cycle();
void timerinit();
void set_next_timer();
uint64 get_time_ms();
// pipe.c
int pipealloc(struct file *, struct file *);
void pipeclose(struct pipe *, int);
int piperead(struct pipe *, uint64, int);
int pipewrite(struct pipe *, uint64, int);

// file.c
void fileclose(struct file *);
struct file* filealloc();
int fileopen(char*, uint64);
uint64 filewrite(struct file*, uint64, uint64);
uint64 fileread(struct file*, uint64, uint64);

// plic.c
void plicinit(void);
void plicinithart(void);
int plic_claim(void);
void plic_complete(int);

// virtio_disk.c
void virtio_disk_init(void);
void virtio_disk_rw(struct buf *, int);
void virtio_disk_intr(void);

// fs.c
void fsinit();
int dirlink(struct inode *, char *, uint);
struct inode *dirlookup(struct inode *, char *, uint *);
struct inode *ialloc(uint, short);
struct inode *idup(struct inode *);
void iinit();
void ivalid(struct inode *);
void iput(struct inode *);
void iunlock(struct inode *);
void iunlockput(struct inode *);
void iupdate(struct inode *);
struct inode *namei(char *);
struct inode *root_dir();
int readi(struct inode *, int, uint64, uint, uint);
int writei(struct inode *, int, uint64, uint, uint);
void itrunc(struct inode *);

// bio.c
void binit(void);
struct buf *bread(uint, uint);
void brelse(struct buf *);
void bwrite(struct buf *);
void bpin(struct buf *);
void bunpin(struct buf *);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#define PAGE_SIZE (4096)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif // DEFS_H



#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define TRUE 1
#define FALSE 0
