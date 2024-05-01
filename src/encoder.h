#pragma once

typedef struct
{
    unsigned long left;
    unsigned long right;
} encoder_position_t;

void encoder_init();

encoder_position_t encoder_position();
