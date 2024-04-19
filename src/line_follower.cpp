#include "line_follower.h"
#include <MeRGBLineFollower.h>

MeRGBLineFollower RGBLineFollower(PORT_9);


void line_follower_init()
{
    RGBLineFollower.begin();
    RGBLineFollower.setKp(1);
}


void line_follower_loop()
{
    // dela to jen kazdych 8 ms
    RGBLineFollower.loop();
}


int16_t line_follower_offset()
{
    return RGBLineFollower.getPositionOffset();
}

uint8_t line_follower_state()
{
    return RGBLineFollower.getPositionState();
}
