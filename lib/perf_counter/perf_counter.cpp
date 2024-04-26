#include "perf_counter.h"
#include <Arduino.h>


void perf_counter_start(perf_counter_t * counter)
{
    counter->start = micros();
}

void perf_counter_stop(perf_counter_t * counter)
{
    unsigned long time = micros() - counter->start;
    counter->max = time > counter->max ? time : counter->max;
}


unsigned long perf_counter_reset(perf_counter_t * counter)
{
    typeof(counter->max) max = counter->max;
    counter->max = 0;
    return max;
}
