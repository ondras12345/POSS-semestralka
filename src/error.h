#pragma once

typedef enum {
    e_OK = 0,
    e_should_be_following = 1,
    e_unexpected_crossroad = 2,
    e_emergency = 3,
    e_no_valid_direction = 4,
    e_cr_0 = 5,
} error_code_t;

void error_code(error_code_t code);
void set_emergency();
bool get_emergency();
