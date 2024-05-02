#pragma once
#include <stdint.h>
#include "crossroad.h"

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


void maze_route_init(maze_route_t *);
maze_route_node_t maze_route_pop(maze_route_t *);
void maze_route_push(maze_route_t *, maze_route_node_t);
void maze_route_clone(maze_route_t *clone, maze_route_t *old);
void maze_route_invert(maze_route_t *route);

extern maze_route_t maze_route_current;
