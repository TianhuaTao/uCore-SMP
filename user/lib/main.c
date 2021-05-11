#include <unistd.h>

extern int main(int argc, char *argv[]);

void __start_main(int argc, char* argv[])
{
    exit(main(argc, argv));
}
