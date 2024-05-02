#include "maze.h"

maze_route_t maze_route_current;  // initialized in main


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
