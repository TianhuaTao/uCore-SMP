#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>

int main()
{
    puts("execute dummy");
    for (int i = 0; i < 100; ++i)
    {
        int pid = fork();
        if (pid == 0)
            exec("dummy");
        // else
        //     sleep(100);
    }
    return 0;
}