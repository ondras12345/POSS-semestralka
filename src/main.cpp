#include <Arduino.h>
#include <MeAuriga.h>
#include "hardware.h"
#include "encoder.h"
#include "motor.h"
#include "cli.h"
#include "line_follower.h"
#include "robot.h"

/*
 * V cili je potreba jasne indikovat, ze jsme do nej dojeli.
 * Prvni kriz neni potreba mapovat.
 *
 * TODO bluetooth nefunguje
 * sudo rfcomm bind 0 <btaddr>
 */


int rychlostJizdy = 200;
int minRychlost = 100;
int maxRychlost = 255;

// Ultrazvukovy snimac
// pouziti: vzdalenost = sonar.distanceCm()
MeUltrasonicSensor sonar(PORT_10);

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

// Gyro
//MeGyro gyro(1,0x69);


void setup() {
    // nastav piny narazniku
    pinMode(PIN_BUMPER_LEFT, INPUT_PULLUP);
    pinMode(PIN_BUMPER_RIGHT, INPUT_PULLUP);

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

    // inicializace gyra
    //gyro.begin();

    // inicializace sledovani cary
    line_follower_init();
    // inicializace sériového kanálu
    Serial.begin(115200);
    cli_init();

    Serial.println("waiting for left bumper");
    while (digitalRead(PIN_BUMPER_LEFT)) {
        // nepokracuj dokud neni stiknut levy naraznik
    }
}

void loop()
{
    cli_loop();
    line_follower_loop();

    switch (robot_state)
    {
        case s_boot:
            break;

        case s_line_follow:
            if (line_follower_crossroad() != cr_I)
            {
                motor_move_lin(0, 0);
                robot_state = s_boot;
            }
            else
            {
                // TODO
            }
            break;
    }
}
