#include <unity.h>

#include "maze.cpp"

bool compare_nodes(maze_route_node_t a, maze_route_node_t b)
{
    return a.crossroad == b.crossroad && a.direction == b.direction && a.distance_mm == b.distance_mm;
}


void test_maze_stack()
{
    maze_route_t route;
    maze_route_init(&route);
    maze_route_node_t node;

    // nothing left on stack
    node = maze_route_pop(&route);
    TEST_ASSERT_EQUAL_UINT8(0, node.distance_mm);
    TEST_ASSERT_EQUAL_UINT(cr_0, node.crossroad);

    maze_route_node_t node1 = { cr_X, crd_left, 1500 };
    maze_route_node_t node2 = { cr_X, crd_straight, 200 };

    maze_route_push(&route, node1);
    maze_route_push(&route, node2);
    node = maze_route_pop(&route);
    TEST_ASSERT_TRUE(compare_nodes(node, node2));
    TEST_ASSERT_FALSE(compare_nodes(node, node1));
    node = maze_route_pop(&route);
    TEST_ASSERT_TRUE(compare_nodes(node, node1));

    node = maze_route_pop(&route);
    TEST_ASSERT_EQUAL_UINT8(0, node.distance_mm);
    TEST_ASSERT_EQUAL_UINT(cr_0, node.crossroad);
}


int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_maze_stack);
    return UNITY_END();
}


int main(void)
{
    return runUnityTests();
}
