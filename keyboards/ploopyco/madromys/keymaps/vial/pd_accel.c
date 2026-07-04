// Copyright 2024 burkfers (@burkfers)
// Copyright 2024 Wimads (@wimads)
// Copyright 2021 Christopher Courtney, aka Drashna Jael're (@drashna)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from drashna's `pointing_device_accel` community module. See pd_accel.h.

#include QMK_KEYBOARD_H
#include "pointing_device.h"
#include "pointing_device_internal.h"
#include "pd_accel.h"
#include <math.h>

pd_accel_config_t g_pd_accel_config;

static uint32_t pd_accel_timer;

void pd_accel_init(void) {
    g_pd_accel_config = (pd_accel_config_t){
        .growth_rate = POINTING_DEVICE_ACCEL_GROWTH_RATE,
        .offset      = POINTING_DEVICE_ACCEL_OFFSET,
        .limit       = POINTING_DEVICE_ACCEL_LIMIT,
        .takeoff     = POINTING_DEVICE_ACCEL_TAKEOFF,
        .enabled     = true,
    };
}

float pd_accel_get_takeoff(void) {
    return g_pd_accel_config.takeoff;
}
void pd_accel_set_takeoff(float val) {
    if (val < POINTING_DEVICE_ACCEL_TAKEOFF_MIN) val = POINTING_DEVICE_ACCEL_TAKEOFF_MIN;
    if (val > POINTING_DEVICE_ACCEL_TAKEOFF_MAX) val = POINTING_DEVICE_ACCEL_TAKEOFF_MAX;
    g_pd_accel_config.takeoff = val;
}

float pd_accel_get_growth_rate(void) {
    return g_pd_accel_config.growth_rate;
}
void pd_accel_set_growth_rate(float val) {
    if (val < 0) val = 0;
    if (val > POINTING_DEVICE_ACCEL_GROWTH_RATE_MAX) val = POINTING_DEVICE_ACCEL_GROWTH_RATE_MAX;
    g_pd_accel_config.growth_rate = val;
}

float pd_accel_get_offset(void) {
    return g_pd_accel_config.offset;
}
void pd_accel_set_offset(float val) {
    if (val < POINTING_DEVICE_ACCEL_OFFSET_MIN) val = POINTING_DEVICE_ACCEL_OFFSET_MIN;
    if (val > POINTING_DEVICE_ACCEL_OFFSET_MAX) val = POINTING_DEVICE_ACCEL_OFFSET_MAX;
    g_pd_accel_config.offset = val;
}

float pd_accel_get_limit(void) {
    return g_pd_accel_config.limit;
}
void pd_accel_set_limit(float val) {
    if (val < 0) val = 0;
    if (val > POINTING_DEVICE_ACCEL_LIMIT_MAX) val = POINTING_DEVICE_ACCEL_LIMIT_MAX;
    g_pd_accel_config.limit = val;
}

void pd_accel_set_enabled(bool enable) {
    g_pd_accel_config.enabled = enable;
    pd_dprintf("PDACCEL: enabled: %d\n", g_pd_accel_config.enabled);
}
bool pd_accel_get_enabled(void) {
    return g_pd_accel_config.enabled;
}
void pd_accel_toggle_enabled(void) {
    pd_accel_set_enabled(!pd_accel_get_enabled());
}

report_mouse_t pd_accel_apply(report_mouse_t mouse_report) {
    // rounding carry to recycle dropped floats from int mouse reports, to smoothen low speed movements
    static float rounding_carry_x = 0;
    static float rounding_carry_y = 0;
    // time since last mouse report:
    const uint16_t delta_time = timer_elapsed32(pd_accel_timer);
    // skip accel maths if report = 0, or if accel not enabled.
    if ((mouse_report.x == 0 && mouse_report.y == 0) || !g_pd_accel_config.enabled) {
        return mouse_report;
    }
    // reset timer:
    pd_accel_timer = timer_read32();
    // Reset carry if too much time passed
    if (delta_time > POINTING_DEVICE_ACCEL_ROUNDING_CARRY_TIMEOUT_MS) {
        rounding_carry_x = 0;
        rounding_carry_y = 0;
    }
    // Reset carry when pointer swaps direction, to follow user's hand.
    if (mouse_report.x * rounding_carry_x < 0) rounding_carry_x = 0;
    if (mouse_report.y * rounding_carry_y < 0) rounding_carry_y = 0;
    // Limit expensive calls to get device cpi settings only when mouse stationary for > throttle ms.
    static uint16_t device_cpi = 300;
    if (delta_time > POINTING_DEVICE_ACCEL_CPI_THROTTLE_MS) {
        device_cpi = pointing_device_get_cpi();
    }
    // calculate dpi correction factor (for normalizing velocity range across different user dpi settings)
    const float dpi_correction = (float)1000.0f / device_cpi;
    // calculate euclidean distance moved (sqrt(x^2 + y^2))
    const float distance = sqrtf(mouse_report.x * mouse_report.x + mouse_report.y * mouse_report.y);
    // calculate delta velocity: dv = distance/dt
    const float velocity_raw = distance / delta_time;
    // correct raw velocity for dpi
    const float velocity = dpi_correction * velocity_raw;
    // letter variables for readability of maths:
    const float k = g_pd_accel_config.takeoff;
    const float g = g_pd_accel_config.growth_rate;
    const float s = g_pd_accel_config.offset;
    const float m = g_pd_accel_config.limit;
    // acceleration factor: f(v) = 1 - (1 - M) / {1 + e^[K(v - S)]}^(G/K):
    // Generalised Sigmoid Function, see https://www.desmos.com/calculator/k9vr0y2gev
    const float pd_accel_factor =
        POINTING_DEVICE_ACCEL_LIMIT_UPPER -
        (POINTING_DEVICE_ACCEL_LIMIT_UPPER - m) / powf(1 + expf(k * (velocity - s)), g / k);
    // multiply mouse reports by acceleration factor, and account for previous quantization errors:
    const float new_x = rounding_carry_x + pd_accel_factor * mouse_report.x;
    const float new_y = rounding_carry_y + pd_accel_factor * mouse_report.y;
    // Accumulate any difference from next integer (quantization).
    rounding_carry_x = new_x - (int)new_x;
    rounding_carry_y = new_y - (int)new_y;
    // clamp values
    mouse_report.x = (mouse_xy_report_t)CONSTRAIN_HID_XY(new_x);
    mouse_report.y = (mouse_xy_report_t)CONSTRAIN_HID_XY(new_y);

    return mouse_report;
}

float pd_accel_get_mod_step(float step) {
    const uint8_t mod_mask = get_mods();
    if (mod_mask & MOD_MASK_CTRL) {
        step *= 10; // control increases by factor 10
    }
    if (mod_mask & MOD_MASK_SHIFT) {
        step *= -1; // shift inverts
    }
    return step;
}

void pd_accel_takeoff_increment(void) {
    pd_accel_set_takeoff(pd_accel_get_takeoff() + pd_accel_get_mod_step(POINTING_DEVICE_ACCEL_TAKEOFF_STEP));
}
void pd_accel_growth_rate_increment(void) {
    pd_accel_set_growth_rate(pd_accel_get_growth_rate() + pd_accel_get_mod_step(POINTING_DEVICE_ACCEL_GROWTH_RATE_STEP));
}
void pd_accel_offset_increment(void) {
    pd_accel_set_offset(pd_accel_get_offset() + pd_accel_get_mod_step(POINTING_DEVICE_ACCEL_OFFSET_STEP));
}
void pd_accel_limit_increment(void) {
    pd_accel_set_limit(pd_accel_get_limit() + pd_accel_get_mod_step(POINTING_DEVICE_ACCEL_LIMIT_STEP));
}
