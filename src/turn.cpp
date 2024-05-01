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
static PID_angle_t pid;

#define Ts 10UL

void turn_init()
{
    PID_angle_init(&pid, Ts*1e-3);
    pid.umax = 255 - max(MOTOR_LEFT_OFFSET, MOTOR_RIGHT_OFFSET);
    PID_angle_new_params(&pid);
}


void turn_loop(unsigned long now)
{
    static unsigned long prev_millis = 0;
    if (now - prev_millis < Ts) return;
    prev_millis = now;

    if (turning)
    {
        float y = imu_angle_Z();
        int16_t u = (int16_t)PID_angle_loop(&pid, y, target);
        motor_move_lin(-u, u);

        if (abs(PID_angle_wrap(y - target)) < 5.0 && abs(u) < 10)
        {
            turning = false;
            motor_move_lin(0, 0);
        }
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
    float y = imu_angle_Z();
    target = PID_angle_wrap(y + angle);

    pid.Kp = conf.turn_Kp;
    pid.Ki = conf.turn_Ki;
    pid.Kd = conf.turn_Kd;
    pid.Tf = conf.turn_Tf;
    pid.Tt = conf.turn_Tt;
    PID_angle_new_params(&pid);
    PID_angle_reset(&pid, y);
}


/// Returns true if turn is in progress
bool turn_status()
{
    return turning;
}
