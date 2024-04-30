#include "PID.h"
#include <string.h>


void PID_init(PID_t *pid, float Ts)
{
    memset(pid, 0, sizeof(PID_t));
    pid->b = 1.0;
    pid->c = 1.0;
    pid->umax = 1.0;
    pid->Ts = Ts;
}


void PID_new_params(PID_t *pid)
{
    // vypocet diskretnich parametru
    pid->ci = pid->Ki * pid->Ts / 2;
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
