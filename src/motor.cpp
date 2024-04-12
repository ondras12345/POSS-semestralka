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
    bool reverse_left = (speed_left < 0);
    digitalWrite(PIN_MOTOR_LEFT_IN1, !reverse_left);
    digitalWrite(PIN_MOTOR_LEFT_IN2, reverse_left);
    analogWrite(PIN_MOTOR_LEFT_IN1, abs(speed_left));

    bool reverse_right = (speed_right < 0);
    digitalWrite(PIN_MOTOR_RIGHT_IN1, !reverse_right);
    digitalWrite(PIN_MOTOR_RIGHT_IN2, reverse_right);
    analogWrite(PIN_MOTOR_RIGHT_IN1, abs(speed_right));
}
