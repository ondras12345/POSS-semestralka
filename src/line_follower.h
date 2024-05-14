#pragma once
#include <stdint.h>
#include "crossroad.h"
#include "encoder.h"

void line_follower_init();
void line_follower_loop(unsigned long now);
int16_t line_follower_offset();
uint8_t line_follower_state();
uint8_t line_follower_state_debounced();
crossroad_t line_follower_crossroad();
crossroad_t line_follower_crossroad_fast();
crossroad_t line_follower_last_crossroad();
encoder_position_t line_follower_last_crossroad_position();
bool line_follower_last_crossroad_updated();
void line_follower_clear();

void line_follower_follow(uint8_t speed);
void line_follower_stop();
bool line_follower_following();
