#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    printf("Will sleep for 10 seconds\n");
    for (int i = 0; i < 10; i++)
    {
        printf("%d\n",i);
        sleep(1000);
    }

    return 0;
}