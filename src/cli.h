#pragma once
#include <Arduino.h>
#include "maze.h"

void cli_init();
void cli_loop();
void print_maze_node(maze_route_node_t node, Print *response);
