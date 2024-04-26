#pragma once
#include <stdint.h>
#include <Arduino.h>

typedef struct {
    uint8_t base_speed;
    float Kp, Ki; //, Kd, Tt, Tf;
} conf_t;

extern conf_t conf;

void conf_init();
void conf_print(Print *response, conf_t c);
