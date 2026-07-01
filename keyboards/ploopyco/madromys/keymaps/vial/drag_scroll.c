// Copyright 2025 Christopher Courtney, aka Drashna Jael're (@drashna)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from drashna's `drag_scroll` community module. See drag_scroll.h.

#include QMK_KEYBOARD_H
#include "drag_scroll.h"

static bool set_scrolling = false;

// When true, drag scroll is forced on and any request to turn it off is ignored.
// Driven by the keymap from the host lock-LED state (see set_drag_scroll_force()).
static bool force_scrolling = false;

// Invert scroll output (toggled by DRG_INV). Default from DRAG_SCROLL_DEFAULT_INVERTED.
static bool scroll_inverted = DRAG_SCROLL_DEFAULT_INVERTED;

// Accumulated fractional scroll values
static int8_t scroll_remainder_h = 0;
static int8_t scroll_remainder_v = 0;

static int8_t scroll_divisor_h = (int8_t)SCROLL_DIVISOR_H;
static int8_t scroll_divisor_v = (int8_t)SCROLL_DIVISOR_V;

report_mouse_t drag_scroll_apply(report_mouse_t mouse_report) {
    if (set_scrolling) {
        scroll_remainder_h += mouse_report.x;
        scroll_remainder_v += mouse_report.y;

        mouse_report.h = scroll_remainder_h / scroll_divisor_h;
        mouse_report.v = scroll_remainder_v / scroll_divisor_v;

        // Keep the fractional remainder for the next report
        scroll_remainder_h %= scroll_divisor_h;
        scroll_remainder_v %= scroll_divisor_v;

        // Movement is consumed into scrolling, so clear X/Y
        mouse_report.x = 0;
        mouse_report.y = 0;

        // Invert scroll output as the very last step. This runs AFTER the
        // divide-and-remainder bookkeeping above, so scroll_remainder_h/v stay
        // based on un-inverted raw accumulation — toggling invert mid-scroll
        // can't corrupt the fractional carry.
        if (scroll_inverted) {
            mouse_report.h = -mouse_report.h;
            mouse_report.v = -mouse_report.v;
        }
    } else {
        // Clear leftover remainders when not scrolling
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
void set_drag_scroll_h_divisor(int8_t divisor) {
    scroll_divisor_h = divisor;
}
void set_drag_scroll_v_divisor(int8_t divisor) {
    scroll_divisor_v = divisor;
}
void set_drag_scroll_divisor(int8_t divisor) {
    scroll_divisor_h = divisor;
    scroll_divisor_v = divisor;
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
    // While forced on (host lock LED active), ignore any request to turn off.
    if (force_scrolling && !scrolling) {
        return;
    }
    set_scrolling = scrolling;
    set_drag_scroll_scrolling_user(scrolling);
}

void set_drag_scroll_force(bool force) {
    force_scrolling = force;
    if (force) {
        set_drag_scroll_scrolling(true); // force on immediately
    }
    // Releasing the force does not turn scrolling off here; the caller decides.
}

bool get_drag_scroll_force(void) {
    return force_scrolling;
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
