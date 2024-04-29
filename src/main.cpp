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

/*
 * V cili je potreba jasne indikovat, ze jsme do nej dojeli.
 * Prvni kriz neni potreba mapovat.
 *
 * TODO bluetooth nefunguje
 * sudo rfcomm bind 0 <btaddr>
 */

// Ultrazvukovy snimac
// pouziti: vzdalenost = sonar.distanceCm()
//MeUltrasonicSensor sonar(PORT_10);

// Servo
const byte servoPin = 68;
const byte servoMin = 13;
const byte servoMax = 137;
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

perf_counter_t pc_cli =             { "cli",           0, 0};
perf_counter_t pc_line_follower =   { "line_follower", 0, 0};
perf_counter_t pc_state_machine =   { "state_machine", 0, 0};
perf_counter_t pc_pid_line =        { "pid_line",      0, 0};
perf_counter_t pc_imu =             { "IMU",           0, 0};

perf_counter_t * perf_counters[] = {
    &pc_cli,
    &pc_line_follower,
    &pc_state_machine,
    &pc_pid_line,
    &pc_imu,
    NULL
};

bool emergency = true;
bool get_emergency() { return emergency; }


// TODO
#define PID_LINE_TS 20UL
PID_t PID_line;


void setup() {
    // nastav piny narazniku
    pinMode(PIN_BUMPER_LEFT, INPUT_PULLUP);
    pinMode(PIN_BUMPER_RIGHT, INPUT_PULLUP);

    PID_init(&PID_line, PID_LINE_TS*1e-3);
    conf_init();

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

    // inicializace sériového kanálu
    Serial.begin(115200);
    cli_init();
}

void loop()
{
    perf_counter_measure(&pc_cli, cli_loop());
    perf_counter_measure(&pc_line_follower, line_follower_loop());
    perf_counter_measure(&pc_imu, imu_loop());

    unsigned long now = millis();
    static unsigned long prev_millis = 0;

    perf_counter_start(&pc_state_machine);
    switch (robot_state)
    {
        case s_emergency:
            if (now - prev_millis >= 500)
            {
                Serial.println(F("emergency, waiting for left bumper"));
                prev_millis = now;
            }
            if (digitalRead(PIN_BUMPER_LEFT) == LOW) emergency = false;
            if (!get_emergency()) robot_state = s_idle;
            break;

        case s_idle:
            break;

        case s_stop:
            motor_move_lin(0, 0);
            robot_state = s_idle;
            break;

        case s_line_follow:
            // TODO start motors & controller
            robot_state = s_line_following;
            prev_millis = now-PID_LINE_TS-1;
            PID_line.Kp = conf.Kp;
            PID_line.Ki = conf.Ki;
            PID_line.Tt = 1e3;
            PID_line.Tf = 1e3;
            PID_line.umax = conf.umax;
            PID_new_params(&PID_line);
            break;

        case s_line_following:
            crossroad_t cr = line_follower_crossroad();  // TODO tato funkce neumi vsechny krizovatky
            if (cr != cr_I && cr != cr_7 && cr != cr_G)
            {
                robot_state = s_stop;
            }
            else
            {
                if (now - prev_millis >= PID_LINE_TS)
                {
                    perf_counter_start(&pc_pid_line);
                    int8_t u = (int8_t)PID_loop(&PID_line, line_follower_offset(), 0);
                    perf_counter_stop(&pc_pid_line);
                    motor_move_lin(conf.base_speed-u, conf.base_speed+u);
                    prev_millis = now;
                }
            }
            break;
    }
    perf_counter_stop(&pc_state_machine);
}
