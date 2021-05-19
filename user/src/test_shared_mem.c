#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    void* shmem1 = sharedmem("shmem1", 8192);
    void* shmem2 = sharedmem("shmem2", 8192);
    printf("shared mem 1 start: %p\n", shmem1);
    printf("shared mem 2 start: %p\n", shmem2);
    return 0;
}