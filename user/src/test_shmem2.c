#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    void *shmem2 = sharedmem("shmem2", 8192);
    sleep(1000);
    void *shmem1 = sharedmem("shmem1", 0);
    
    sleep(1000);
    printf("[prog 2] shared mem 1 start: %p\n", shmem1);
    printf("[prog 2] shared mem 2 start: %p\n", shmem2);

    char *buf = (char *)shmem2;
    for (int i = 0; i < 8192; i++)
    {
        buf[i] = 2;
    }
    sleep(1000);

    buf = (char *)shmem1;
    for (int i = 0; i < 8192; i++)
    {
        buf[i] += 2;
    }
    sleep(1000);

    buf = (char *)shmem2;
    for (int i = 0; i < 8192; i++)
    {
        if(buf[i]!=3){
            exit(-1);
        }
    }
    sleep(1000);

    printf("[prog 2] check good\n");

    return 0;
}