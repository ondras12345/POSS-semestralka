#include "turn.h"
#include "imu.h"
#include <PID.h>

static bool turning = false;
static float target = 0.0;
static PID_t pid;

#define Ts 10UL

void turn_init()
{
}


void turn_loop(unsigned long now)
{
    // TODO check emergency
}


void turn_turn_relative(float angle)
{
    // TODO check emergency
}


bool turn_status()
{
}
