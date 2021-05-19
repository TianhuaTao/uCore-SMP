#include <arch/cpu.h>
#include <file/file.h>
#include <ucore/ucore.h>
uint64 get_free_page_count();

struct mem_stat
{
    uint64 physical_total;
    uint64 physical_free;
};

int64 mem_write(char *src, int64 len, int from_user)
{
    // you cannot write to mem stat
    return -1;
}
int64 mem_read(char *dst, int64 len, int to_user)
{
    struct mem_stat stat_buf;

    if (len > sizeof(struct mem_stat))
    {
        len = sizeof(struct mem_stat);
    }

    stat_buf.physical_free = get_free_page_count() * PGSIZE;
    stat_buf.physical_total = 128 * 1024 * 1024;
    // printf("get_free_page_count=%d\n", get_free_page_count());

    if (either_copyout(dst, &stat_buf, len, to_user) < 0)
        return -1;

    return len;
}

void mem_device_init()
{
    device_rw_handler[MEM_DEVICE].read = mem_read;
    device_rw_handler[MEM_DEVICE].write = mem_write;
}
