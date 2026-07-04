// SPDX-License-Identifier: GPL-2.0-or-later
//
// Wheel chords — see wheel_chords.h. Direction binning and ratchet math
// mirror pd_gestures.c exactly so the two features feel identical.

#include QMK_KEYBOARD_H
#include "wheel_chords.h"
#include <math.h>

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

static uint16_t wc_table[WHEEL_CHORDS_BUTTONS][WHEEL_CHORDS_DIRECTIONS];
static bool     wc_enabled = WHEEL_CHORDS_ENABLED_DEFAULT;
static uint16_t wc_step    = WHEEL_CHORDS_STEP_DEFAULT;
static uint16_t wc_hold_ms = WHEEL_CHORDS_HOLD_MS_DEFAULT;
static uint8_t  wc_held    = 0; // bitmask, bit 0 = BTN1
static int32_t  wc_acc_x   = 0;
static int32_t  wc_acc_y   = 0;
static int8_t   wc_active_button = -1; // button whose table we're firing
static uint32_t wc_press_time    = 0;  // when the active button became active

bool wheel_chords_get_enabled(void) {
    return wc_enabled;
}

void wheel_chords_set_enabled(bool enabled) {
    wc_enabled = enabled;
    if (!enabled) {
        wc_active_button = -1;
        wc_acc_x = wc_acc_y = 0;
    }
}

uint16_t wheel_chords_get_step(void) {
    return wc_step;
}

void wheel_chords_set_step(uint16_t step) {
    if (step < WHEEL_CHORDS_STEP_MIN) step = WHEEL_CHORDS_STEP_MIN;
    if (step > WHEEL_CHORDS_STEP_MAX) step = WHEEL_CHORDS_STEP_MAX;
    wc_step = step;
}

uint16_t wheel_chords_get_hold_ms(void) {
    return wc_hold_ms;
}

void wheel_chords_set_hold_ms(uint16_t ms) {
    if (ms > WHEEL_CHORDS_HOLD_MS_MAX) ms = WHEEL_CHORDS_HOLD_MS_MAX;
    wc_hold_ms = ms;
}

uint16_t (*wheel_chords_table(void))[WHEEL_CHORDS_DIRECTIONS] {
    return wc_table;
}

static bool wc_button_has_slots(uint8_t button) {
    for (uint8_t d = 0; d < WHEEL_CHORDS_DIRECTIONS; d++) {
        if (wc_table[button][d] != KC_NO) return true;
    }
    return false;
}

void wheel_chords_on_button(uint8_t button, bool pressed) {
    if (button >= WHEEL_CHORDS_BUTTONS) return;
    if (pressed) {
        wc_held |= (1 << button);
    } else {
        wc_held &= ~(1 << button);
    }
    // Re-pick the capture target: lowest held button with configured slots.
    int8_t previous = wc_active_button;
    wc_active_button = -1;
    for (uint8_t b = 0; b < WHEEL_CHORDS_BUTTONS; b++) {
        if ((wc_held & (1 << b)) && wc_button_has_slots(b)) {
            wc_active_button = b;
            break;
        }
    }
    if (wc_active_button != previous) {
        wc_acc_x = wc_acc_y = 0; // fresh gesture per chord
        wc_press_time = timer_read32();
    }
}

bool wheel_chords_capturing(void) {
    if (!wc_enabled || wc_active_button < 0) return false;
    // Hold delay: motion during a quick click-drag passes through as normal
    // cursor movement; only a deliberate hold turns the ball into a chord.
    return wc_hold_ms == 0 || timer_elapsed32(wc_press_time) >= wc_hold_ms;
}

// Same squared-compare as pd_gestures (no sqrt, no 32-bit overflow).
static bool wc_reached(int32_t x, int32_t y, uint16_t thresh) {
    int64_t d2 = (int64_t)x * (int64_t)x + (int64_t)y * (int64_t)y;
    int64_t t2 = (int64_t)thresh * (int64_t)thresh;
    return d2 >= t2;
}

// 8-way bin, 45° sectors, E then clockwise (mouse +y = south). Identical to
// pd_gestures so a set migrated between the two features feels the same.
static uint8_t wc_direction(int32_t x, int32_t y) {
    float     r     = atan2f((float)y, (float)x);
    float     d     = 180.0f * r / (float)M_PI;
    int16_t   id    = (int16_t)d;
    const int split = WHEEL_CHORDS_DIRECTIONS;
    return ((id + 360 + 360 / split / 2) % 360 / (360 / split)) % split;
}

static void wc_fire(uint8_t button, uint8_t direction, int32_t x, int32_t y) {
    uint16_t keycode = wc_table[button][direction];
    // Empty diagonal falls back to the nearest cardinal by dominant axis
    // (pd_gestures rule) so 4-way chord sets keep the old 90°-sector feel.
    if (keycode == KC_NO && (direction & 1)) {
        uint8_t cardinal;
        int64_t ax = x < 0 ? -(int64_t)x : (int64_t)x;
        int64_t ay = y < 0 ? -(int64_t)y : (int64_t)y;
        if (ax >= ay) {
            cardinal = (x >= 0) ? 0 : 4; // E or W
        } else {
            cardinal = (y >= 0) ? 2 : 6; // S or N
        }
        keycode = wc_table[button][cardinal];
    }
    if (keycode != KC_NO) {
        tap_code16(keycode);
    }
}

void wheel_chords_feed(int16_t dx, int16_t dy) {
    if (!wheel_chords_capturing()) return;
    wc_acc_x += dx;
    wc_acc_y += dy;
    if (wc_reached(wc_acc_x, wc_acc_y, wc_step)) {
        wc_fire((uint8_t)wc_active_button, wc_direction(wc_acc_x, wc_acc_y), wc_acc_x, wc_acc_y);
        wc_acc_x = 0;
        wc_acc_y = 0;
    }
}

report_mouse_t wheel_chords_apply(report_mouse_t mouse_report) {
    if (!wheel_chords_capturing()) {
        return mouse_report;
    }
    wheel_chords_feed(mouse_report.x, mouse_report.y);
    mouse_report.x = 0;
    mouse_report.y = 0;
    return mouse_report;
}
