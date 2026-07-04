// Copyright 2024 @pandrr
// Copyright 2025 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
// SPDX-License-Identifier: GPL-2.0-or-later
//
// See wiggle_ball.h.

#include QMK_KEYBOARD_H
#include "wiggle_ball.h"
#include "drag_scroll.h"
#include "pd_gestures.h"
#include "pointing_device_internal.h"

// Runtime detection parameters. Seeded from the compile-time defaults;
// keyboard_post_init_user overwrites them with the EEPROM-persisted values,
// and the companion app adjusts them live over raw HID.
static uint16_t wb_direction_switch_timeout = WIGGLE_BALL_DIRECTION_SWITCH_TIMEOUT;
static uint16_t wb_cooldown                 = WIGGLE_BALL_TIMEOUT;
static uint8_t  wb_movement_threshold       = WIGGLE_BALL_MOVEMENT_THRESHOLD;
static bool     wb_enabled                  = WIGGLE_BALL_ENABLED_DEFAULT;

bool get_wiggle_ball_enabled(void) {
    return wb_enabled;
}

void set_wiggle_ball_enabled(bool enabled) {
    wb_enabled = enabled;
}

uint16_t get_wiggle_ball_direction_switch_timeout(void) {
    return wb_direction_switch_timeout;
}

void set_wiggle_ball_direction_switch_timeout(uint16_t ms) {
    if (ms < WIGGLE_BALL_INTERVAL_MIN) ms = WIGGLE_BALL_INTERVAL_MIN;
    if (ms > WIGGLE_BALL_INTERVAL_MAX) ms = WIGGLE_BALL_INTERVAL_MAX;
    wb_direction_switch_timeout = ms;
}

uint16_t get_wiggle_ball_timeout(void) {
    return wb_cooldown;
}

void set_wiggle_ball_timeout(uint16_t ms) {
    if (ms < WIGGLE_BALL_COOLDOWN_MIN) ms = WIGGLE_BALL_COOLDOWN_MIN;
    if (ms > WIGGLE_BALL_COOLDOWN_MAX) ms = WIGGLE_BALL_COOLDOWN_MAX;
    wb_cooldown = ms;
}

uint8_t get_wiggle_ball_movement_threshold(void) {
    return wb_movement_threshold;
}

void set_wiggle_ball_movement_threshold(uint8_t counts) {
    if (counts > WIGGLE_BALL_THRESHOLD_MAX) counts = WIGGLE_BALL_THRESHOLD_MAX;
    wb_movement_threshold = counts;
}

report_mouse_t wiggle_ball_apply(report_mouse_t mouse_report) {
    static uint8_t  shake_count         = 0;
    static bool     last_direction      = false;
    static uint16_t last_toggle_time    = 0;
    static uint16_t last_switch_time    = 0;

    // Kill switch (runtime + HID-tunable): pass reports through untouched so
    // an accidental-toggle-prone setup can turn shake detection off entirely.
    if (!wb_enabled) {
        return mouse_report;
    }

    // Post-toggle cooldown: skip detection entirely for wb_cooldown ms
    // after the last toggle, so the same shake can't immediately re-trigger.
    if (timer_elapsed(last_toggle_time) > wb_cooldown) {
        // A reversal gap wider than the switch timeout means the shake stalled;
        // start counting fresh.
        if (timer_elapsed(last_switch_time) > wb_direction_switch_timeout) {
            shake_count = 0;
        }

        // Always read the raw ball axes, never the wheel output: this must run
        // before drag_scroll_apply() in the pipeline (see wiggle_ball.h) so x/y
        // are the actual sensor deltas even while scrolling. Reading h/v instead
        // while scrolling was the bug - divided by SCROLL_DIVISOR_H/V (40/64),
        // the wheel output almost never exceeds magnitude 1, so the >1/<-1 shake
        // check below could essentially never fire and scrolling couldn't be
        // wiggled back off.
        bool              scrolling  = get_drag_scroll_scrolling();
        mouse_xy_report_t x_movement = mouse_report.x;
        mouse_xy_report_t y_movement = mouse_report.y;

        if (x_movement > 1 && y_movement < wb_movement_threshold && !last_direction) {
            shake_count++;
            last_switch_time = timer_read();
            last_direction   = !last_direction;
        }
        if (x_movement < -1 && y_movement < wb_movement_threshold && last_direction) {
            shake_count++;
            last_switch_time = timer_read();
            last_direction   = !last_direction;
        }

        if (shake_count > 3) {
            // A shake while a gesture is open cancels the gesture and is
            // consumed there (it does NOT also toggle drag scroll); otherwise a
            // shake toggles drag scroll as usual. This is why wiggle runs before
            // pd_gestures_apply() in the pipeline - a live gesture zeroes x/y, so
            // the shake has to be detected before the gesture swallows it.
            if (pd_gestures_is_active()) {
                pd_gestures_cancel();
                pd_dprintf("Wiggle: gesture cancelled\n");
            } else {
                set_drag_scroll_scrolling(!scrolling);
                pd_dprintf("Wiggle: scrolling=%d\n", (int)!scrolling);
            }
            shake_count      = 0;
            last_toggle_time = timer_read();
            last_switch_time = timer_read();
        }
    }

    return mouse_report;
}
