#pragma once
#include <stdint.h>

typedef struct
{
    uint32_t left;
    uint32_t right;
} encoder_position_t;

void encoder_init();

encoder_position_t encoder_position();
int32_t encoder_distance_pulses(encoder_position_t start, encoder_position_t end);
int32_t encoder_distance_mm(encoder_position_t start, encoder_position_t end);
