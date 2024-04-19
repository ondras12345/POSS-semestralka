#include "cli.h"
#include <Arduino.h>
#include <Shellminator.hpp>
#include <Shellminator-IO.hpp>
#include <Commander-API.hpp>
#include <Commander-API-Commands.hpp>
#include <Commander-IO.hpp>
#include "encoder.h"
#include "line_follower.h"

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
}

static Commander::API_t API_tree[] = {
    apiElement("encoder",       "Read rotary encoders",     cmnd_encoder),
    apiElement("line",          "Read line follower",       cmnd_line),
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
