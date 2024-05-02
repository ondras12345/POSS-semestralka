#pragma once

void turn_init();
void turn_loop(unsigned long now);

void turn_turn_relative(float angle, bool expect_line=false);
bool turn_status();
