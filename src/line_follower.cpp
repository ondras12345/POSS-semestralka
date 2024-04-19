#include "line_follower.h"
#include <Arduino.h>
#include <MeRGBLineFollower.h>

static MeRGBLineFollower RGBLineFollower(PORT_9);

static crossroad_t crossroad = cr_0;
static uint8_t state_debounced = 0b1111;


void line_follower_init()
{
    RGBLineFollower.begin();
    RGBLineFollower.setKp(1);
}


void line_follower_loop()
{
    static unsigned long prev_millis = 0;

    unsigned long now = millis();
    if (now - prev_millis < 10UL) return;

    prev_millis = now;
    // sam to stejne dela jen kazdych 9 ms
    RGBLineFollower.loop();

    uint8_t counters[4] = {0};
    constexpr uint8_t counter_max = 10;
    uint8_t state = line_follower_state();
    for (uint8_t i = 0; i < 4; i++)
    {
        if (state & (1<<i))
        {
            if (counters[i] < counter_max) counters[i]++;
        }
        else
        {
            if (counters[i] > 0) counters[i]--;
        }

        if (counters[i] >= counter_max)
        {
            state_debounced |= (1<<i);
            counters[i] = counter_max; // tohle by nemelo byt potreba
        }
        else if (counters[i] == 0)
        {
            state_debounced &= ~(1<<i);
        }
        // TODO state_debounced se zasekava v nule
    }
    Serial.println(counters[0]); // TODO jen 0 nebo 1

    // detekce krizovatek
    // 0 je cara
    switch (state_debounced & 0x09)  // vnejsi cidla
    {
        case 0b0001:
            crossroad = cr_G;
            break;

        case 0b1000:
            crossroad = cr_7;
            break;

        case 0b0000:
            crossroad = cr_T;

        default:  // 0b1001
            crossroad = cr_I;
            break;
    }

    // TODO potrebujeme popojet dal
    // T -> X?
    // G -> E?
    // 7 -> 3?
}


int16_t line_follower_offset()
{
    return RGBLineFollower.getPositionOffset();
}


uint8_t line_follower_state()
{
    // 0 je cara, 1 je okoli
    // LSB je uplne vlevo
    return RGBLineFollower.getPositionState();
}

uint8_t line_follower_state_debounced()
{
    return state_debounced;
}

crossroad_t line_follower_crossroad()
{
    return crossroad;
}
