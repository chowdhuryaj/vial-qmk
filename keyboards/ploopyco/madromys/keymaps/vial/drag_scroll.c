// Copyright 2025 Christopher Courtney, aka Drashna Jael're (@drashna)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Fresh port of drashna's `drag_scroll` module. See drag_scroll.h. The scrolling
// math in drag_scroll_apply() is drashna's canonical pointing_device_task_drag_scroll
// verbatim: the running sum (x + remainder) is formed inline at full int width for
// BOTH the divide and the modulo, and only the modulo (always bounded by the
// divisor) is stored back into the int8 remainder. That is what keeps a firm,
// fast roll from corrupting the wheel output under MOUSE_EXTENDED_REPORT (where
// mouse_report.x/y are int16). Everything below drag_scroll_apply() is
// keymap-only: invert, force-on via host lock LEDs, and live divisor tuning.

#include QMK_KEYBOARD_H
#include "drag_scroll.h"

static bool set_scrolling = false;

// Invert scroll output (toggled by DRG_INV). Default from DRAG_SCROLL_DEFAULT_INVERTED.
static bool scroll_inverted = DRAG_SCROLL_DEFAULT_INVERTED;

// Accumulated fractional scroll values.
static int8_t scroll_remainder_h = 0;
static int8_t scroll_remainder_v = 0;

static int8_t scroll_divisor_h = (int8_t)SCROLL_DIVISOR_H;
static int8_t scroll_divisor_v = (int8_t)SCROLL_DIVISOR_V;

report_mouse_t drag_scroll_apply(report_mouse_t mouse_report) {
    if (set_scrolling) {
        // drashna canonical: form (x + remainder) inline (promoted to int, so no
        // int8 truncation of the sum), divide for the wheel tick, and store back
        // only the modulo — which always fits int8.
        mouse_report.h = (mouse_report.x + scroll_remainder_h) / scroll_divisor_h;
        mouse_report.v = (mouse_report.y + scroll_remainder_v) / scroll_divisor_v;

        scroll_remainder_h = (mouse_report.x + scroll_remainder_h) % scroll_divisor_h;
        scroll_remainder_v = (mouse_report.y + scroll_remainder_v) % scroll_divisor_v;

        // Movement is consumed into scrolling, so clear X/Y.
        mouse_report.x = 0;
        mouse_report.y = 0;

        // Keymap extension: invert scroll output as the very last step. This runs
        // AFTER the divide-and-remainder bookkeeping above, so scroll_remainder_h/v
        // stay based on un-inverted raw accumulation — toggling invert mid-scroll
        // can't corrupt the fractional carry.
        if (scroll_inverted) {
            mouse_report.h = -mouse_report.h;
            mouse_report.v = -mouse_report.v;
        }
    } else {
        // Clear leftover remainders when not scrolling.
        scroll_remainder_h = 0;
        scroll_remainder_v = 0;
    }
    return mouse_report;
}

int8_t get_drag_scroll_h_divisor(void) {
    return scroll_divisor_h;
}
int8_t get_drag_scroll_v_divisor(void) {
    return scroll_divisor_v;
}
// Divisors are denominators — clamp so a raw setter call (e.g. from the HID
// tuning path) can never install 0 and divide-by-zero the scroll math.
static int8_t clamp_divisor(int8_t divisor) {
    if (divisor < SCROLL_DIVISOR_MIN) return SCROLL_DIVISOR_MIN;
    if (divisor > SCROLL_DIVISOR_MAX) return SCROLL_DIVISOR_MAX;
    return divisor;
}
void set_drag_scroll_h_divisor(int8_t divisor) {
    scroll_divisor_h = clamp_divisor(divisor);
}
void set_drag_scroll_v_divisor(int8_t divisor) {
    scroll_divisor_v = clamp_divisor(divisor);
}
void set_drag_scroll_divisor(int8_t divisor) {
    scroll_divisor_h = clamp_divisor(divisor);
    scroll_divisor_v = clamp_divisor(divisor);
}

// Modifier-adjusted step (Ctrl x10, Shift inverts).
static int16_t scroll_mod_step(int16_t step) {
    const uint8_t mod_mask = get_mods();
    if (mod_mask & MOD_MASK_CTRL) step *= 10;
    if (mod_mask & MOD_MASK_SHIFT) step *= -1;
    return step;
}

void drag_scroll_divisor_increment(void) {
    int16_t step  = scroll_mod_step((int16_t)SCROLL_DIVISOR_STEP);
    int16_t new_h = (int16_t)scroll_divisor_h + step;
    int16_t new_v = (int16_t)scroll_divisor_v + step;
    if (new_h < SCROLL_DIVISOR_MIN) new_h = SCROLL_DIVISOR_MIN;
    if (new_h > SCROLL_DIVISOR_MAX) new_h = SCROLL_DIVISOR_MAX;
    if (new_v < SCROLL_DIVISOR_MIN) new_v = SCROLL_DIVISOR_MIN;
    if (new_v > SCROLL_DIVISOR_MAX) new_v = SCROLL_DIVISOR_MAX;
    scroll_divisor_h = (int8_t)new_h;
    scroll_divisor_v = (int8_t)new_v;
}

bool get_drag_scroll_scrolling(void) {
    return set_scrolling;
}

__attribute__((weak)) bool set_drag_scroll_scrolling_user(bool scrolling) {
    return true;
}

void set_drag_scroll_scrolling(bool scrolling) {
    set_scrolling = scrolling;
    set_drag_scroll_scrolling_user(scrolling);
}

bool get_drag_scroll_inverted(void) {
    return scroll_inverted;
}
void set_drag_scroll_inverted(bool inverted) {
    scroll_inverted = inverted;
}
void drag_scroll_toggle_inverted(void) {
    scroll_inverted = !scroll_inverted;
}
