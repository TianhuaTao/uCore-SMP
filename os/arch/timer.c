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
    const uint64 timebase = TICK_FREQ / TICKS_PER_SEC; // how many ticks
    set_timer(r_time() + timebase);
}


uint64 get_time_ms() {
    return get_cycle() / (CPU_FREQ / MSEC_PER_SEC);
}

uint64 get_time_us() {
    // TODO: use variables, not literal numbers
    return get_cycle() / (CPU_FREQ / USEC_PER_SEC);
}