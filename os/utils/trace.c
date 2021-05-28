#include "trace.h"
int64 tracepool[NCPU][MAX_TRACE_CNT];
int trace_last[NCPU]; // point to last write trace
struct spinlock tracelock;

void init_trace() {
    init_spin_lock_with_name(&tracelock, "tracelock");
    for (int i = 0; i < NCPU; i++) {
        for (int j = 0; j < MAX_TRACE_CNT; j++) {
            tracepool[i][j] = -1;
        }
        trace_last[i] = 0;
    }
}

void pushtrace(int64 id) {
    acquire(&tracelock);
    uint64 tp = cpuid();
    // phex(trace_last[tp]);
    trace_last[tp]++;
    trace_last[tp] = trace_last[tp] % MAX_TRACE_CNT;
    tracepool[tp][trace_last[tp]] = id;
    release(&tracelock);
}

int64 get_last_trace() {
    uint64 tp = cpuid();
    return tracepool[tp][trace_last[tp]];
}

void printtrace() {
    acquire(&tracelock);
    uint64 tp = cpuid();
    int last = trace_last[tp];
    printf("traceback: ");
    for (int i = 0; i < MAX_TRACE_CNT; i++) {
        printf("0x%x  ", tracepool[tp][last]);
        last--;
        if (last < 0)
            last = MAX_TRACE_CNT - 1;
    }
    printf("\n");
    release(&tracelock);
}