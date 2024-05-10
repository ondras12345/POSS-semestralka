#include <unity.h>
#include <ArduinoFake.h>  // needed for Print (debug.h)
using namespace fakeit;

#include "maze.cpp"
#include "conf.cpp"
#include "encoder.cpp"
#include "debug.cpp"
#include "crossroad.cpp"
#include "line_follower.cpp"

// mocks
encoder_position_t pos;
encoder_position_t encoder_position()
{
    return pos;
}
int16_t speed_left, speed_right;
void motor_move_lin(int16_t L, int16_t R)
{
    speed_left = L;
    speed_right = R;
}
int16_t line_offset;
int16_t line_follower_offset()
{
    return line_offset;
}
uint8_t line_state;
uint8_t line_follower_state()
{
    return line_state;
}
void turn_turn_relative(float angle, bool expect_line)
{
    return;
}
bool turn_status()
{
    return false;
}
void error_code(error_code_t code)
{
    return;
}
void set_emergency()
{
    return;
}


void setUp()
{
    ArduinoFakeReset();
    pos = {0, 0};
    speed_left = 0;
    speed_right = 0;
    line_offset = 0;
    line_state = 0x00;
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
