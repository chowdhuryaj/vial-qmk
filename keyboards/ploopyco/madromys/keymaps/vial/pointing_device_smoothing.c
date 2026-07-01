// Copyright 2025 Christopher Courtney, aka Drashna Jael're (@drashna)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from drashna's `pointing_device_smoothing` community module. See
// pointing_device_smoothing.h.

#include QMK_KEYBOARD_H
#include "pointing_device.h"
#include "pointing_device_smoothing.h"

static bool     smooth_enabled       = true;
static float    smooth_factor        = POINTING_DEVICE_SMOOTHING_FACTOR;
static uint32_t smooth_reset_timeout = POINTING_DEVICE_SMOOTHING_RESET_TIMEOUT_MS;

// Exponential moving average state
static float ema_x = 0.0f;
static float ema_y = 0.0f;

// Rounding carry to reduce precision loss when converting float -> int
static float carry_x = 0.0f;
static float carry_y = 0.0f;

static uint32_t smooth_timer = 0;

float pointing_device_smoothing_get_factor(void) {
    return smooth_factor;
}

void pointing_device_smoothing_set_factor(float factor) {
    if (factor >= 0.0f && factor <= 1.0f) {
        smooth_factor = factor;
    }
}

uint32_t pointing_device_smoothing_get_reset_timeout(void) {
    return smooth_reset_timeout;
}

void pointing_device_smoothing_set_reset_timeout(uint32_t timeout_ms) {
    smooth_reset_timeout = timeout_ms;
}

bool pointing_device_smoothing_get_enabled(void) {
    return smooth_enabled;
}

void pointing_device_smoothing_set_enabled(bool enable) {
    smooth_enabled = enable;
    if (!enable) {
        // Reset state when disabling so stale EMA doesn't bleed into next enable
        ema_x   = 0.0f;
        ema_y   = 0.0f;
        carry_x = 0.0f;
        carry_y = 0.0f;
    }
}

void pointing_device_smoothing_toggle_enabled(void) {
    pointing_device_smoothing_set_enabled(!smooth_enabled);
}

// Modifier-adjusted float step (Ctrl x10, Shift inverts).
static float smoothing_mod_step_f(float step) {
    const uint8_t mod_mask = get_mods();
    if (mod_mask & MOD_MASK_CTRL) {
        step *= 10.0f;
    }
    if (mod_mask & MOD_MASK_SHIFT) {
        step *= -1.0f;
    }
    return step;
}

// Modifier-adjusted integer step (Ctrl x10, Shift inverts).
static int32_t smoothing_mod_step_i32(uint32_t step) {
    const uint8_t mod_mask = get_mods();
    int32_t       s        = (int32_t)step;
    if (mod_mask & MOD_MASK_CTRL) {
        s *= 10;
    }
    if (mod_mask & MOD_MASK_SHIFT) {
        s *= -1;
    }
    return s;
}

void pointing_device_smoothing_factor_increment(void) {
    pointing_device_smoothing_set_factor(smooth_factor + smoothing_mod_step_f(POINTING_DEVICE_SMOOTHING_FACTOR_STEP));
}

void pointing_device_smoothing_timeout_increment(void) {
    int32_t step        = smoothing_mod_step_i32(POINTING_DEVICE_SMOOTHING_TIMEOUT_STEP);
    int32_t new_timeout = (int32_t)smooth_reset_timeout + step;
    if (new_timeout < 0) new_timeout = 0;
    pointing_device_smoothing_set_reset_timeout((uint32_t)new_timeout);
}

report_mouse_t pointing_device_smoothing_apply(report_mouse_t mouse_report) {
    if (!smooth_enabled) {
        return mouse_report;
    }

    // Reset EMA after a period of inactivity
    if (mouse_report.x == 0 && mouse_report.y == 0) {
        if (timer_elapsed32(smooth_timer) > smooth_reset_timeout) {
            ema_x   = 0.0f;
            ema_y   = 0.0f;
            carry_x = 0.0f;
            carry_y = 0.0f;
        }
        return mouse_report;
    }

    smooth_timer = timer_read32();

    // Reset carry when direction reverses to follow the user's hand
    if (mouse_report.x * carry_x < 0) carry_x = 0.0f;
    if (mouse_report.y * carry_y < 0) carry_y = 0.0f;

    // Apply EMA
    ema_x = smooth_factor * (float)mouse_report.x + (1.0f - smooth_factor) * ema_x;
    ema_y = smooth_factor * (float)mouse_report.y + (1.0f - smooth_factor) * ema_y;

    // Accumulate rounding carry to avoid dropping sub-pixel movement
    float out_x = ema_x + carry_x;
    float out_y = ema_y + carry_y;

    mouse_xy_report_t int_x = (mouse_xy_report_t)CONSTRAIN_HID_XY(out_x);
    mouse_xy_report_t int_y = (mouse_xy_report_t)CONSTRAIN_HID_XY(out_y);

    carry_x = out_x - (float)int_x;
    carry_y = out_y - (float)int_y;

    mouse_report.x = int_x;
    mouse_report.y = int_y;

    return mouse_report;
}
