#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    void *shmem1 = sharedmem("shmem1", 8192);
    printf("[prog 1] shared mem 1 start: %p\n", shmem1);

    sleep(1000);
    void *shmem2 = sharedmem("shmem2", 0);
    printf("[prog 1] shared mem 2 start: %p\n", shmem2);

    char *buf = (char *)shmem1;
    for (int i = 0; i < 8192; i++)
    {
        buf[i] = 1;
    }
    sleep(1000);
    buf = (char *)shmem2;
    for (int i = 0; i < 8192; i++)
    {

        buf[i] += 1;
    }
    sleep(1000);

    buf = (char *)shmem1;
    for (int i = 0; i < 8192; i++)
    {
        if(buf[i]!=3){
            exit(-1);
        }
    }
    printf("[prog 1] check good\n");
    
    return 0;
}