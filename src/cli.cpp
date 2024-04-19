#include "cli.h"
#include <Arduino.h>
#include <Shellminator.hpp>
#include <Shellminator-IO.hpp>
#include <Commander-API.hpp>
#include <Commander-API-Commands.hpp>
#include <Commander-IO.hpp>
#include "encoder.h"
#include "motor.h"
#include "line_follower.h"
#include "robot.h"

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
    if (!robot_set_state(args))
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
    return;

usage:
    response->println(F("usage: state [s_name]"));
}


static Commander::API_t API_tree[] = {
    apiElement("encoder",       "Read rotary encoders",         cmnd_encoder),
    apiElement("line",          "Read line follower",           cmnd_line),
    apiElement("motor_move",    "Set motor output (nonlinear)", cmnd_motor_move),
    apiElement("motor_move_lin","Set motor output (linearized)",cmnd_motor_move_lin),
    apiElement("state",         "Get/set state machine state",  cmnd_state),
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
