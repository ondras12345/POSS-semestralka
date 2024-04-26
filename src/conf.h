#pragma once
#include <stdint.h>
#include <Arduino.h>

#define CONF_ITEMS(X) \
    X(uint8_t, base_speed, 50) \
    X(float, Kp, 0.5) \
    X(float, Ki, 0.0)

#define X_STRUCT(type, name, default) type name;
typedef struct {
    CONF_ITEMS(X_STRUCT)
} conf_t;
#undef X_STRUCT

extern conf_t conf;

void conf_init();
void conf_print(Print *response, conf_t c);
