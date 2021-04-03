#include <unistd.h>

extern int main();

int __start_main(long* p)
{
    exit(main());
    return 0;
}
