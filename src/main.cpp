#include <Arduino.h>
#include <MeAuriga.h>
#include <perf_counter.h>
#include <PID.h>
#include "hardware.h"
#include "encoder.h"
#include "motor.h"
#include "cli.h"
#include "line_follower.h"
#include "robot.h"
#include "conf.h"
#include "imu.h"
#include "turn.h"
#include "maze.h"
#include "debug.h"
#include "error.h"

/*
 * V cili je potreba jasne indikovat, ze jsme do nej dojeli.
 * Prvni kriz neni potreba mapovat.
 */

// Ultrazvukovy snimac
// pouziti: vzdalenost = sonar.distanceCm()
//MeUltrasonicSensor sonar(PORT_10);

// Servo
const byte servoPin = 68;
//const byte servoMin = 13;
//const byte servoMax = 137;
Servo servo;

// RGB LED ring
const byte numberOfLEDs = 12;
const byte rgbLEDringPin = 44;
#define RINGALLLEDS        0
MeRGBLed ledRing(0, numberOfLEDs );

#define amber      255,194,000
#define orange     255,165,000
#define vermillion 227,066,052
#define red        255,000,000
#define magenta    255,000,255
#define purple     128,000,128
#define indigo     075,000,130
#define blue       000,000,255
#define aquamarine 127,255,212
#define green      000,255,000
#define chartreuse 127,255,000
#define yellow     255,255,000
#define white      000,000,000
#define black      255,255,255

// bzučák
MeBuzzer buzzer;

#define PERF_COUNTERS(X) \
    X(cli) \
    X(line_follower) \
    X(state_machine) \
    X(imu) \
    X(turn) \
    X(maze)

#define X_pc(name) perf_counter_t pc_##name = {#name, 0, 0};
PERF_COUNTERS(X_pc)

#define X_pc_array(name) &pc_##name,
perf_counter_t * perf_counters[] = {
    PERF_COUNTERS(X_pc_array)
    NULL
};

bool emergency = true;
bool get_emergency() { return emergency; }
void set_emergency()
{
    emergency = true;
    robot_state = s_emergency;
}

/// Show 4-bit error code on LED ring
void error_code(error_code_t code)
{
    for (uint8_t i = 0; i < 4; i++)
    {
        ledRing.setColorAt(i, (code & (1<<i)) ? 255 : 0, 0, 0);
    }
    ledRing.show();
    buzzer.tone(440, 150);
}


void setup()
{
    // nastav piny narazniku
    pinMode(PIN_BUMPER_LEFT, INPUT_PULLUP);
    pinMode(PIN_BUMPER_RIGHT, INPUT_PULLUP);

    conf_init();
    maze_init();

    encoder_init();
    motor_init();

    // pripoj a omez servo
    servo.attach(servoPin);//,servoMin,servoMax);
    servo.write(90);  // jinak se trese

    // inicializace RGB LED ringu
    ledRing.setpin(rgbLEDringPin);
    ledRing.setColor(RINGALLLEDS, 0, 0, 0);
    ledRing.show();

    buzzer.setpin(PIN_BUZZER);
    buzzer.noTone();

    line_follower_init();
    imu_init();
    turn_init();

    Serial.begin(115200);
    cli_init();

    error_code(e_emergency);
}

void loop()
{
    unsigned long now = millis();

    perf_counter_measure(&pc_cli, cli_loop());
    perf_counter_measure(&pc_line_follower, line_follower_loop(now));
    perf_counter_measure(&pc_imu, imu_loop(now));
    perf_counter_measure(&pc_turn, turn_loop(now));
    perf_counter_measure(&pc_maze, maze_loop(now));

    static encoder_position_t last_report_pos = {0,0};
    const encoder_position_t pos = encoder_position();
    if (encoder_distance_mm(last_report_pos, pos) >= 10)
    {
        DEBUG_encoder->println(F("[D] went 10mm"));
        last_report_pos = pos;
    }

    static unsigned long prev_millis = 0;

    perf_counter_start(&pc_state_machine);
    switch (robot_state)
    {
        case s_emergency:
            emergency = true;
            maze_stop();
            line_follower_stop();
            if (now - prev_millis >= 500)
            {
                Serial.println(F("emergency, left bumper to start mapping, right to start following"));
                prev_millis = now;
            }
            if (digitalRead(PIN_BUMPER_LEFT) == LOW)
            {
                error_code(e_OK);
                emergency = false;
                robot_state = s_map_start;
            }
            if (digitalRead(PIN_BUMPER_RIGHT) == LOW)
            {
                error_code(e_OK);
                emergency = false;
                robot_state = s_maze_follow;
            }
            break;

        case s_idle:
            break;

        case s_line_follow:
            // start following line
            // This is only used for testing
            line_follower_follow(conf.base_speed);
            robot_state = s_line_following;
            break;

        case s_line_following:
        {
            // keep following line until the next nonnegotiable crossroad,
            // then stop
            crossroad_t cr = line_follower_crossroad();  // pozor, tato funkce neumi vsechny krizovatky
            if (cr != cr_I && cr != cr_7 && cr != cr_G)
            {
                line_follower_stop();
                robot_state = s_stop;
            }
        }
            break;

        case s_maze_follow:
            if (maze_route_current.top == 0)
            {
                // if this was triggered by pressing right bumper in emergency
                // mode and there is no route, go to idle state
                robot_state = s_idle;
                break;
            }
            maze_follow();
            robot_state = s_maze_following;
            break;

        case s_maze_following:
            if (!maze_following()) robot_state = s_finish;
            break;

        case s_map_start:
            maze_map();
            robot_state = s_map_mapping;
            break;

        case s_map_mapping:
            if (!maze_mapping()) robot_state = s_finish;
            break;

        case s_finish:
            // jsme v cili
            if (now - prev_millis >= 500UL)
            {
                ledRing.setColor(RINGALLLEDS, green);
                ledRing.show();
                prev_millis = now;
                buzzer.tone(3000, 200);
            }
            if (!digitalRead(PIN_BUMPER_RIGHT))
            {
                robot_state = s_maze_follow;
            }
            break;

        default:
            Serial.println(F("[E] state not handled"));
        case s_stop:
            maze_stop();
            line_follower_stop();
            motor_move_lin(0, 0);
            robot_state = s_idle;
            break;
    }
    perf_counter_stop(&pc_state_machine);
}
