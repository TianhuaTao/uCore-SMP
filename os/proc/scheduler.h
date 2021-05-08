#ifndef UCORE_SMP_SCHEDULER_H
#define UCORE_SMP_SCHEDULER_H
#include <ucore/types.h>
void init_scheduler();
void scheduler();
static const int64 BIGSTRIDE = 0x7FFFFFFFLL;

#endif //UCORE_SMP_SCHEDULER_H
