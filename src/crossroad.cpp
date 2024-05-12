#include "crossroad.h"

bool crossroad_direction_valid(crossroad_t cr, crossroad_direction_t crd)
{
    switch (crd)
    {
        case crd_left:
            return cr == cr_7 || cr == cr_T || cr == cr_3 || cr == cr_X;

        case crd_straight:
            return cr == cr_E || cr == cr_3 || cr == cr_X;

        case crd_right:
            return cr == cr_G || cr == cr_T || cr == cr_E || cr == cr_X;

        case crd_back:
        case crd_invalid:
            break;
    }
    return false;
}


/**
 * Return crossroad that will be seen when coming back from direction crd.
 */
crossroad_t crossroad_rotate(crossroad_t cr, crossroad_direction_t crd)
{
    switch (cr)
    {
        case cr_G:
            if (crd == crd_right) return cr_7;
            break;
        case cr_7:
            if (crd == crd_left) return cr_G;
            break;
        case cr_T:
            if (crd == crd_left) return cr_E;
            if (crd == crd_right) return cr_3;
            break;
        case cr_E:
            if (crd == crd_right) return cr_T;
            if (crd == crd_straight) return cr_3;
            break;
        case cr_3:
            if (crd == crd_left) return cr_T;
            if (crd == crd_straight) return cr_E;
            break;
        case cr_X:
            return cr_X;

        // these inputs are not expected:
        case cr_I:
        case cr_F:
            return cr;
        case cr_0:
        case cr_i:
            return cr_0;
    }

    // invalid input
    return cr_0;
}


crossroad_direction_t crossroad_direction_rotate(crossroad_direction_t rotate_me, crossroad_direction_t from)
{
    if (rotate_me == from) return crd_invalid;
    switch (rotate_me)
    {
        case crd_left:
            if (from == crd_straight) return crd_right;
            if (from == crd_right) return crd_straight;
            break;

        case crd_straight:
            if (from == crd_left) return crd_left;
            if (from == crd_right) return crd_right;
            break;

        case crd_right:
            if (from == crd_left) return crd_straight;
            if (from == crd_straight) return crd_left;

        // crd_back is special, it is used during backtracking
        case crd_back:
            if (from == crd_left) return crd_right;
            if (from == crd_straight) return crd_straight;
            if (from == crd_right) return crd_left;

        case crd_invalid:
            break;
    }
    return crd_invalid;
}
