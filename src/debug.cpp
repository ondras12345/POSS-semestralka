#include "debug.h"


class NullPrint : public Print {
    public:
        size_t write(uint8_t c)
        {
            return 1;
        }

        size_t write(const uint8_t *data, size_t size)
        {
            return size;
        }
};


static NullPrint null = NullPrint();

Print * DEBUG_null = &null;

#define X_def(name) Print * DEBUG_##name = DEBUG_null;
DEBUGGERS(X_def)


void print_maze_node(maze_route_node_t node, Print *response)
{
    response->write(node.crossroad);
    response->print('\t');
    response->write(node.direction);
    response->print('\t');
    response->println(node.distance_mm);
}
