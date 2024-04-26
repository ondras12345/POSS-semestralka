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
#include "hardware.h"

static Shellminator shell(&Serial);
static Commander commander;

static void cmnd_encoder(char *args, Stream *response)
{
    (void)args;
    response->print(F("encoder: L"));
    response->print(encoder_pulse_left());
    response->print(F(" R"));
    response->println(encoder_pulse_right());
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
        Serial.println(F("out of range"));
        goto usage;
    }
    Serial.println(F("moving"));
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
        Serial.println(F("out of range"));
        goto usage;
    }
    Serial.println(F("moving"));
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
    response->println("Usage: conf [name value]");

    char * setting_name = strsep(&args, " ");
    char * setting_value = args;
    if (setting_name == nullptr)
    {
        // this is ok, just print out config
    }
    //else if (strcmp(setting_name, "-s") == 0)
    //{
    //    settings_write(s);
    //}
    else if (setting_value == nullptr)
    {
        response->println("Missing value");
    }

#define uint8_conf(name) \
    else if (strcmp(setting_name, #name) == 0) \
    { \
        unsigned int tmp; \
        sscanf(setting_value, "%u", &tmp); \
        if (tmp > 255) \
        { \
            response->println("Value not in range 0-255"); \
            goto bad; \
        } \
        conf.name = (uint8_t)tmp; \
    }

#define Bool_conf(name) \
    else if (strcmp(setting_name, #name) == 0) \
    { \
        conf.name = (setting_value[0] == '1'); \
    }

#define float_conf(name) \
    else if (strcmp(setting_name, #name) == 0) \
    { \
        conf.name = atof(setting_value); \
    }


    uint8_conf(base_speed)
    float_conf(Kp)
    float_conf(Ki)

    else
    {
        response->print("Invalid config option: ");
        response->println(setting_name);
    }

bad:
    response->println();
    response->println("configuration:");
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


static Commander::API_t API_tree[] = {
    apiElement("encoder",       "Read rotary encoders",         cmnd_encoder),
    apiElement("line",          "Read line follower",           cmnd_line),
    apiElement("motor_move",    "Set motor output (nonlinear)", cmnd_motor_move),
    apiElement("motor_move_lin","Set motor output (linearized)",cmnd_motor_move_lin),
    apiElement("state",         "Get/set state machine state",  cmnd_state),
    apiElement("perf",          "Print perf counters",          cmnd_perf),
    apiElement("conf",          "Get/set config",               cmnd_conf),
    apiElement("imu",           "Get IMU state",                cmnd_imu),
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
