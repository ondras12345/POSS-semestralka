#pragma once

// state numbers must start from 0 and be consecutive
#define ROBOT_STATES(X) \
    X(s_emergency, 0) \
    X(s_idle, 1) \
    X(s_line_follow, 2) \
    X(s_line_following, 3) \
    X(s_stop, 4)

#define X_ENUM(name, value) name = value,
typedef enum {
    ROBOT_STATES(X_ENUM)
} robot_state_t;
#undef X_ENUM

extern const char * robot_states[];

bool robot_set_state(const char * str);

extern robot_state_t robot_state;
