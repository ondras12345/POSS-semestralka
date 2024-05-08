#pragma once

typedef enum {
    cr_0 = '0', // nevim / neinicializovano
    cr_I = 'I', // rovne
    cr_G = 'G', // doprava
    cr_7 = '7', // doleva
    cr_T = 'T', // doleva a doprava
    cr_E = 'E', // doprava a rovne
    cr_3 = '3', // doleva a rovne
    cr_X = 'X', // +
    cr_i = 'i', // dead end
} crossroad_t;


typedef enum {
    crd_straight = 'I',
    crd_left = 'L',
    crd_right = 'R',
} crossroad_direction_t;
