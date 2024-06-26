#include "encoder.h"
#include "conf.h"

#ifndef UNIT_TEST
#include <Arduino.h>
#include <util/atomic.h>
#include "hardware.h"

static volatile uint32_t pulse_left;
static volatile uint32_t pulse_right;


static int8_t pulse_diff(bool A, bool B, bool oldA)
{
    if (B)
    {
        if (oldA - A < 0) return 1;
        else return -1;
    }
    else
    {
        if (oldA - A > 0) return 1;
        return -1;
    }
}


static void ISR_left()
{
    bool A = digitalRead(PIN_ENCODER_LEFT_A);
    bool B = digitalRead(PIN_ENCODER_LEFT_B);
    static bool oldA = false;
    pulse_left += pulse_diff(A, B, oldA);
    oldA = A;
}

static void ISR_right()
{
    bool A = digitalRead(PIN_ENCODER_RIGHT_A);
    bool B = digitalRead(PIN_ENCODER_RIGHT_B);
    static bool oldA = false;
    // pravy kanal pocita opacne --> -1*pulse_diff
    pulse_right += -pulse_diff(A, B, oldA);
    oldA = A;
}


void encoder_init()
{
    pinMode(PIN_ENCODER_LEFT_A, INPUT_PULLUP);
    pinMode(PIN_ENCODER_LEFT_B, INPUT_PULLUP);
    pinMode(PIN_ENCODER_RIGHT_A, INPUT_PULLUP);
    pinMode(PIN_ENCODER_RIGHT_B, INPUT_PULLUP);

    // inicializace obsluhy preruseni od kanalů A enkoderů
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_LEFT_A), ISR_left, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_RIGHT_A), ISR_right, CHANGE);
}


encoder_position_t encoder_position()
{
    encoder_position_t pos;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        pos.left = pulse_left;
        pos.right = pulse_right;
    }
    return pos;
}

#endif // UNIT_TEST


int32_t encoder_distance_mm(encoder_position_t start, encoder_position_t end)
{
    // TODO do not use float conf constant?
    return encoder_distance_pulses(start, end) * conf.mm_per_pulse;
}


int32_t encoder_distance_pulses(encoder_position_t start, encoder_position_t end)
{
    return (int64_t((int32_t)(end.left-start.left)) + (int32_t)(end.right-start.right))/2;
}
