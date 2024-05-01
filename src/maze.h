#pragma once
#include <stdint.h>
#include "line_follower.h"

typedef struct {
    /// Type of crossroad
    crossroad_t crossroad;
    /// Which direction to take / which direction was taken
    crossroad_direction_t direction;
    /// Distance from last node
    uint16_t distance_mm;
} maze_route_node_t;


typedef struct {
    maze_route_node_t stack[25];
    uint8_t top;
} maze_route_t;


maze_route_node_t maze_route_pop(maze_route_t *);
void maze_route_push(maze_route_t *, maze_route_node_t);
