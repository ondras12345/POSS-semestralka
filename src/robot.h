#pragma once

typedef enum {
    s_boot = 0,
    s_line_follow = 1,
    s_MAX, // last state, do not use
} robot_state_t;

extern robot_state_t robot_state;
