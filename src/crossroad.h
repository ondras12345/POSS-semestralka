#pragma once

typedef enum {
    cr_0 = '0', // nevim / neinicializovano / mimo caru / nevalidni
    cr_I = 'I', // rovne
    cr_G = 'G', // doprava
    cr_7 = '7', // doleva
    cr_T = 'T', // doleva a doprava
    cr_E = 'E', // doprava a rovne
    cr_3 = '3', // doleva a rovne
    cr_X = 'X', // +
    cr_i = 'i', // dead end
    cr_F = 'F', // finish
} crossroad_t;


typedef enum {
    crd_straight = 'I',
    crd_left = 'L',
    crd_right = 'R',
    crd_back = 'B',  ///< go back the same direction you came from
    crd_invalid = '0',
} crossroad_direction_t;


bool crossroad_direction_valid(crossroad_t, crossroad_direction_t);

crossroad_t crossroad_rotate(crossroad_t, crossroad_direction_t);
crossroad_direction_t crossroad_direction_rotate(crossroad_direction_t rotate_me, crossroad_direction_t from);
