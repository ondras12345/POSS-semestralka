#pragma once
#include <stdint.h>

void motor_init();
void motor_move(int16_t speed_left, int16_t speed_right);
void motor_move_lin(int16_t speed_left, int16_t speed_right);
