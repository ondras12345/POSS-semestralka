#include "PID.h"
#include <string.h>
#include <math.h>


void PID_init(PID_t *pid, float Ts)
{
    memset(pid, 0, sizeof(PID_t));
    pid->b = 1.0;
    pid->c = 1.0;
    pid->umax = 1.0;
    pid->Ts = Ts;
    pid->Tt = INFINITY;
}


void PID_new_params(PID_t *pid)
{
    // vypocet diskretnich parametru
    pid->ci = 0.5f * pid->Ki * pid->Ts;
    pid->cd1 = -(pid->Ts - 2*pid->Tf) / (pid->Ts + 2*pid->Tf);
    pid->cd2 = 2*pid->Kd / (pid->Ts + 2*pid->Tf);
}


float PID_loop(PID_t *pid, float y, float w)
{
    // Bezrazove prepnuti
    pid->yi -= pid->Kp*(pid->b * pid->wkm1 - pid->ykm1);
    float edm1 = pid->c * pid->wkm1 - pid->ykm1;
    float edm2 = pid->c * pid->wkm2 - pid->ykm2;
    pid->yi -= pid->cd1 * pid->yd_old + pid->cd2 * (edm1 - edm2);

    // regulacni odchylka pro I, P, D
    float e = w - y - pid->tv;  // e[k]
    float ekm1 = pid->wkm1 - pid->ykm1 - pid->tvm1;  // e[k-1]
    float ep = pid->b*w - y;
    float ed = pid->c*w - y;

    pid->yi += pid->ci * (e + ekm1);
    pid->yd_old = pid->yd;
    pid->yd = pid->cd1 * pid->yd + pid->cd2 * (ed - edm1);
    float yp = pid->Kp * ep;

    float u_nosat = yp + pid->yi + pid->yd;
    float u_sat = u_nosat;

    if (u_nosat > pid->umax)
    {
        u_sat = pid->umax;
    }
    else if (u_nosat < -pid->umax)
    {
        u_sat = -pid->umax;
    }

    pid->tvm1 = pid->tv;
    pid->tv = (u_nosat - u_sat) / pid->Tt;

    // ulozime stare hodnoty
    pid->wkm2 = pid->wkm1;
    pid->ykm2 = pid->ykm1;
    pid->wkm1 = w;
    pid->ykm1 = y;

    // druha cast bezrazoveho prepnuti
    pid->yi += yp + pid->yd;

    return u_sat;
}


void PID_angle_init(PID_angle_t *pid, float Ts)
{
    memset(pid, 0, sizeof(PID_angle_t));
    pid->umax = 1.0;
    pid->Ts = Ts;
    pid->Tt = INFINITY;
}


void PID_angle_new_params(PID_angle_t *pid)
{
    pid->ci = 0.5f * pid->Ki * pid->Ts;
    pid->cd1 = -(pid->Ts - 2*pid->Tf) / (pid->Ts + 2*pid->Tf);
    pid->cd2 = 2*pid->Kd / (pid->Ts + 2*pid->Tf);
}


/**
 * Reset the controller to zero initial conditions.
 * Useful after not calling loop() for a while.
 */
void PID_angle_reset(PID_angle_t *pid, float y)
{
    pid->ykm1 = y;
    pid->wkm1 = 0;
    pid->yi = 0;
    pid->yi = 0;
    pid->tv = 0;
    pid->tvm1 = 0;
    pid->yd = 0;
}


/**
 * PID loop for angles.
 * This does NOT support 2DoF.
 * Does not support changing parameters on the fly without disturbing the
 * output.
 */
float PID_angle_loop(PID_angle_t *pid, float y, float w)
{
    float e = PID_angle_wrap(w - y);
    float ekm1 = PID_angle_wrap(pid->wkm1 - pid->ykm1);

    pid->yi += pid->ci * PID_angle_wrap(e-pid->tv + ekm1-pid->tvm1);
    pid->yd = pid->cd1 * pid->yd + pid->cd2 * (e - ekm1);
    float yp = pid->Kp * e;

    float u_nosat = yp + pid->yi + pid->yd;
    float u_sat = u_nosat;

    if (u_nosat > pid->umax)
    {
        u_sat = pid->umax;
    }
    else if (u_nosat < -pid->umax)
    {
        u_sat = -pid->umax;
    }

    pid->tvm1 = pid->tv;
    pid->tv = (u_nosat - u_sat) / pid->Tt;

    // ulozime stare hodnoty
    pid->wkm1 = w;
    pid->ykm1 = y;

    return u_sat;
}


/**
 * Normalize angle in degrees to between -180 and 180.
 */
float PID_angle_wrap(float angle)
{
    angle -= 360*floor(angle / 360);
    if (angle > 180) angle -= 360;
    return angle;
}
