#pragma once
#include <stdint.h>
#include <Arduino.h>

typedef struct {
    uint8_t test_uint;
    bool test_bool;
    float test_float;
} conf_t;

extern conf_t conf;

void conf_init();
void conf_print(Print *response, conf_t c);
