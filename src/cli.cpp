#include "cli.h"
#include <Arduino.h>
#include <Shellminator.hpp>
#include <Shellminator-IO.hpp>
#include <Commander-API.hpp>
#include <Commander-API-Commands.hpp>
#include <Commander-IO.hpp>
#include <perf_counter.h>
#include "encoder.h"
#include "motor.h"
#include "line_follower.h"
#include "imu.h"
#include "robot.h"
#include "conf.h"
#include "turn.h"
#include "debug.h"
#include "error.h"
#include "maze.h"

static Shellminator shell(&Serial);
static Commander commander;


static void cmnd_encoder(char *args, Stream *response)
{
    (void)args;
    response->print(F("encoder: L"));
    encoder_position_t pos = encoder_position();
    response->print(pos.left);
    response->print(F(" R"));
    response->println(pos.right);
}


static void cmnd_line(char *args, Stream *response)
{
    response->println(F("line:"));
    response->print(F("  offset: "));
    response->println(line_follower_offset());
    response->print(F("  state: "));
    response->println(line_follower_state(), BIN);
    response->print(F("  state_debounced: "));
    response->println(line_follower_state_debounced(), BIN);
    response->print(F("  crossroad: "));
    response->println((char)line_follower_crossroad());
    response->print(F("  last_crossroad: "));
    response->println((char)line_follower_last_crossroad());
    response->print(F("  last_crossroad_position: "));
    encoder_position_t pos = line_follower_last_crossroad_position();
    response->print(pos.left);
    response->print(F(", "));
    response->println(pos.right);
    response->print(F("  last_crossroad distance [mm]: "));
    response->println(encoder_distance_mm(pos, encoder_position()));
}


static void cmnd_motor_move(char *args, Stream *response)
{
    int speed_left, speed_right;
    char c; // there should be no extra char
    if (sscanf(args, " %i %i%c", &speed_left, &speed_right, &c) != 2)
    {
        response->print(F("bad count: "));
        response->println(args);
        goto usage;
    }
    if (abs(speed_left) > 255 || abs(speed_right) > 255)
    {
        response->println(F("out of range"));
        goto usage;
    }
    response->println(F("moving"));
    motor_move(speed_left, speed_right);
    return;


usage:
    response->println(F("usage: motor_move speed_left speed_right"));
}


static void cmnd_motor_move_lin(char *args, Stream *response)
{
    int speed_left, speed_right;
    char c; // there should be no extra char
    if (sscanf(args, " %i %i%c", &speed_left, &speed_right, &c) != 2)
    {
        response->print(F("bad count: "));
        response->println(args);
        goto usage;
    }
    if (abs(speed_left) > 255 || abs(speed_right) > 255)
    {
        response->println(F("out of range"));
        goto usage;
    }
    response->println(F("moving"));
    motor_move_lin(speed_left, speed_right);
    return;


usage:
    response->println(F("usage: motor_move_lin speed_left speed_right"));
}


static void cmnd_state(char *args, Stream *response)
{
    if (*args == ' ') args++;
    if (!*args)
    {
        // no arg, just print state
    }
    else if (!robot_set_state(args))
    {
        response->print(F("invalid state: "));
        response->println(args);
        response->print(F("available: "));
        for (size_t i = 0; robot_states[i] != nullptr; i++)
        {
            response->print(robot_states[i]);
            response->print(' ');
        }
        response->println();
        goto usage;
    }

    response->print(F("state: "));
    response->println(robot_states[robot_state]);
    response->print(F("emergency: "));
    response->println(get_emergency());
    return;

usage:
    response->println(F("usage: state [s_name]"));
}


static void cmnd_perf(char *args, Stream *response)
{
    extern perf_counter_t * perf_counters[];
    for (perf_counter_t ** counter_ptr = perf_counters; *counter_ptr; counter_ptr++)
    {
        perf_counter_t * counter = *counter_ptr;
        response->print(counter->name);
        response->print('\t');
        response->println(perf_counter_reset(counter));
    }
}


static void cmnd_conf(char *args, Stream *response)
{
    response->println(F("Usage: conf [name value]"));

    char * setting_name = strsep(&args, " ");
    const char * setting_value = args;
    if (setting_name == nullptr)  // TODO || *setting_name == '\0'
    {
        // this is ok, just print out config
    }
    //else if (strcmp(setting_name, "-s") == 0)
    //{
    //    settings_write(s);
    //}
    else if (setting_value == nullptr)
    {
        response->println(F("Missing value"));
    }

#define scanconf_uint8_t(name) \
    else if (strcmp(setting_name, #name) == 0) \
    { \
        unsigned int tmp; \
        sscanf(setting_value, "%u", &tmp); \
        if (tmp > 255) \
        { \
            response->println(F("Value not in range 0-255")); \
            goto bad; \
        } \
        conf.name = (uint8_t)tmp; \
    }

#define scanconf_bool(name) \
    else if (strcmp(setting_name, #name) == 0) \
    { \
        conf.name = (setting_value[0] == '1'); \
    }

#define scanconf_float(name) \
    else if (strcmp(setting_name, #name) == 0) \
    { \
        conf.name = atof(setting_value); \
    }

#define X_scanconf(type, name, default) scanconf_##type(name)

    CONF_ITEMS(X_scanconf)

    else
    {
        response->print(F("Invalid config option: "));
        response->println(setting_name);
    }

bad:
    response->println();
    response->println(F("configuration:"));
    conf_print(response, conf);
}


static void cmnd_imu(char *args, Stream *response)
{
    response->println(F("imu:"));
    response->print(F("  angle_X: "));
    response->println(imu_angle_X());
    response->print(F("  angle_Y: "));
    response->println(imu_angle_Y());
    response->print(F("  angle_Z: "));
    response->println(imu_angle_Z());
}


static void cmnd_turn(char *args, Stream *response)
{
    int angle;
    char c;
    if (sscanf(args, " %d%c", &angle, &c) == 1)
    {
        response->print(F("turning "));
        response->println(angle);
        turn_turn_relative(angle);
    }

    if (sscanf(args, " line %d%c", &angle, &c) == 1)
    {
        response->print(F("turning to line "));
        response->println(angle);
        turn_turn_relative(angle, true);
    }

    response->print(F("turning: "));
    response->println(turn_status());
    response->print(F("target: "));
    response->println(turn_target());
    response->print(F("angle_Z: "));
    response->println(imu_angle_Z());

    response->println(F("usage: turn [[line] target]"));
}


static void cmnd_debug(char *args, Stream *response)
{
    char * debugger_name = strsep(&args, " ");
    const char * debugger_value = args;

    if (debugger_name == nullptr || *debugger_name == '\0')
    {
        // this is ok, just print out config
    }
    else if (debugger_value == nullptr || *debugger_value == '\0')
    {
        response->println(F("missing value"));
        goto usage;
    }

#define X_debug_scan(name) \
    else if (strcmp(debugger_name, #name) == 0) \
    { \
        DEBUG_##name = (*debugger_value == '0') ? DEBUG_null : &Serial; \
    }

    DEBUGGERS(X_debug_scan)

    else
    {
        response->print(F("Unknown debugger: "));
        response->println(debugger_name);
        goto usage;
    }


#define X_debug_print(name) \
    response->print(F(#name ": ")); \
    response->println(DEBUG_##name != DEBUG_null);

    DEBUGGERS(X_debug_print)
    return;

usage:
    response->println(F("usage: debug [name (1|0)]"));
}


static void cmnd_maze_print(char *args, Stream *response)
{
    response->println(F("maze_route_current (bottom to top)"));
    response->println(F("\tcrossroad\tdirection\tdistance [mm]"));
    for (uint8_t i = 0; i < maze_route_current.top; i++)
    {
        response->print(F("maze_push "));
        print_maze_node(maze_route_current.stack[i], response);
    }
}


static void cmnd_maze_pop(char *args, Stream *response)
{
    maze_route_node_t node = maze_route_pop(&maze_route_current);
    print_maze_node(node, response);
}


static void cmnd_maze_push(char *args, Stream *response)
{
    char crossroad, direction, c;
    uint16_t distance_mm;
    if (sscanf(args, " %c %c %" SCNu16 "%c", &crossroad, &direction, &distance_mm, &c) != 3)
    {
        goto usage;
    }

    maze_route_node_t node;
    // no input checking
    node.crossroad = (crossroad_t)crossroad;

    switch (direction)
    {
        case 'L':
        case 'R':
        case 'I':
            node.direction = (crossroad_direction_t)direction;
            break;
        default:
            response->println(F("invalid direction"));
            goto usage;
    }
    node.distance_mm = distance_mm;

    maze_route_push(&maze_route_current, node);
    return;

usage:
    response->println(F("usage: maze_push crossrad direction distance_mm"));
}


static Commander::API_t API_tree[] = {
    // TODO PROGMEM
    apiElement("encoder",       "Read rotary encoders",         cmnd_encoder),
    apiElement("line",          "Read line follower",           cmnd_line),
    apiElement("motor_move",    "Set motor output (nonlinear)", cmnd_motor_move),
    apiElement("motor_move_lin","Set motor output (linearized)",cmnd_motor_move_lin),
    apiElement("state",         "Get/set state machine state",  cmnd_state),
    apiElement("perf",          "Print perf counters",          cmnd_perf),
    apiElement("conf",          "Get/set config",               cmnd_conf),
    apiElement("imu",           "Get IMU state",                cmnd_imu),
    apiElement("turn",          "Get / set turn state",         cmnd_turn),
    apiElement("debug",         "Enable/disable debug",         cmnd_debug),
    apiElement("maze_print",    "Print current maze route",     cmnd_maze_print),
    apiElement("maze_pop",      "Pop a node off the route",     cmnd_maze_pop),
    apiElement("maze_push",     "Push a node onto the route",   cmnd_maze_push),
    // commander pre-made commands
    API_ELEMENT_UPTIME,
};


void cli_init()
{
    // Clear the terminal
    //shell.clear();

    //commander.attachDebugChannel(DEBUG);

    commander.attachTree(API_tree);
    commander.init();
    shell.attachCommander(&commander);
    shell.begin("sluka");
}


void cli_loop()
{
    shell.update();
}
