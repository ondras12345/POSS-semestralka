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
bool map_backtracking;
encoder_position_t map_prev_cr_pos;
const crossroad_direction_t map_dir_order[] = {
    // TODO optimize
    crd_left,
    crd_right,
    crd_straight,
};


/**
 * Given a crossroad and optionally the previous direction, return the next
 * direction from map_dir_order.
 * Return value crd_invalid means all options have already been tried.
 */
crossroad_direction_t map_dir_next(crossroad_t cr, crossroad_direction_t crd=crd_invalid)
{
    if (crd == crd_invalid) crd = map_dir_order[0];
    bool found_start = false;
    for (uint8_t i = 0; i < sizeof(map_dir_order)/sizeof(map_dir_order[0]); i++)
    {
        crossroad_direction_t crd_i = map_dir_order[i];
        if (crd_i == crd) found_start = true;
        if (!found_start) continue;
        if (crossroad_direction_valid(cr, crd_i))
        {
            return crd_i;
        }
    }
    // no solution found
    return crd_invalid;
}


/**
 * Initiate turn_turn_relative from given crossroad direction
 */
bool turn_from_direction(crossroad_direction_t crd)
{
    switch (crd)
    {
        case crd_back:  // invalid input
        case crd_invalid:  // invalid input
        case crd_straight:  // no need to turn
            return false;

        case crd_left:
        case crd_right:
            line_follower_stop();
            float angle = conf.turn_target;
            if (crd == crd_left) angle *= -1;
            turn_turn_relative(angle, true);
            robot_state = s_map_turning;
            Serial.print(F("[D] turning "));
            Serial.println(angle);
            return true;
    }
    return false;  // this should never happen
}


typedef enum {
    e_OK = 0,
    e_should_be_following = 1,
    e_unexpected_crossroad = 2,
    e_emergency = 3,
    e_no_valid_direction = 4,
    e_cr_0 = 5,
} error_t;

/// Show 4-bit error code on LED ring
void error_code(error_t code)
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
    maze_route_init(&maze_route_current);

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
                robot_state = s_idle;  // TODO start mapping instead
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
            // start following maze_route_current
            maze_route_clone(&route_follow_route, &maze_route_current);
            route_follow_index = 0;
            line_follower_follow(conf.base_speed);
            line_follower_clear();
            robot_state = s_maze_following;
            DEBUG_maze_follow->println(F("[D] starting maze_follow"));
            break;

        case s_maze_following:
        {
            if (!line_follower_following())
            {
                Serial.println(F("[E] should be following"));
                error_code(e_should_be_following);
                robot_state = s_emergency;
                break;
            }

            maze_route_node_t node = route_follow_route.stack[route_follow_index];

            int32_t dist = encoder_distance_mm(line_follower_last_crossroad_position(), pos);
            line_follower_follow(
                (dist >= conf.fast_offset_mm &&
                 dist <= node.distance_mm - conf.fast_offset_mm)
                ? conf.fast_speed
                : conf.base_speed
            );

            if (line_follower_last_crossroad_updated())
            {
                crossroad_t cr = line_follower_last_crossroad();

                if (cr == cr_0)
                {
                    Serial.println(F("[E] cr_0"));
                    error_code(e_cr_0);
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

                    if (turn_from_direction(node.direction))
                    {
                        robot_state = s_maze_following_turning;
                    }
                }
                else
                {
                    Serial.print(F("[W] unexpected crossroad: "));
                    Serial.write(cr);
                    Serial.println();
                    error_code(e_unexpected_crossroad);
                }
            }
        }
            break;

        case s_maze_following_turning:
            if (!turn_status())
            {
                // finished turning
                line_follower_follow(conf.base_speed);
                robot_state = s_maze_following;
                line_follower_clear();
                DEBUG_maze_follow->println(F("[D] finished turning "));
            }
            break;

        case s_map_start:
            map_backtracking = false;
            maze_route_init(&maze_route_current);  // clear route
            map_prev_cr_pos = pos;
            line_follower_follow(conf.map_speed);
            line_follower_clear();
            robot_state = s_map_straight;
            break;

        case s_map_straight:
            if (!line_follower_following())
            {
                Serial.println(F("[E] should be following"));
                error_code(e_should_be_following);
                robot_state = s_emergency;
                break;
            }

            if (line_follower_last_crossroad_updated())
            {
                crossroad_t cr = line_follower_last_crossroad();
                if (cr == cr_0)
                {
                    Serial.println(F("[E] cr_0"));
                    error_code(e_cr_0);
                    robot_state = s_emergency;
                    break;
                }

                if (map_backtracking)
                {
                    maze_route_node_t node = maze_route_pop(&maze_route_current);
                    if (crossroad_rotate(node.crossroad, node.direction) != cr)
                    {
                        Serial.println(F("[E] e_unexpected_crossroad"));
                        error_code(e_unexpected_crossroad);
                        robot_state = s_emergency;
                        break;
                    }

                    crossroad_direction_t prev_dir = node.direction;
                    crossroad_direction_t next_dir = map_dir_next(node.crossroad, prev_dir);
                    if (next_dir == crd_invalid)
                    {
                        // all directions have been tried, return to previous crossroad
                        crossroad_direction_t next_dir_rotated = crossroad_direction_rotate(crd_back, prev_dir);
                        // No maze_route_push, we are removing this crossroad from
                        // the stack.
                        if (turn_from_direction(next_dir_rotated))
                        {
                            robot_state = s_map_turning;
                        }
                    }
                    else
                    {
                        // try next direction
                        map_backtracking = false;
                        node.direction = next_dir;
                        maze_route_push(&maze_route_current, node);
                        crossroad_direction_t next_dir_rotated = crossroad_direction_rotate(next_dir, prev_dir);
                        if (turn_from_direction(next_dir_rotated))
                        {
                            robot_state = s_map_turning;
                        }
                    }
                }
                else
                {
                    if (cr == cr_i)
                    {
                        // dead end, turn 180 degrees
                        map_backtracking = true;
                        line_follower_stop();
                        turn_turn_relative(180, true);
                        robot_state = s_map_turning;
                        DEBUG_map->println("[D] dead end, turning 180");
                        break;
                    }

                    if (cr == cr_F)
                    {
                        line_follower_stop();
                        robot_state = s_finish;
                        DEBUG_map->println("[D] finish");
                        break;
                    }

                    // new crossroad that we haven't seen yet
                    line_follower_stop();
                    //delay(500); // TODO ??
                    maze_route_node_t node;
                    node.crossroad = cr;
                    encoder_position_t lpos = line_follower_last_crossroad_position();
                    node.distance_mm = encoder_distance_mm(map_prev_cr_pos, lpos);
                    map_prev_cr_pos = lpos;
                    node.direction = map_dir_next(cr);
                    if (node.direction == crd_invalid)
                    {
                        Serial.println(F("[E] no valid direction"));
                        error_code(e_no_valid_direction);
                        robot_state = s_emergency;
                        break;
                    }
                    DEBUG_map->print(F("[D] found new crossroad: "));
                    print_maze_node(node, DEBUG_map);
                    maze_route_push(&maze_route_current, node);
                    if (turn_from_direction(node.direction))
                    {
                            robot_state = s_map_turning;
                    }
                }
            }
            break;

        case s_map_turning:
            if (!turn_status())
            {
                // finished turning
                line_follower_follow(conf.map_speed);
                robot_state = s_map_straight;
                line_follower_clear();
                DEBUG_map->println(F("[D] finished turning "));
            }
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
            line_follower_stop();
            motor_move_lin(0, 0);
            robot_state = s_idle;
            break;
    }
    perf_counter_stop(&pc_state_machine);
}
