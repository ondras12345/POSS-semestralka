#include <unity.h>
#include <unistd.h>

/**
 * Map data file specification:
 * - line following / crossroad detection is done based off of blue channel
 *   (blue==0 line, blue!=0 no line)
 * - red and green channels are used for extra info
 * - start point should have red=255, green=0
 * - finish area should have red=0, green=255
 */


#include "debug.cpp"
char log_buf[200];
size_t log_i;
class TestPrint : public Print {
    public:
        size_t write(uint8_t c)
        {
            //putchar(c);
            if (c != '\r') log_buf[log_i++] = c;
            if (c == '\n')
            {
                log_buf[log_i++] = '#';
                log_buf[log_i++] = ' ';
            }
            log_buf[log_i] = '\0';
            return 1;
        }
};
TestPrint tp = TestPrint();

#define F(s) s
#define Serial tp
// Arduino.h
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

#include "maze.cpp"
#include "conf.cpp"
#include "encoder.cpp"
#include "crossroad.cpp"
#include "line_follower.cpp"

// position on map in pixels
typedef struct
{
    uint16_t x;
    uint16_t y;
} pos2_t;

typedef struct
{
    int16_t x;
    int16_t y;
} vec2_t;

vec2_t vec2_scale(vec2_t v, int16_t s)
{
    v.x *= s;
    v.y *= s;
    return v;
}

vec2_t vec2_rotate90(vec2_t v, bool right = false)
{
    vec2_t r;
    // our y axis is inverted (down is positive)
    if (right)
    {
        r.x = -v.y;
        r.y = +v.x;
    }
    else
    {
        r.x = +v.y;
        r.y = -v.x;
    }
    return r;
}
pos2_t pos2_vec2_add(pos2_t p, vec2_t v)
{
    return {(uint16_t)(p.x+v.x), (uint16_t)(p.y+v.y)};
}

pos2_t map_pos;
pos2_t start_pos;
vec2_t orientation_vec;

// mocks
encoder_position_t pos;
encoder_position_t encoder_position()
{
    return pos;
}
int16_t speed_left, speed_right;
void motor_move_lin(int16_t L, int16_t R)
{
    tp.print("motor_move_lin ");
    tp.print(L);
    tp.print(' ');
    tp.println(R);
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
    tp.print("turn_turn_relative ");
    tp.print(angle);
    tp.print(' ');
    tp.println(expect_line);

    // otoceni meni i pozici na mape
    map_pos = pos2_vec2_add(map_pos, vec2_scale(orientation_vec, -2));

    uint32_t N_pulses = 200 / conf.mm_per_pulse;
    if (angle == conf.turn_target)
    {
        // doprava
        orientation_vec = vec2_rotate90(orientation_vec, true);
        pos.left += N_pulses;
        pos.right -= N_pulses;
    }
    else if (angle == -conf.turn_target)
    {
        // doleva
        orientation_vec = vec2_rotate90(orientation_vec, false);
        pos.left -= N_pulses;
        pos.right += N_pulses;
    }
    else if (angle == 180 || angle == -180)
    {
        orientation_vec = vec2_scale(orientation_vec, -1);
        pos.left -= 2*N_pulses;
        pos.right += 2*N_pulses;
    }
    else
    {
        TEST_FAIL_MESSAGE("unexpected angle for turn_turn_relative");
    }

    map_pos = pos2_vec2_add(map_pos, vec2_scale(orientation_vec, +2));
}
bool turn_status()
{
    return false;
}
void error_code(error_code_t code)
{
    return;
}
bool emergency;
void set_emergency()
{
    tp.println("set_emergency");
    emergency = true;
    return;
}

#define MAP_WIDTH 344
#define MAP_HEIGHT 263
uint8_t map_lines[MAP_WIDTH][MAP_HEIGHT];
bool map_lines_get(pos2_t pos)
{
    return map_lines[pos.x][pos.y] & 0x01;
}
bool map_lines_finish(pos2_t pos)
{
    return map_lines[pos.x][pos.y] & 0x02;
}


FILE * fo;

void setUp()
{
    fo = NULL;
    log_i = 0;
    log_buf[0] = '\0';
#define X_TestPrint(name) DEBUG_##name = &tp;
    DEBUGGERS(X_TestPrint)
    pos = {0, 0};
    speed_left = 0;
    speed_right = 0;
    line_offset = 0;
    line_state = 0x00;
    emergency = false;

    FILE * fm = fopen("maze.data", "rb");
    TEST_ASSERT_NOT_NULL(fm);
    for (size_t y = 0; y < MAP_HEIGHT; y++)
    {
        for (size_t x = 0; x < MAP_WIDTH; x++)
        {
            uint8_t b[3];  // RGB
            TEST_ASSERT_EQUAL_size_t(1, fread(&b, sizeof b, 1, fm));
            map_lines[x][y] = b[2] ? 0x01 : 0x0;  // B
            if (b[0] == 255 && b[1] == 0)
            {
                // red
                start_pos = {(uint16_t)x, (uint16_t)y};
            }
            // finish
            else if (b[0] == 0 && b[1] == 255)
            {
                map_lines[x][y] |= 0x02;
            }
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(fgetc(fm) == EOF, "did not read all of fm");
    fclose(fm);

    //printf("start_pos: %u,%u\n", start_pos.x, start_pos.y);
    //FILE * fo = fopen("maze_dump.txt", "w");
    //TEST_ASSERT_NOT_NULL(fo);
    //// dump map
    //for (size_t y = 0; y < MAP_HEIGHT; y++)
    //{
    //    for (size_t x = 0; x < MAP_WIDTH; x++)
    //    {
    //        putc(map_lines_get({x, y}) ? ' ' : '+', fo);
    //    }
    //    putc('\n', fo);
    //}
    //fclose(fo);

    map_pos = start_pos;

    orientation_vec = {1, 0}; // ->
    conf_init();
}


void tearDown()
{
    if (fo != NULL) fclose(fo);
}


void test_utils()
{
    TEST_ASSERT_EQUAL_INT(1, orientation_vec.x);
    TEST_ASSERT_EQUAL_INT(0, orientation_vec.y);
    vec2_t r = vec2_rotate90(orientation_vec, false); // left
    TEST_ASSERT_EQUAL_INT(0, r.x);
    TEST_ASSERT_EQUAL_INT(-1, r.y);
    r = vec2_rotate90(r, false); // left
    TEST_ASSERT_EQUAL_INT(-1, r.x);
    TEST_ASSERT_EQUAL_INT(0, r.y);

    r = vec2_rotate90(orientation_vec, true); // right
    TEST_ASSERT_EQUAL_INT(0, r.x);
    TEST_ASSERT_EQUAL_INT(1, r.y);
    r = vec2_rotate90(r, true); // right
    TEST_ASSERT_EQUAL_INT(-1, r.x);
    TEST_ASSERT_EQUAL_INT(0, r.y);
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


void fo_header()
{
    fprintf(fo, "k\tmap_pos_x\tmap_pos_y\torientation_x\torientation_y\tline_state\tcrossroad\tlast_crossroad\tencoder_pos_left\tencoder_pos_right\tspeed_left\tspeed_right\n");
}


void fo_line(size_t k)
{
    fprintf(fo, "# %s\n", log_buf);
    log_i = 0;
    log_buf[0] = '\0';
    fprintf(
        fo,
        "%zu\t%u\t%u\t%i\t%i\t0x%hhx\t%c\t%c\t%u\t%u\t%d\t%d\n",
        k, map_pos.x, map_pos.y, orientation_vec.x, orientation_vec.y, line_state, line_follower_crossroad(), line_follower_last_crossroad(), pos.left, pos.right, speed_left, speed_right
    );
}

void step_mocks(size_t k)
{
    if (speed_right > 0 && speed_left > 0)
    {
        if (k % 10 == 0)
        {
            map_pos = pos2_vec2_add(map_pos, orientation_vec);
            TEST_ASSERT_GREATER_THAN_UINT(0, map_pos.x);
            TEST_ASSERT_GREATER_THAN_UINT(0, map_pos.y);
            TEST_ASSERT_LESS_THAN_UINT(MAP_WIDTH, map_pos.x);
            TEST_ASSERT_LESS_THAN_UINT(MAP_HEIGHT, map_pos.y);
        }
        // encoder position
        // map pixels are 10x10mm
        // this is executed 10x per pixel
        uint32_t dist_pulses = 10/10 / conf.mm_per_pulse;
        pos.left += dist_pulses;
        pos.right += dist_pulses;
    }
    else if (speed_right < 0 && speed_left < 0)
    {
        TEST_FAIL_MESSAGE("reverse movement");
    }
    else if (speed_right == 0 && speed_left == 0)
    {
        // that's fine
    }
    else
    {
        TEST_FAIL_MESSAGE("unexpected turn");
    }

    line_state = (
        (map_lines_get(map_pos) ? 0b0110 : 0)
        | (map_lines_get(pos2_vec2_add(map_pos, vec2_rotate90(orientation_vec, true))) ? 0b1000 : 0)
        | (map_lines_get(pos2_vec2_add(map_pos, vec2_rotate90(orientation_vec, false))) ? 0b0001 : 0)
    );
}


void test_maze_follow()
{
    TEST_ASSERT_FALSE(map_lines_finish(map_pos));

    fo = fopen("maze_follow.csv", "w");
    TEST_ASSERT_NOT_NULL(fo);
    fo_header();

    maze_init();
    maze_route_push(&maze_route_current, {cr_X, crd_straight, 100});
    maze_route_push(&maze_route_current, {cr_3, crd_straight, 300});
    maze_route_push(&maze_route_current, {cr_X, crd_left, 300});
    maze_route_push(&maze_route_current, {cr_G, crd_right, 300});
    maze_route_push(&maze_route_current, {cr_7, crd_left, 300});
    maze_route_push(&maze_route_current, {cr_G, crd_right, 600});
    maze_route_push(&maze_route_current, {cr_E, crd_right, 1200});
    maze_route_push(&maze_route_current, {cr_X, crd_straight, 1200});
    maze_route_push(&maze_route_current, {cr_3, crd_left, 300});
    maze_route_push(&maze_route_current, {cr_3, crd_left, 600});
    maze_route_push(&maze_route_current, {cr_X, crd_right, 1200});
    maze_route_push(&maze_route_current, {cr_F, crd_straight, 300});

    // limit max number of iterations to prevent endless loop in case of bug
    for (size_t k = 0; k < 10000U; k++)
    {
        tp.print("k=");
        tp.println(k);

        step_mocks(k);

        if ((line_state & 0b0110) != 0b0000)
        {
            // TODO make this less sensitive
            //TEST_FAIL_MESSAGE("left line");
        }

        if (k == 10) maze_follow();  // wait for crossroad to stabilize before starting

        unsigned long now = k * 10UL;
        line_follower_loop(now);
        maze_loop(now);

        fo_line(k);

        TEST_ASSERT_FALSE_MESSAGE(emergency, "emergency mode");
        if (k >= 10 && !maze_following())
        {
            // jsme v cili?
            char buf[80];
            snprintf(buf, sizeof buf, "not at finish x=%u y=%u", map_pos.x, map_pos.y);
            TEST_ASSERT_TRUE_MESSAGE(map_lines_finish(map_pos), buf);
            return;
        }
    }
    TEST_FAIL_MESSAGE("too many iterations");
}


void test_maze_map()
{
    TEST_ASSERT_FALSE(map_lines_finish(map_pos));

    fo = fopen("maze_map.csv", "w");
    TEST_ASSERT_NOT_NULL(fo);
    fo_header();

    maze_init();

    // limit max number of iterations to prevent endless loop in case of bug
    for (size_t k = 0; k < 20000U; k++)
    {
        tp.print("k=");
        tp.println(k);

        step_mocks(k);

        if ((line_state & 0b0110) != 0b0000)
        {
            // TODO make this less sensitive
            //TEST_FAIL_MESSAGE("left line");
        }

        if (k == 10) maze_map();  // wait for crossroad to stabilize before starting

        unsigned long now = k * 10UL;
        line_follower_loop(now);
        maze_loop(now);

        fo_line(k);

        TEST_ASSERT_FALSE_MESSAGE(emergency, "emergency mode");
        if (k >= 10 && !maze_mapping())
        {
            // jsme v cili?
            char buf[80];
            snprintf(buf, sizeof buf, "not at finish x=%u y=%u", map_pos.x, map_pos.y);
            TEST_ASSERT_TRUE_MESSAGE(map_lines_finish(map_pos), buf);
            return;
        }
    }
    TEST_FAIL_MESSAGE("too many iterations");
}


int runUnityTests(void)
{
    if (chdir("test/test_maze"))
    {
        fprintf(stderr, "failed to chdir");
        return 1;
    }

    UNITY_BEGIN();
    RUN_TEST(test_utils);
    RUN_TEST(test_maze_stack);
    RUN_TEST(test_maze_follow);
    RUN_TEST(test_maze_map);

    fprintf(stderr, "gnuplot result: %d\n", system("./maze_follow.gpi"));

    return UNITY_END();
}


int main(void)
{
    return runUnityTests();
}
