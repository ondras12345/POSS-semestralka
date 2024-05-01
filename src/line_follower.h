#pragma once
#include <stdint.h>
#include "crossroad.h"

void line_follower_init();
void line_follower_loop(unsigned long now);
int16_t line_follower_offset();
uint8_t line_follower_state();
uint8_t line_follower_state_debounced();
crossroad_t line_follower_crossroad();
crossroad_t line_follower_last_crossroad();
bool line_follower_last_crossroad_updated();
