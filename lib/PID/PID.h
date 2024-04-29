#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // snare parametry pro bezrazove prepnuti
    float Kp_old;
    float b_old;
    float Kd_old;
    float Tf_old;
    float c_old;
    float yd_old;
    // parametry
    float Kp, Ki, Kd, b, c, Tt, Tf;
    // Tt ... casova konstanta vysledovani
    // Tf ... casova konstanta filtru D
    float umax;
    float Ts;
    // diskretni parametry
    float ci;
    float cd1;
    float cd2;
    // stare vstupy
    float wkm1, ykm1;
    float tv;
    float ekm1;  // posledni vstup I
    float edm1;  // posledni vstup D
    // TODO
    float yi;  // interni stav I
    float yd;  // interni stav D
} PID_t;


void PID_init(PID_t *, float Ts);
void PID_new_params(PID_t *);
float PID_loop(PID_t *, float y, float w);


#ifdef __cplusplus
}
#endif
