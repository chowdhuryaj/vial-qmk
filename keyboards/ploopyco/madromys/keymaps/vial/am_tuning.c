// SPDX-License-Identifier: GPL-2.0-or-later
//
// See am_tuning.h.

#include QMK_KEYBOARD_H
#include "pointing_device_auto_mouse.h"
#include "pointing_device_internal.h"
#include "am_tuning.h"
#include <stdlib.h>

static uint16_t am_threshold = AUTO_MOUSE_THRESHOLD;

uint16_t am_tuning_get_threshold(void) {
    return am_threshold;
}

void am_tuning_set_threshold(uint16_t threshold) {
    if (threshold > 0) {
        am_threshold = threshold;
    }
}

// Modifier-adjusted step (Ctrl x10, Shift inverts).
static int32_t am_mod_step(uint32_t step) {
    const uint8_t mod_mask = get_mods();
    int32_t       s        = (int32_t)step;
    if (mod_mask & MOD_MASK_CTRL) s *= 10;
    if (mod_mask & MOD_MASK_SHIFT) s *= -1;
    return s;
}

void am_tuning_threshold_increment(void) {
    int32_t new_threshold = (int32_t)am_threshold + am_mod_step(AUTO_MOUSE_THRESHOLD_STEP);
    if (new_threshold < 1) new_threshold = 1; // floor: 0 would activate on any non-zero report
    am_threshold = (uint16_t)new_threshold;
    pd_dprintf("AM: threshold=%u\n", am_threshold);
}

void am_tuning_timeout_increment(void) {
    int32_t new_timeout = (int32_t)get_auto_mouse_timeout() + am_mod_step(AUTO_MOUSE_TIME_STEP);
    if (new_timeout < 50) new_timeout = 50; // floor: avoid the layer flickering off near-instantly
    set_auto_mouse_timeout((uint16_t)new_timeout);
    pd_dprintf("AM: timeout=%u\n", (uint16_t)new_timeout);
}

void am_tuning_toggle_enabled(void) {
    set_auto_mouse_enable(!get_auto_mouse_enable());
}

// Overrides the weak default in pointing_device_auto_mouse.c, which performs
// the same accumulate-and-compare but against the compile-time
// AUTO_MOUSE_THRESHOLD macro instead of the mutable am_threshold above.
// Resets accumulated movement on the same condition the core uses
// (is_auto_mouse_active(), read before the core updates status.is_activated
// with this call's return value) so stale drift doesn't bleed across an
// already-engaged period the way the core's own internal accumulator avoids.
bool auto_mouse_activation(report_mouse_t mouse_report) {
    static int32_t  total_x = 0, total_y = 0, total_h = 0, total_v = 0;
    static uint16_t last_debug_time = 0;
    total_x += mouse_report.x;
    total_y += mouse_report.y;
    total_h += mouse_report.h;
    total_v += mouse_report.v;

    bool activated = abs(total_x) > (int32_t)am_threshold || abs(total_y) > (int32_t)am_threshold ||
                      abs(total_h) > (int32_t)am_threshold || abs(total_v) > (int32_t)am_threshold ||
                      mouse_report.buttons;

    // Live tuning feedback (qmk console): throttled while accumulating below
    // threshold, and once more the instant it crosses. See config.h's
    // POINTING_DEVICE_DEBUG for how to view this.
    if (!activated && !is_auto_mouse_active() && timer_elapsed(last_debug_time) > 100) {
        pd_dprintf("AM: x=%ld y=%ld h=%ld v=%ld thr=%u\n", (long)total_x, (long)total_y, (long)total_h, (long)total_v, am_threshold);
        last_debug_time = timer_read();
    }
    if (activated) {
        pd_dprintf("AM: ACTIVATED thr=%u total=(%ld,%ld,%ld,%ld)\n", am_threshold, (long)total_x, (long)total_y, (long)total_h, (long)total_v);
    }

    if (activated || is_auto_mouse_active()) {
        total_x = total_y = total_h = total_v = 0;
    }
    return activated;
}
