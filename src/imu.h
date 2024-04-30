#pragma once

void imu_init();
void imu_loop(unsigned long now);
double imu_angle_X();
double imu_angle_Y();
double imu_angle_Z();
