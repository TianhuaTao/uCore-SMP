#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dsid.h>

const char str[] = "the quick brown fox jumps over the lazy dog";

int main()
{
    pid_t pid = getpid();
    int dsid = 0xA, mask = 0xF00D;
    set_dsid(pid, dsid);
    set_dsid_param(dsid, 0, 0, 0, mask);
    printf("pid: %d, dsid: %p, mask: %p\n", pid, dsid, mask);
    int64 prev_l2_traffic = get_l2_traffic(dsid);
    for (int i = 0; i < 10; ++i)
    {
        int64 l2_traffic = get_l2_traffic(dsid);
        if (l2_traffic < prev_l2_traffic)
            prev_l2_traffic -= UINT_MAX;
        printf("l1 to l2 speed: %d B/s\n", (l2_traffic - prev_l2_traffic) * 8);
        prev_l2_traffic = l2_traffic;
        puts(str);
        sleep(1000);
    }
    get_l2_traffic(0);
    printf("dsid test 1 passed!\n");
    return 0;
}
