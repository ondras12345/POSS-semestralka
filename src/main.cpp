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
    X(turn)

#define X_pc(name) perf_counter_t pc_##name = {#name, 0, 0};
PERF_COUNTERS(X_pc)

#define X_pc_array(name) &pc_##name,
perf_counter_t * perf_counters[] = {
    PERF_COUNTERS(X_pc_array)
    NULL
};

bool emergency = true;
bool get_emergency() { return emergency; }


maze_route_t maze_route_current;
maze_route_t route_follow_route;
uint8_t route_follow_index;

typedef enum {
    e_OK = 0,
    e_should_be_following = 1,
    e_unexpected_crossroad = 2,
    e_emergency = 3,
} error_t;

/// Show 4-bit error code on LED ring
void error_code(error_t code)
{
    for (uint8_t i = 0; i < 4; i++)
    {
        ledRing.setColorAt(i, (code & (1<<i)) ? 255 : 0, 0, 0);
    }
    ledRing.show();
}


void setup() {
    // nastav piny narazniku
    pinMode(PIN_BUMPER_LEFT, INPUT_PULLUP);
    pinMode(PIN_BUMPER_RIGHT, INPUT_PULLUP);

    conf_init();
    maze_route_init(&maze_route_current);  // TODO move?

    encoder_init();
    motor_init();

    // pripoj a omez servo
    servo.attach(servoPin);//,servoMin,servoMax);
    servo.write(90);

    // inicializace RGB LED ringu
    // pro ovládání slouží metoda
    // bool MeRGBLed::setColor(uint8_t index, uint8_t red, uint8_t green, uint8_t blue)
    ledRing.setpin( rgbLEDringPin );
    ledRing.setColor( RINGALLLEDS, 0, 0, 0);
    ledRing.show();

    // nastavení bzučáku
    buzzer.setpin(PIN_BUZZER);
    buzzer.noTone();

    // inicializace sledovani cary
    line_follower_init();
    imu_init();
    turn_init();

    // inicializace sériového kanálu
    Serial.begin(115200);
    cli_init();
}

void loop()
{
    unsigned long now = millis();

    perf_counter_measure(&pc_cli, cli_loop());
    perf_counter_measure(&pc_line_follower, line_follower_loop(now));
    perf_counter_measure(&pc_imu, imu_loop(now));
    perf_counter_measure(&pc_turn, turn_loop(now));

    static encoder_position_t last_report_pos = {0,0};
    encoder_position_t pos = encoder_position();
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
            error_code(e_emergency);
            line_follower_stop();
            if (now - prev_millis >= 500)
            {
                Serial.println(F("emergency, waiting for left bumper"));
                prev_millis = now;
            }
            if (digitalRead(PIN_BUMPER_LEFT) == LOW)
            {
                error_code(e_OK);
                emergency = false;
                robot_state = s_idle;
            }
            break;

        case s_idle:
            break;

        case s_line_follow:
            // start following line
            line_follower_follow(conf.base_speed);
            robot_state = s_line_following;
            break;

        case s_line_following:
        {
            // keep following line until the next nonnegotiable crossroad,
            // then stop
            crossroad_t cr = line_follower_crossroad();  // TODO tato funkce neumi vsechny krizovatky
            if (cr != cr_I && cr != cr_7 && cr != cr_G)
            {
                line_follower_stop();
                robot_state = s_stop;
            }
        }
            break;

        case s_maze_follow:
            // start following maze_route_current
            maze_route_clone(&route_follow_route, &maze_route_current);
            route_follow_index = 0;
            line_follower_follow(conf.base_speed);
            line_follower_last_crossroad_updated();  // clear it
            robot_state = s_maze_following;
            DEBUG_maze_follow->println(F("[D] starting maze_follow"));
            break;

        case s_maze_following:
            if (!line_follower_following())
            {
                Serial.println(F("[E] should be following"));
                error_code(e_should_be_following);
                robot_state = s_emergency;
                break;
            }

            if (line_follower_last_crossroad_updated())
            {
                maze_route_node_t node = route_follow_route.stack[route_follow_index];
                crossroad_t cr = line_follower_last_crossroad();

                if (cr == cr_0)
                {
                    Serial.println(F("[E] cr_0"));
                    robot_state = s_emergency;
                    break;
                }

                if (cr == node.crossroad)
                {
                    DEBUG_maze_follow->println(F("[D] expected last crossroad"));
                    if (route_follow_index < route_follow_route.top-1)
                    {
                        route_follow_index++;
                    }
                    else
                    {
                        robot_state = s_finish;
                        line_follower_stop();
                        break;
                    }

                    switch (node.direction)
                    {
                        case crd_straight:
                            break;

                        case crd_left:
                        case crd_right:
                            line_follower_stop();
                            float angle = conf.turn_target;
                            if (node.direction == crd_left) angle *= -1;
                            turn_turn_relative(angle, true);
                            robot_state = s_maze_following_turning;
                            DEBUG_maze_follow->print(F("[D] turning "));
                            DEBUG_maze_follow->println(angle);
                            break;
                    }
                }
                else
                {
                    Serial.print(F("[W] undexpected crossroad: "));
                    Serial.write(cr);
                    Serial.println();
                    error_code(e_unexpected_crossroad);
                }
            }
            break;

        case s_maze_following_turning:
            if (!turn_status())
            {
                // finished turning
                line_follower_follow(conf.base_speed);
                robot_state = s_maze_following;
                DEBUG_maze_follow->println(F("[D] finished turning "));
            }
            break;

        case s_finish:
            // jsme v cili
            if (now - prev_millis >= 500UL)
            {
                ledRing.setColor(0, green);
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
            line_follower_stop();
            motor_move_lin(0, 0);
            robot_state = s_idle;
            break;
    }
    perf_counter_stop(&pc_state_machine);
}
