#include "maze.h"

maze_route_node_t maze_route_pop(maze_route_t *route)
{
    if (route->top == 0)
    {
        // invalid
        return { cr_0, crd_straight, 0 };
    }
    return route->stack[--route->top];
}


void maze_route_push(maze_route_t *route, maze_route_node_t node)
{
    if (route->top >= sizeof(((maze_route_t){}).stack)/sizeof(maze_route_node_t))
        return;
    route->stack[route->top++] = node;
}
