#include "turn.h"
#include "imu.h"
#include "motor.h"
#include "hardware.h"
#include "conf.h"
#include <PID.h>
#include <stdint.h>
#include <Arduino.h>

static bool turning = false;
static float target = 0.0;
static PID_t pid;

#define Ts 10UL

void turn_init()
{
    PID_init(&pid, Ts*1e-3);
    pid.umax = 255 - max(MOTOR_LEFT_OFFSET, MOTOR_RIGHT_OFFSET);
    PID_new_params(&pid);
}


void turn_loop(unsigned long now)
{
    static unsigned long prev_millis = 0;
    if (now - prev_millis < Ts) return;
    prev_millis = now;

    if (turning)
    {
        float y = imu_angle_Z();
        int16_t u = (int16_t)PID_loop(&pid, y, target);
        motor_move_lin(-u, u);

        if (abs(y - target) < 5.0 && abs(u) < 10) turning = false;
    }
}


/**
 * Start a relative turn.
 * Angle is in degrees, negative angles turn left.
 */
void turn_turn_relative(float angle)
{
    // No need to check emergency, motor_move already does that.
    turning = true;
    target = imu_angle_Z() + angle;
    target -= 360*floor(target / 360);
    if (target > 180) target -= 360;

    pid.Kp = conf.turn_Kp;
    pid.Ki = conf.turn_Ki;
    pid.Kd = conf.turn_Kd;
    pid.b = conf.turn_b;
    pid.c = conf.turn_c;
    pid.Tf = conf.turn_Tf;
    pid.Tt = conf.turn_Tt;
    PID_new_params(&pid);

    // TODO reset PID to start in steady state / 0 integral
}


/// Returns true if turn is in progress
bool turn_status()
{
    return turning;
}
