#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
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

    // stare vstupy, interni stav
    float wkm1, ykm1, wkm2, ykm2;
    float tv, tvm1;
    float yi;  // interni stav I
    float yd;  // interni stav D
    float yd_old;
} PID_t;


void PID_init(PID_t *, float Ts);
void PID_new_params(PID_t *);
float PID_loop(PID_t *, float y, float w);


#ifdef __cplusplus
}
#endif
