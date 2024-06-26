#include "line_follower.h"

#ifndef UNIT_TEST
#include <Arduino.h>
#include <MeRGBLineFollower.h>
#endif

#include "debug.h"
#include "conf.h"
#include "motor.h"

#ifndef UNIT_TEST
static MeRGBLineFollower RGBLineFollower(PORT_9);
#endif

static crossroad_t crossroad = cr_0;
static crossroad_t last_crossroad = cr_0;
static crossroad_t prev_crossroad_cp = cr_0;
static bool last_crossroad_updated = false;
static uint8_t state_debounced = 0b1111;
static encoder_position_t last_crossroad_position;
static encoder_position_t prev_0_pos;
static encoder_position_t prev_T_pos;
static encoder_position_t prev_nontrivial_pos;  // position of prev cr that is not cr_0 or cr_I
static bool handled = true;

static bool following = false;
#define PID_LINE_TS 20UL
static uint8_t base_speed;


#ifndef UNIT_TEST
void line_follower_init()
{
    RGBLineFollower.begin();
    // CLI readings will be multiplied by default Kp=0.3 until
    // line_follower_follow() is first called.
    //RGBLineFollower.setKp(1.0);
}
#endif

static crossroad_t decode_crossroad(uint8_t state)
{
    // detekce krizovatek
    // 0 je cara
    switch (state_debounced)
    {
        case 0b0001:
        case 0b0011:
        case 0b0101:
        case 0b0111:
            return cr_G;

        case 0b1000:
        case 0b1010:
        case 0b1100:
        case 0b1110:
            return cr_7;

        case 0b0000:
            return cr_T;

        case 0b1001:
        case 0b1101:
        case 0b1011:
            return cr_I;

        default:
            // invalid
            return cr_0;
    }
}


void line_follower_loop(unsigned long now)
{
    if (following)
    {
        static unsigned long controller_prev_millis = 0;
        if (now - controller_prev_millis >= PID_LINE_TS)
        {
            int16_t off = line_follower_offset();
            int8_t u = (int8_t)(constrain(off, -conf.line_umax, conf.line_umax));
            if (crossroad == cr_0 || crossroad == cr_7 || crossroad == cr_G) u = 0;
            motor_move_lin(base_speed+u, base_speed-u);
            controller_prev_millis = now;
        }
    }


    static unsigned long prev_millis = 0;

    if (now - prev_millis < 10UL) return;
    prev_millis = now;
#ifndef UNIT_TEST
    // sam to stejne dela jen kazdych 9 ms
    RGBLineFollower.loop();
#endif

    constexpr uint8_t N=4;
    static uint8_t counters[N] = {0};
    uint8_t state = line_follower_state();
    for (uint8_t i = 0; i < N; i++)
    {
        if (state & (1<<i))
        {
            if (counters[i] < conf.line_debounce) counters[i]++;
        }
        else
        {
            if (counters[i] > 0) counters[i]--;
        }

        if (counters[i] >= conf.line_debounce)
        {
            state_debounced |= (1<<i);
            // this should never be needed
            counters[i] = conf.line_debounce;
        }
        else if (counters[i] == 0)
        {
            state_debounced &= ~(1<<i);
        }
    }


    const encoder_position_t pos = encoder_position();
    crossroad_t prev_crossroad = crossroad;

    crossroad = decode_crossroad(state_debounced);

    if (prev_crossroad != cr_0 && crossroad == cr_0)
    {
        prev_0_pos = pos;
    }
    else if (prev_crossroad != cr_T && crossroad == cr_T)
    {
        prev_T_pos = pos;
    }

    if (crossroad != cr_0 && crossroad != cr_I)
    {
        prev_nontrivial_pos = pos;
    }

    if (prev_crossroad != crossroad)
    {
        // never change from complex prev_crossroad to simple
        if (prev_crossroad_cp != cr_T && prev_crossroad != cr_0 && prev_crossroad != cr_I)
        {
            prev_crossroad_cp = prev_crossroad;
            handled = false;
        }

        DEBUG_crossroad->print(F("[D] crossroad: "));
        DEBUG_crossroad->write(crossroad);
        DEBUG_crossroad->print('\t');
        DEBUG_crossroad->print(state_debounced, BIN);
        DEBUG_crossroad->print(F("\tprev: "));
        DEBUG_crossroad->write(prev_crossroad);
        DEBUG_crossroad->print(F("\tprev_cp: "));
        DEBUG_crossroad->write(prev_crossroad_cp);
        DEBUG_crossroad->println();
    }

    if (encoder_distance_mm(prev_nontrivial_pos, pos) >= conf.cr_delay_mm && !handled)
    {
        handled = true;
        if (crossroad == cr_I || crossroad == cr_0)
        {
            last_crossroad_updated = true;
            if (crossroad == cr_0)
            {
                last_crossroad = prev_crossroad_cp;
            }
            else
            {
                switch (prev_crossroad_cp)
                {
                    case cr_T:
                        last_crossroad = cr_X;
                        break;
                    case cr_G:
                        last_crossroad = cr_E;
                        break;
                    case cr_7:
                        last_crossroad = cr_3;
                        break;
                    case cr_0:
                        // cr_0 is handled somewhere else
                        //last_crossroad = cr_0;
                        last_crossroad_updated = false;
                        break;
                    default:
                        Serial.print(F("[E] weird prev_crossroad: "));
                        Serial.write(prev_crossroad_cp);
                        Serial.println();
                        break;
                }
            }
            if (last_crossroad_updated)
            {
                last_crossroad_position = pos;
                DEBUG_crossroad->print(F("[D] last crossroad: "));
                DEBUG_crossroad->write(last_crossroad);
                DEBUG_crossroad->println();
            }

            if (crossroad == cr_0) prev_0_pos = pos;
            prev_crossroad_cp = crossroad;  // do not get stuck in more complex
        }
    }

    if (last_crossroad != cr_i && last_crossroad != cr_0 && prev_crossroad_cp == cr_I && crossroad == cr_0 && encoder_distance_mm(prev_0_pos, pos) >= conf.dead_end_dist)
    {
        // dead end
        DEBUG_crossroad->println(F("[D] last crossroad: i (dist)"));
        last_crossroad_updated = true;
        last_crossroad_position = pos;
        last_crossroad = cr_i;
    }
    if (last_crossroad != cr_0 && prev_crossroad == cr_0 && encoder_distance_mm(prev_0_pos, pos) >= 100)
    {
        DEBUG_crossroad->println(F("[D] last crossroad: 0 (dist)"));
        last_crossroad_updated = true;
        last_crossroad_position = pos;
        last_crossroad = cr_0;
    }
    else if (last_crossroad != cr_F && prev_crossroad == cr_T && encoder_distance_mm(prev_T_pos, pos) >= 50)
    {
        DEBUG_crossroad->println(F("[D] last crossroad: F (dist)"));
        last_crossroad_updated = true;
        last_crossroad_position = pos;
        last_crossroad = cr_F;
    }
}


#ifndef UNIT_TEST
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
#endif

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
 * Return crossroad read from sensor, no debouncing.
 */
crossroad_t line_follower_crossroad_fast()
{
    return decode_crossroad(line_follower_state());
}


/**
 * Return last valid crossroad.
 * This is only updated once the crossroad is passed.
 */
crossroad_t line_follower_last_crossroad()
{
    return last_crossroad;
}


encoder_position_t line_follower_last_crossroad_position()
{
    return last_crossroad_position;
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


/// Reset crossroad detection. This should be called when on an 'I' crossroad.
void line_follower_clear()
{
    DEBUG_crossroad->println(F("[D] line_follower_clear"));
    last_crossroad_updated = false;
    prev_crossroad_cp = cr_I;
    encoder_position_t pos = encoder_position();
    prev_0_pos = pos;  // do not detect cr_i right after turning
    prev_nontrivial_pos = pos;
    handled = true;
    // TODO cr_0 (dist) won't work after clear until another crossroad is detected
}


void line_follower_follow(uint8_t speed)
{
    following = true;
#ifndef UNIT_TEST
    RGBLineFollower.setKp(conf.line_Kp);
#endif
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
