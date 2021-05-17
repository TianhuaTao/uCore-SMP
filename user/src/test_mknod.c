#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int err;
    int fd;

    err = mknod("console2", 1, 0);
    assert(err==0, -1);

    fd = open("console2", O_RDWR);

    assert(fd>=0, -2);

    return 0;
}