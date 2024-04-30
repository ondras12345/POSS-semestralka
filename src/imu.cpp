#include "imu.h"
#include <MeGyro.h>

// IMU
static MeGyro gyro(1,0x69);

void imu_init()
{
    gyro.begin();
}

void imu_loop(unsigned long now)
{
    static unsigned long prev_millis = 0;
    if (now - prev_millis < 10UL) return;
    prev_millis = now;

    gyro.update();
}


double imu_angle_X() { return gyro.getAngleX(); };
double imu_angle_Y() { return gyro.getAngleY(); };
double imu_angle_Z() { return gyro.getAngleZ(); };


// TODO use IMU
// - nas zajime osa Z, minus je doleva
// - cisla jsou ve stupnich
// - perf counter rika 2900us
// - fast_update je uplne stejne pomaly
