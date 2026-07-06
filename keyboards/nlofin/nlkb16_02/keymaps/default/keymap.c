/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Smoke-test keymap — stock-ish layer 0 only. Real keymap is keymaps/flask. */

#include QMK_KEYBOARD_H

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
                KC_1,    KC_2,    KC_3,    KC_4,     KC_MPLY,
                KC_5,    KC_6,    KC_7,    KC_8,     KC_TRNS,
                KC_9,    KC_0,    KC_UP,   KC_ENT,   KC_MUTE,
                KC_A,    KC_LEFT, KC_DOWN, KC_RIGHT
            ),
};

#ifdef ENCODER_MAP_ENABLE
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { ENCODER_CCW_CW(KC_MPRV, KC_MNXT), ENCODER_CCW_CW(KC_PGDN, KC_PGUP), ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
};
#endif
