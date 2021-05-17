#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int err;

    err = mkdir("tmp_dir1");
    assert(err==0, -1);

    err = mkdir("tmp_dir2");
    assert(err==0, -2);

    err = mkdir("tmp_dir3");
    assert(err==0, -3);

    err = mkdir("tmp_dir3");    // existing
    assert(err==-1, -4);

    err = mkdir("/tmp_dir4");   // absolute
    assert(err==0, -5);

    err = mkdir("/tmp_dir1/tmp_dir_n1");   // nested
    assert(err==0, -6);

    err = mkdir("tmp_dir2/tmp_dir_n2");   // nested
    assert(err==0, -7);

    return 0;
}