#pragma once
#include <stdint.h>

typedef enum {
    cr_0 = '0', // nevim / neinicializovano
    cr_I = 'I', // rovne
    cr_G = 'G', // doprava
    cr_7 = '7', // doleva
    cr_T = 'T', // doleva a doprava
    cr_E = 'E', // doprava a rovne
    cr_3 = '3', // doleva a rovne
    cr_X = 'X', // +
} crossroad_t;

void line_follower_init();
void line_follower_loop(unsigned long now);
int16_t line_follower_offset();
uint8_t line_follower_state();
uint8_t line_follower_state_debounced();
crossroad_t line_follower_crossroad();
crossroad_t line_follower_last_crossroad();
bool line_follower_last_crossroad_updated();
