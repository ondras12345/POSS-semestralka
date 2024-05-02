#include "line_follower.h"
#include <Arduino.h>
#include <MeRGBLineFollower.h>
#include <PID.h>
#include "hardware.h"
#include "conf.h"
#include "motor.h"

static MeRGBLineFollower RGBLineFollower(PORT_9);

static crossroad_t crossroad = cr_0;
static crossroad_t last_crossroad = cr_0;
static bool last_crossroad_updated = false;
static uint8_t state_debounced = 0b1111;

static bool following = false;
#define PID_LINE_TS 20UL
static PID_t PID_line;
static uint8_t base_speed;


void line_follower_init()
{
    RGBLineFollower.begin();
    RGBLineFollower.setKp(1);
}


void line_follower_loop(unsigned long now)
{
    static unsigned long controller_prev_millis = 0;
    if (now - controller_prev_millis >= PID_LINE_TS)
    {
        int8_t u = (int8_t)PID_loop(&PID_line, line_follower_offset(), 0);
        motor_move_lin(base_speed-u, base_speed+u);
        controller_prev_millis = now;
    }


    static unsigned long prev_millis = 0;

    if (now - prev_millis < 10UL) return;
    prev_millis = now;
    // sam to stejne dela jen kazdych 9 ms
    RGBLineFollower.loop();

    constexpr uint8_t N=4;
    static uint8_t counters[N] = {0};
    uint8_t state = line_follower_state();
    for (uint8_t i = 0; i < N; i++)
    {
        if (state & (1<<i))
        {
            if (counters[i] < LINE_FOLLOWER_DEBOUNCE) counters[i]++;
        }
        else
        {
            if (counters[i] > 0) counters[i]--;
        }

        if (counters[i] >= LINE_FOLLOWER_DEBOUNCE)
        {
            state_debounced |= (1<<i);
            counters[i] = LINE_FOLLOWER_DEBOUNCE; // tohle by nemelo byt potreba
        }
        else if (counters[i] == 0)
        {
            state_debounced &= ~(1<<i);
        }
    }

    static crossroad_t prev_crossroad = cr_0;
    // never change from complex prev_crossroad to simple
    if (!(prev_crossroad == cr_T && (crossroad == cr_G || crossroad == cr_7 )))
    {
        prev_crossroad = crossroad;
    }
    // detekce krizovatek
    // 0 je cara
    switch (state_debounced)
    {
        case 0b0001:
        case 0b0011:
        //case 0b0111:  // to uz je moc
            crossroad = cr_G;
            break;

        case 0b1000:
        case 0b1100:
        //case 0b1110:  // to uz je moc
            crossroad = cr_7;
            break;

        case 0b0000:
            crossroad = cr_T;
            break;

        case 0b1001:
            crossroad = cr_I;
            break;

        default:
            // invalid
            crossroad = cr_0;
            break;
    }

    if (prev_crossroad != crossroad)
    {
        last_crossroad_updated = true;
        if (prev_crossroad == cr_T)
        {
            if (crossroad == cr_I) last_crossroad = cr_X;
            else last_crossroad = cr_T;
        }
        else if (prev_crossroad == cr_G)
        {
            if (crossroad == cr_I) last_crossroad = cr_E;
            else last_crossroad = cr_G;
        }
        else if (prev_crossroad == cr_7)
        {
            if (crossroad == cr_I) last_crossroad = cr_3;
            else last_crossroad = cr_7;
        }
        else
        {
            // ignore prev_crossroad == cr_0 || prev_crossroad == cr_I
            last_crossroad_updated = false;
        }
    }
    // TODO last_crossroad se zmeni pri vjezdu do T (X)
    // TODO add debug messages
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

/**
 * Return current crossroad.
 * Only use this for debugging, does not detect X/T G/E 7/3 correctly.
 */
crossroad_t line_follower_crossroad()
{
    return crossroad;
}

/**
 * Return last valid crossroad.
 * This is only updated once the crossroad is passed.
 */
crossroad_t line_follower_last_crossroad()
{
    return last_crossroad;
}

/**
 * Return true if last_crossroad was updated and clears the flag.
 */
bool line_follower_last_crossroad_updated()
{
    bool tmp = last_crossroad_updated;
    last_crossroad_updated = false;
    return tmp;
}


void line_follower_follow(uint8_t speed)
{
    PID_init(&PID_line, PID_LINE_TS*1e-3);
    PID_line.Kp = conf.Kp;
    PID_line.Ki = conf.Ki;
    PID_line.Tt = 1e3;
    PID_line.Tf = 1e3;
    PID_line.umax = conf.umax;
    PID_new_params(&PID_line);

    following = true;
    base_speed = speed;
}


void line_follower_stop()
{
    following = false;
    motor_move_lin(0, 0);
}


bool line_follower_following()
{
    return following;
}
