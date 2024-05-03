#include "turn.h"
#include "imu.h"
#include "motor.h"
#include "hardware.h"
#include "conf.h"
#include "line_follower.h"
#include <PID.h>
#include <stdint.h>
#include <Arduino.h>

static bool turning = false;
static bool expect_line = false;
static uint16_t line_min = 0;
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

    if (!turning) return;

    float y = imu_angle_Z();
    int16_t u = (int16_t)PID_angle_loop(&pid, y, target);
    motor_move_lin(u, -u);
    // TODO keep encoder diff sum = 0?

    float e = PID_angle_wrap(y - target);
    if (expect_line && e < 0 && e > -2*conf.turn_overshoot)
    {
        int16_t off = line_follower_offset();
        if (abs(off) < line_min)
        {
            line_min = off;
            target = y;
        }
    }

    if (abs(e) < 2.5 && abs(u) < 5)
    {
        turning = false;
        motor_move_lin(0, 0);
    }
}


/**
 * Start a relative turn.
 * Angle is in degrees, negative angles turn left.
 */
void turn_turn_relative(float angle, bool p_expect_line)
{
    // No need to check emergency, motor_move already does that.
    turning = true;
    float y = imu_angle_Z();
    target = PID_angle_wrap(y + angle);
    expect_line = p_expect_line;
    line_min = conf.line_umax;

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


float turn_target()
{
    return target;
}
