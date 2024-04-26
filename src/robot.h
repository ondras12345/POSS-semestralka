#pragma once

// state numbers must start from 0 and be consecutive
#define ROBOT_STATES(X) \
    X(emergency, 0) \
    X(idle, 1) \
    X(line_follow, 2) \
    X(line_following, 3) \
    X(stop, 4)

#define X_ENUM(name, value) s_##name = value,
typedef enum {
    ROBOT_STATES(X_ENUM)
} robot_state_t;
#undef X_ENUM

extern const char * robot_states[];

bool robot_set_state(const char * str);

extern robot_state_t robot_state;
