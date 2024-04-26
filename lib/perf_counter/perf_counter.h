#pragma once
// Measure what slows down main loop

typedef struct {
    const char * name;
    unsigned long start;
    unsigned long max;
} perf_counter_t;

void perf_counter_start(perf_counter_t * counter);

void perf_counter_stop(perf_counter_t * counter);

unsigned long perf_counter_reset(perf_counter_t * counter);

#define perf_counter_measure(counter, code) do { perf_counter_start(counter); code; perf_counter_stop(counter); } while (0)
