#pragma once

#define PIN_ENCODER_LEFT_A 18
#define PIN_ENCODER_LEFT_B 43
#define PIN_ENCODER_RIGHT_A 19
#define PIN_ENCODER_RIGHT_B 42

//#define ENCODER_PULSE_PER_REVOLUTION 9
//#define MOTOR_GEAR_RATIO 39.267f

#define PIN_MOTOR_LEFT_PWM 10
#define PIN_MOTOR_LEFT_IN1 47
#define PIN_MOTOR_LEFT_IN2 46

#define PIN_MOTOR_RIGHT_PWM 11
#define PIN_MOTOR_RIGHT_IN1 49
#define PIN_MOTOR_RIGHT_IN2 48

// pri techto stridach se zacnou roztacet s pasem ve vzduchu
#define MOTOR_LEFT_OFFSET 40
#define MOTOR_RIGHT_OFFSET 55

#define PIN_BUZZER 45

#define PIN_BUMPER_LEFT 62
#define PIN_BUMPER_RIGHT 67

#define LINE_FOLLOWER_DEBOUNCE 5  // * 10ms

bool get_emergency();
