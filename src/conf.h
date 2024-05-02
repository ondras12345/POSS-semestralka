#pragma once
#include <stdint.h>
#include <Arduino.h>

#define CONF_ITEMS(X) \
    X(uint8_t, base_speed, 50) \
    X(float, line_Kp, 0.4) \
    X(uint8_t, line_umax, 50) \
    /* turn, TODO default values */ \
    X(float, turn_Kp, 0.0) \
    X(float, turn_Ki, 0.0) \
    X(float, turn_Kd, 0.0) \
    /* doporucuje se volit Tf = Td/3 ... Td/20 */ \
    X(float, turn_Tf, 0.5) \
    /* doporucuje se 0.5*Ti (PI) nebo sqrt(Ti*Td) (PID) */ \
    X(float, turn_Tt, 0.5)

#define X_STRUCT(type, name, default) type name;
typedef struct {
    CONF_ITEMS(X_STRUCT)
} conf_t;
#undef X_STRUCT

extern conf_t conf;

void conf_init();
void conf_print(Print *response, conf_t c);
