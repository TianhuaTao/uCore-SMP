#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucore.h>
int NCPU = 4;

char bigdata[1024 * 200]; // 200K

int main(int argc, char *argv[]) {
    struct cpu_stat stat_buf[8];
    struct mem_stat mstat;
    struct proc_stat pstat[20];
    int fd = open("cpu", O_RDWR);
    int fd_mem = open("mem", O_RDWR);
    int fd_proc = open("proc", O_RDWR);
    assert(fd >= 0, -1);

    int bytes = read(fd, stat_buf, sizeof(stat_buf));
    printf("cpu_device bytes read %d\n", bytes);

    assert(bytes == sizeof(struct cpu_stat) * NCPU, -2); // only four

    int pid;
    int runtime[] = { 6, 4, 2};
    char name[] = {'n', 'a', 'm', 'e', '0', '0', '\0'};
    int child_id;
    for (int i = 0; i < 1; i++) {
        pid = fork();
        if (pid == 0) {
            child_id = i;
            name[4] += i;

            sleep(1000 * (i + 1));
            break;
        }
    }

    if (pid == 0) {
        int *shm;

        for (int i = 0; i < 20; i++) {
            // waste some memory
            shm = sharedmem(name, 1024 * 512); // 512 KB
            // printf("shm=%p\n",shm);
            name[5] = 'a' + i; // different name
        }

        shm[10] = 2;
        int x = shm[10];
        int t = time_ms();
        uint64 t2;
        // child
        while (1) {
            x = x * x - 3 * x;
            t2 = time_ms() - t;

            if (t2 > runtime[child_id] * 1000) {
                break;
            }
        }
    } else {
        uint64 start_time = time_ms();
        // parent
        while (1) {
            sleep(1000);
            uint64 time_sec = (time_ms() - start_time) / 1000;
            if (time_sec > 10)
                exit(0);
            bytes = read(fd, stat_buf, sizeof(stat_buf));
            int bytes_mem = read(fd_mem, &mstat, sizeof(mstat));
            int bytes_proc = read(fd_proc, &pstat, sizeof(pstat));
            int p_cnt = bytes_proc / sizeof(struct proc_stat);
            assert(bytes == sizeof(struct cpu_stat) * NCPU, -6); // only four
            assert(bytes_mem == sizeof(struct mem_stat), -8);    // only four
            printf("\x1b[2J");                                   // clear
            printf("----------------------------------------------------------------------\n");
            printf(" uCore-SMP Resource Monitor                            Time: %l s\n", time_sec);
            printf("----------------------------------------------------------------------\n\n");

            for (int i = 0; i < NCPU; i++) {

                printf("Core    %d\n", i);
                // printf("busy cycle:    %l\n", stat_buf[i].sample_busy_duration);
                // printf("all cycle:     %l\n", stat_buf[i].sample_duration);
                // printf("uptime cycle:  %l\n", stat_buf[i].uptime);
                int per = (100 * stat_buf[i].sample_busy_duration) / stat_buf[i].sample_duration;
                printf("[\x1b[31m");
                int tens = (per + 1) / 2;
                int i = 0;
                for (i = 0; i < tens; i++) {
                    printf("|");
                }
                for (; i < 50; i++) {
                    printf(" ");
                }

                printf("\x1b[0m] %d%%\n", per);
                printf("\n");
            }

            printf("Memory\n");
            printf("Total :   %l MB\n", mstat.physical_total / 1024 / 1024);
            printf("Free  :   %l MB\n", mstat.physical_free / 1024 / 1024);
            uint64 used = mstat.physical_total - mstat.physical_free;
            int use_per = (used * 100) / mstat.physical_total;
            printf("[\x1b[31m");
            int tens = (use_per + 1) / 2;
            int i = 0;
            for (i = 0; i < tens; i++) {
                printf("|");
            }
            for (; i < 50; i++) {
                printf(" ");
            }

            printf("\x1b[0m] %d%%\n", use_per);
            printf("\n");
            printf("----------------------------------------------------------------------\n");
            printf("Process | pid | ppid | heap | mem | cpu time\n", p_cnt);
            if (p_cnt > 0) {

                for (int i = 0; i < p_cnt; i++) {
                    printf("%s   ", pstat[i].name);
                    printf("%d   ", pstat[i].pid);
                    printf("%d   ", pstat[i].ppid);
                    printf("%l   ", pstat[i].heap_sz);
                    printf("%l   ", pstat[i].total_size);
                    printf("%l   ", pstat[i].cpu_time);
                    if (pstat[i].state == UNUSED) {
                        printf("UNUSED");
                    } else if (pstat[i].state == USED) {
                        printf("USED");
                    } else if (pstat[i].state == SLEEPING) {
                        printf("SLEEPING");
                    } else if (pstat[i].state == RUNNING) {
                        printf("RUNNING");
                    } else if (pstat[i].state == ZOMBIE) {
                        printf("ZOMBIE");
                    } else if (pstat[i].state == RUNNABLE) {
                        printf("RUNNABLE");
                    } else {
                        printf("UNKNOWN");
                    }
                    printf("\n");
                }
            }
            printf("----------------------------------------------------------------------\n\n");
        }
    }

    return 0;
}