#include <arch/riscv.h>
#include <ucore/defs.h>

#include "timer.h"


void timerinit() {

}

void start_timer_interrupt(){
    w_sie(r_sie() | SIE_STIE);
    set_next_timer();
}

void stop_timer_interrupt(){
    w_sie(r_sie() & ~SIE_STIE);
}

/// Set the next timer interrupt (10 ms)
void set_next_timer() {
    // 100Hz @ QEMU
    const uint64 timebase = TICK_FREQ / TIME_SLICE_PER_SEC; // how many ticks
    set_timer(r_time() + timebase);
    // mmiowb();
}

uint64 get_time_ms() {
    // mmiowb();
    return r_time() / (TICK_FREQ / MSEC_PER_SEC);
}

uint64 get_time_us() {
    return r_time() * USEC_PER_SEC / TICK_FREQ;
}