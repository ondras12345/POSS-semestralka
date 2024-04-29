#include "PID.h"
#include <string.h>


void PID_init(PID_t *pid, float Ts)
{
    memset(pid, 0, sizeof(PID_t));
    pid->b = 1.0;
    pid->b_old = 1.0;
    pid->c = 1.0;
    pid->c_old = 1.0;
    pid->umax = 1.0;
    pid->Ts = Ts;
    // TODO?
}


void PID_new_params(PID_t *pid)
{
    // TODO osetrit bezrazove prepnuti
    // %Offset stare hodnoty integrator pro kompenzaci razu vlivem zmeny
    // %parametru - P-slozka
    // %yi = yi -( Kp*(b*wkm1-ykm1)-Kp_old*(b_old*wkm1-ykm1) );
    // yi = yi - Kp*(b*wkm1-ykm1); %treti clen muzu pricist na konci cyklu
    // 
    //        
    // %D - slozka
    // %cd1_old=-(Ts-2*Tf_old)/(Ts+2*Tf_old); 
    // %cd2_old=2*Kd_old/(Ts+2*Tf_old);
    // 
    // %yi = yi -( cd1*yd + cd2*(c*wkm1-ykm1-(c*wkm2-ykm2) ) - (cd1_old*yd + cd2_old*(c_old*wkm1-ykm1-(c_old*wkm2-ykm2))) ); 
    // yi = yi - ( cd1*yd_old + cd2*(c*wkm1-ykm1-(c*wkm2-ykm2) )); %treti clen prictu na konci cyklu

    // "old" budou soucasne, pri pristim volani PID_new_params uz budou old
    pid->Kp_old = pid->Kp;
    pid->Kd_old = pid->Kd;
    pid->b_old = pid->b;
    pid->c_old = pid->c;
    pid->Tf_old = pid->Tf;

    // vypocet diskretnich parametru
    pid->ci = pid->Ki * pid->Ts / 2.0;
    pid->cd1 = -(pid->Ts - 2*pid->Tf) / (pid->Ts + 2*pid->Tf);
    pid->cd2 = 2*pid->Kd / (pid->Ts + 2*pid->Tf);
}


float PID_loop(PID_t *pid, float y, float w)
{
    // regulacni odchylka pro I, P, D
    float e = w - y - pid->tv;
    float ep = pid->b*w - y;
    float ed = pid->c*w - y;

    pid->yi += pid->ci*(e+pid->ekm1);

    pid->yd = pid->cd1*pid->yd + pid->cd2*(ed-pid->edm1);
    float yp = pid->Kp*ep;

    float u_nosat = yp+pid->yi+pid->yd;
    float u_sat = u_nosat;

    if (u_nosat > pid->umax)
    {
        u_sat = pid->umax;
    }
    else if (u_nosat < -pid->umax)
    {
        u_sat = -pid->umax;
    }

    pid->tv = (u_nosat - u_sat) / pid->Tt;


    // TODO zkontrolovat, bezraz

    // ulozime stare hodnoty
    pid->ekm1 = e;
    pid->edm1 = ed;

    return u_sat;
}


// TODO write tests for pid
