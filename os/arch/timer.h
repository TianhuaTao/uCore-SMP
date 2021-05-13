#if !defined(TIMER_H)
#define TIMER_H


#include <ucore/defs.h>
uint64 get_time_ms();
uint64 get_time_us();

void start_timer_interrupt();

void stop_timer_interrupt();

#endif // TIMER_H

