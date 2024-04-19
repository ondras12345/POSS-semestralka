#include "robot.h"
#include <stdint.h>
#include <string.h>


#define X_STR(name, value) #name,
/**
 * Array with strings corresponding to robot_state_t values.
 * Terminated by nullptr.
 * Useful for performing reverse lookup.
 */
const char * robot_states[] = {
    ROBOT_STATES(X_STR)
    nullptr
};
#undef X_STR


bool robot_set_state(const char * str)
{
    for (uint8_t i = 0; robot_states[i] != nullptr; i++)
    {
        if (strcmp(str, robot_states[i]) == 0)
        {
            robot_state = (robot_state_t)i;
            return true;
        }
    }
    return false;
}


robot_state_t robot_state = s_emergency;
