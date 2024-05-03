#pragma once

#include <Print.h>

#define DEBUGGERS(X) \
    X(crossroad) \
    X(encoder) \
    X(maze_follow)

// All of the following Print pointers are guaranteed to not be nullptr.
extern Print * DEBUG_null;

#define X_extern(name) extern Print * DEBUG_##name;
DEBUGGERS(X_extern)
#undef X_extern
