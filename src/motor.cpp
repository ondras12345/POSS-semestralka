#include "motor.h"
#include "hardware.h"
#include <Arduino.h>

void motor_init()
{
    pinMode(PIN_MOTOR_LEFT_PWM, OUTPUT);
    pinMode(PIN_MOTOR_LEFT_IN1, OUTPUT);
    pinMode(PIN_MOTOR_LEFT_IN2, OUTPUT);

    pinMode(PIN_MOTOR_RIGHT_PWM, OUTPUT);
    pinMode(PIN_MOTOR_RIGHT_IN1, OUTPUT);
    pinMode(PIN_MOTOR_RIGHT_IN2, OUTPUT);

    // Nastavení frekvencep pwm na 8KHz pro řízení DC motorů
    TCCR1A = _BV(WGM10);
    TCCR1B = _BV(CS11) | _BV(WGM12);

    TCCR2A = _BV(WGM21) | _BV(WGM20);
    TCCR2B = _BV(CS21);
}


/**
 * Command motors to move.
 * @param speed_left    left motor duty -255..255, negative duty means reverse
 *                      direction
 * @param speed_right   right motor duty -255..255, negative duty means reverse
 *                      direction
 */
void motor_move(int16_t speed_left, int16_t speed_right)
{
    if (get_emergency())
    {
        speed_left = 0;
        speed_right = 0;
    }

    bool reverse_left = (speed_left < 0);
    digitalWrite(PIN_MOTOR_LEFT_IN1, !reverse_left);
    digitalWrite(PIN_MOTOR_LEFT_IN2, reverse_left);
    analogWrite(PIN_MOTOR_LEFT_PWM, abs(speed_left));

    speed_right = -speed_right;  // pravy motor je namontovany opacne
    bool reverse_right = (speed_right < 0);
    digitalWrite(PIN_MOTOR_RIGHT_IN1, !reverse_right);
    digitalWrite(PIN_MOTOR_RIGHT_IN2, reverse_right);
    analogWrite(PIN_MOTOR_RIGHT_PWM, abs(speed_right));
}

static int16_t lin_speed(int16_t speed, int16_t offset)
{
    if (speed == 0) return 0;
    else if (speed > 0) return min(speed + offset, 255);
    else return max(speed - offset, -255);
}

/// Linearized motor movement
/// @see motor_move
/// speed range is the same, values higher than ~200 map to 255
void motor_move_lin(int16_t speed_left, int16_t speed_right)
{
    speed_left = lin_speed(speed_left, MOTOR_LEFT_OFFSET);
    speed_right = lin_speed(speed_right, MOTOR_RIGHT_OFFSET);

    motor_move(speed_left, speed_right);
}
