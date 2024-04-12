#include "encoder.h"
#include <Arduino.h>
#include "hardware.h"
#include <util/atomic.h>

static volatile long pulse_left;
static volatile long pulse_right;


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


long encoder_pulse_left()
{
    long r;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        r = pulse_left;
    }
    return r;
}


long encoder_pulse_right()
{
    long r;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        r = pulse_right;
    }
    return r;
}
