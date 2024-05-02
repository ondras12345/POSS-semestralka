#include <unity.h>

#include "maze.cpp"


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

    node = maze_route_peek(&route);
    TEST_ASSERT_EQUAL_CHAR(node2.crossroad, node.crossroad);
    TEST_ASSERT_EQUAL_CHAR(node2.direction, node.direction);
    TEST_ASSERT_EQUAL_UINT16(node2.distance_mm, node.distance_mm);

    node = maze_route_pop(&route);
    TEST_ASSERT_EQUAL_CHAR(node2.crossroad, node.crossroad);
    TEST_ASSERT_EQUAL_CHAR(node2.direction, node.direction);
    TEST_ASSERT_EQUAL_UINT16(node2.distance_mm, node.distance_mm);
    TEST_ASSERT_FALSE(node.direction == node1.direction);

    node = maze_route_pop(&route);
    TEST_ASSERT_EQUAL_CHAR(node1.crossroad, node.crossroad);
    TEST_ASSERT_EQUAL_CHAR(node1.direction, node.direction);
    TEST_ASSERT_EQUAL_UINT16(node1.distance_mm, node.distance_mm);

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
