#include "line_follower.h"
#include <Arduino.h>
#include <MeRGBLineFollower.h>
#include "hardware.h"
#include "conf.h"
#include "motor.h"
#include "debug.h"

static MeRGBLineFollower RGBLineFollower(PORT_9);

static crossroad_t crossroad = cr_0;
static crossroad_t last_crossroad = cr_0;
static crossroad_t prev_crossroad = cr_0;
static bool last_crossroad_updated = false;
static uint8_t state_debounced = 0b1111;
static encoder_position_t last_crossroad_position;
static encoder_position_t prev_0_pos;
static encoder_position_t prev_T_pos;

static bool following = false;
#define PID_LINE_TS 20UL
static uint8_t base_speed;


void line_follower_init()
{
    RGBLineFollower.begin();
    // CLI readings will be multiplied by default Kp=0.3 until
    // line_follower_follow() is first called.
    //RGBLineFollower.setKp(1.0);
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
            motor_move_lin(base_speed+u, base_speed-u);
            controller_prev_millis = now;
        }
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


    // never change from complex prev_crossroad to simple
    if (!(prev_crossroad == cr_T && (crossroad == cr_G || crossroad == cr_7 )))
    {
        if (prev_crossroad != cr_0 && crossroad == cr_0)
        {
            prev_0_pos = encoder_position();
        }
        else if (prev_crossroad != cr_T && crossroad == cr_T)
        {
            prev_T_pos = encoder_position();
        }
        prev_crossroad = crossroad;
    }

    // detekce krizovatek
    // 0 je cara
    switch (state_debounced)
    {
        case 0b0001:
        case 0b0011:
        case 0b0111:
            crossroad = cr_G;
            break;

        case 0b1000:
        case 0b1100:
        case 0b1110:
            crossroad = cr_7;
            break;

        case 0b0000:
            crossroad = cr_T;
            break;

        case 0b1001:
        case 0b1101:
        case 0b1011:
            crossroad = cr_I;
            break;

        default:
            // invalid
            crossroad = cr_0;
            break;
    }

    if (prev_crossroad != crossroad)
    {
        DEBUG_crossroad->print(F("[D] crossroad: "));
        DEBUG_crossroad->write(crossroad);
        DEBUG_crossroad->print('\t');
        DEBUG_crossroad->println(state_debounced, BIN);

        if (crossroad == cr_I || crossroad == cr_0)
        {
            last_crossroad_updated = true;
            if (crossroad == cr_0)
            {
                last_crossroad = prev_crossroad;
            }
            else
            {
                switch (prev_crossroad)
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
                        last_crossroad = cr_0;
                        break;
                    default:
                        Serial.print(F("[E] weird prev_crossroad: "));
                        Serial.write(prev_crossroad);
                        Serial.println();
                        break;
                }
            }
            last_crossroad_position = encoder_position();
            DEBUG_crossroad->print(F("[D] last crossroad: "));
            DEBUG_crossroad->write(last_crossroad);
            DEBUG_crossroad->println();

            if (crossroad == cr_0) prev_0_pos = encoder_position();
            prev_crossroad = crossroad;  // do not get stuck in more complex
        }
    }
    else if (last_crossroad == cr_I && prev_crossroad == cr_0 && encoder_distance_mm(prev_0_pos, encoder_position()) >= 80)
    {
        // dead end
        DEBUG_crossroad->println(F("[D] last crossroad: i (dist)"));
        last_crossroad_updated = true;
        last_crossroad = cr_i;
    }
    else if (last_crossroad != cr_0 && prev_crossroad == cr_0 && encoder_distance_mm(prev_0_pos, encoder_position()) >= 100)
    {
        DEBUG_crossroad->println(F("[D] last crossroad: 0 (dist)"));
        last_crossroad_updated = true;
        last_crossroad = cr_0;
    }
    else if (last_crossroad != cr_F && prev_crossroad == cr_T && encoder_distance_mm(prev_T_pos, encoder_position()) >= 80)
    {
        DEBUG_crossroad->println(F("[D] last crossroad: F (dist)"));
        last_crossroad_updated = true;
        last_crossroad = cr_F;
    }
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
    prev_crossroad = cr_I;
}


void line_follower_follow(uint8_t speed)
{
    following = true;
    RGBLineFollower.setKp(conf.line_Kp);
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
