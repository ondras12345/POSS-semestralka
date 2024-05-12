#include "maze.h"
#include "encoder.h"
#include "debug.h"
#include "line_follower.h"
#include "conf.h"
#include "error.h"
#include "turn.h"


void maze_route_init(maze_route_t *route)
{
    route->top = 0;
}

maze_route_node_t maze_route_pop(maze_route_t *route)
{
    if (route->top == 0)
    {
        // invalid
        return { cr_0, crd_straight, 0 };
    }
    return route->stack[--route->top];
}


maze_route_node_t maze_route_peek(maze_route_t *route)
{
    if (route->top == 0)
    {
        // invalid
        return { cr_0, crd_straight, 0 };
    }
    return route->stack[route->top-1];
}


void maze_route_push(maze_route_t *route, maze_route_node_t node)
{
    if (route->top >= sizeof(((maze_route_t){}).stack)/sizeof(maze_route_node_t))
        return;
    route->stack[route->top++] = node;
}


void maze_route_clone(maze_route_t *clone, maze_route_t *old)
{
    clone->top = old->top;
    for (uint8_t i = 0; i < old->top; i++)
        clone->stack[i] = old->stack[i];
}


// mapping / backtracking
typedef enum {
    ms_idle,
    ms_following,
    ms_following_turning,
    ms_mapping,
    ms_mapping_turning,
} state_t;

static state_t state;
maze_route_t maze_route_current;
static maze_route_t route_follow_route;
static uint8_t route_follow_index;
static bool map_backtracking;
static encoder_position_t map_prev_cr_pos;
static const crossroad_direction_t map_dir_order[] = {
    // TODO optimize
    crd_left,
    crd_right,
    crd_straight,
};


/**
 * Given a crossroad and optionally the previous direction, return the next
 * direction from map_dir_order.
 * Return value crd_invalid means all options have already been tried.
 */
crossroad_direction_t map_dir_next(crossroad_t cr, crossroad_direction_t crd=crd_invalid)
{
    bool found_start = false;
    if (crd == crd_invalid) found_start = true;
    for (uint8_t i = 0; i < sizeof(map_dir_order)/sizeof(map_dir_order[0]); i++)
    {
        crossroad_direction_t crd_i = map_dir_order[i];
        if (found_start && crossroad_direction_valid(cr, crd_i))
        {
            return crd_i;
        }
        if (crd_i == crd) found_start = true;
    }
    // no solution found
    return crd_invalid;
}


/**
 * Initiate turn_turn_relative from given crossroad direction
 */
bool turn_from_direction(crossroad_direction_t crd)
{
    switch (crd)
    {
        case crd_back:  // invalid input
        case crd_invalid:  // invalid input
        case crd_straight:  // no need to turn
            return false;

        case crd_left:
        case crd_right:
            line_follower_stop();
            float angle = conf.turn_target;
            if (crd == crd_left) angle *= -1;
            turn_turn_relative(angle, true);
            Serial.print(F("[D] turning "));
            Serial.println(angle);
            return true;
    }
    return false;  // this should never happen
}


void maze_init()
{
    maze_route_init(&maze_route_current);
}


void maze_loop(unsigned long now)
{
    const encoder_position_t pos = encoder_position();

    switch (state)
    {
        case ms_idle:
            break;

        case ms_following:
        {
            if (!line_follower_following())
            {
                Serial.println(F("[E] should be following"));
                error_code(e_should_be_following);
                set_emergency();
                break;
            }

            maze_route_node_t node = route_follow_route.stack[route_follow_index];

            int32_t dist = encoder_distance_mm(line_follower_last_crossroad_position(), pos);
            line_follower_follow(
                ((dist >= conf.fast_offset_mm || route_follow_route.stack[route_follow_index-1].direction == crd_straight) &&
                 (dist <= node.distance_mm - conf.fast_offset_mm || node.direction == crd_straight)
                )
                ? conf.fast_speed
                : conf.base_speed
            );

            if (line_follower_last_crossroad_updated())
            {
                crossroad_t cr = line_follower_last_crossroad();

                if (cr == cr_0)
                {
                    Serial.println(F("[E] cr_0"));
                    error_code(e_cr_0);
                    set_emergency();
                    break;
                }

                if (cr == node.crossroad)
                {
                    DEBUG_maze_follow->println(F("[D] expected last crossroad"));
                    if (route_follow_index < route_follow_route.top-1)
                    {
                        route_follow_index++;
                    }
                    else
                    {
                        // finish
                        state = ms_idle;
                        DEBUG_maze_follow->println("[D] finish");
                        line_follower_stop();
                        break;
                    }

                    if (turn_from_direction(node.direction))
                    {
                        state = ms_following_turning;
                    }
                }
                else
                {
                    Serial.print(F("[W] unexpected crossroad: "));
                    Serial.write(cr);
                    Serial.println();
                    error_code(e_unexpected_crossroad);
                }
            }
        }
            break;

        case ms_following_turning:
            if (!turn_status())
            {
                // finished turning
                line_follower_follow(conf.base_speed);
                state = ms_following;
                line_follower_clear();
                DEBUG_maze_follow->println(F("[D] finished turning"));
            }
            break;

        case ms_mapping:
            if (!line_follower_following())
            {
                Serial.println(F("[E] should be following"));
                error_code(e_should_be_following);
                set_emergency();
                break;
            }

            if (line_follower_last_crossroad_updated())
            {
                crossroad_t cr = line_follower_last_crossroad();
                if (cr == cr_0)
                {
                    Serial.println(F("[E] cr_0"));
                    error_code(e_cr_0);
                    set_emergency();
                    break;
                }

                if (map_backtracking)
                {
                    maze_route_node_t node = maze_route_pop(&maze_route_current);
                    if (crossroad_rotate(node.crossroad, node.direction) != cr)
                    {
                        Serial.println(F("[E] e_unexpected_crossroad"));
                        error_code(e_unexpected_crossroad);
                        set_emergency();
                        break;
                    }

                    crossroad_direction_t prev_dir = node.direction;
                    crossroad_direction_t next_dir = map_dir_next(node.crossroad, prev_dir);
                    if (next_dir == crd_invalid)
                    {
                        // all directions have been tried, return to previous crossroad
                        crossroad_direction_t next_dir_rotated = crossroad_direction_rotate(crd_back, prev_dir);
                        // No maze_route_push, we are removing this crossroad from
                        // the stack.
                        if (turn_from_direction(next_dir_rotated))
                        {
                            state = ms_mapping_turning;
                        }
                    }
                    else
                    {
                        // try next direction
                        map_backtracking = false;
                        DEBUG_map->println(F("map_backtracking=false"));
                        node.direction = next_dir;
                        maze_route_push(&maze_route_current, node);
                        crossroad_direction_t next_dir_rotated = crossroad_direction_rotate(next_dir, prev_dir);
                        if (turn_from_direction(next_dir_rotated))
                        {
                            state = ms_mapping_turning;
                        }
                    }
                }
                else
                {
                    if (cr == cr_I) break;

                    if (cr == cr_i)
                    {
                        // dead end, turn 180 degrees
                        map_backtracking = true;
                        DEBUG_map->println(F("map_backtracking=true"));
                        line_follower_stop();
                        turn_turn_relative(180, true);
                        state = ms_mapping_turning;
                        DEBUG_map->println(F("[D] dead end, turning 180"));
                        break;
                    }

                    if (cr == cr_F)
                    {
                        line_follower_stop();
                        // finish
                        state = ms_idle;
                        DEBUG_map->println(F("[D] finish"));
                        break;
                    }

                    // new crossroad that we haven't seen yet
                    line_follower_stop();
                    maze_route_node_t node;
                    node.crossroad = cr;
                    encoder_position_t lpos = line_follower_last_crossroad_position();
                    node.distance_mm = encoder_distance_mm(map_prev_cr_pos, lpos);
                    map_prev_cr_pos = lpos;
                    node.direction = map_dir_next(cr);
                    if (node.direction == crd_invalid)
                    {
                        Serial.println(F("[E] no valid direction"));
                        error_code(e_no_valid_direction);
                        set_emergency();
                        break;
                    }
                    DEBUG_map->print(F("[D] found new crossroad: "));
                    print_maze_node(node, DEBUG_map);
                    maze_route_push(&maze_route_current, node);
                    if (turn_from_direction(node.direction))
                    {
                        state = ms_mapping_turning;
                    }
                }
            }
            break;

        case ms_mapping_turning:
            if (!turn_status())
            {
                // finished turning
                line_follower_follow(conf.map_speed);
                state = ms_mapping;
                line_follower_clear();
                DEBUG_map->println(F("[D] finished turning "));
            }
            break;
    }
}


void maze_follow()
{
    // start following maze_route_current
    maze_route_clone(&route_follow_route, &maze_route_current);
    route_follow_index = 0;
    line_follower_follow(conf.base_speed);
    line_follower_clear();
    state = ms_following;
    DEBUG_maze_follow->println(F("[D] starting maze_follow"));
}


bool maze_following()
{
    return state == ms_following || state == ms_following_turning;
}


void maze_map()
{
    map_backtracking = false;
    maze_route_init(&maze_route_current);  // clear route
    map_prev_cr_pos = encoder_position();
    line_follower_follow(conf.map_speed);
    line_follower_clear();
    state = ms_mapping;
}


bool maze_mapping()
{
    return state == ms_mapping || state == ms_mapping_turning;
}


/// Stop whatever you are doing (following / mapping)
void maze_stop()
{
    state = ms_idle;
    line_follower_stop();
}
