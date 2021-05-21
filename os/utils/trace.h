#if !defined(TRACE_H)
#define TRACE_H
#include <ucore/ucore.h>
#include <arch/riscv.h>
#include <lock/lock.h>
#define MAX_TRACE_CNT 64
extern int64 tracepool[NCPU][MAX_TRACE_CNT];
extern int trace_last[NCPU];  // point to last write trace
extern struct spinlock tracelock;
void pushtrace(int64 id);
int64 get_last_trace();
void printtrace();
void init_trace();
#endif // TRACE_H
