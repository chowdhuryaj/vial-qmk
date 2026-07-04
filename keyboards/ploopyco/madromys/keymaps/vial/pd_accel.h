// Copyright 2024 burkfers (@burkfers)
// Copyright 2024 Wimads (@wimads)
// Copyright 2021 Christopher Courtney, aka Drashna Jael're (@drashna)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from drashna's `pointing_device_accel` community module into a
// self-contained keymap module. The community-module plumbing (API-version
// assert, `_kb` call chaining, custom keycodes and EEPROM/VIA integration) has
// been removed; the keymap drives this through pd_accel_apply() and the
// setter/increment helpers.

#pragma once

#include "action.h"
#include "report.h"

// lower/higher value = curve starts more smoothly/abruptly
#ifndef POINTING_DEVICE_ACCEL_TAKEOFF
#    define POINTING_DEVICE_ACCEL_TAKEOFF 2.0f
#endif
// lower/higher value = curve reaches its upper limit slower/faster
#ifndef POINTING_DEVICE_ACCEL_GROWTH_RATE
#    define POINTING_DEVICE_ACCEL_GROWTH_RATE 0.25f
#endif
// lower/higher value = acceleration kicks in earlier/later
#ifndef POINTING_DEVICE_ACCEL_OFFSET
#    define POINTING_DEVICE_ACCEL_OFFSET 2.2f
#endif
// lower limit of accel curve (minimum acceleration factor)
#ifndef POINTING_DEVICE_ACCEL_LIMIT
#    define POINTING_DEVICE_ACCEL_LIMIT 0.2f
#endif

// HID tuning clamp ranges (also enforced by the setters below, not just the
// wire handler in keymap.c — so a keymap.c bug can't smuggle an out-of-range
// value past the module).
#ifndef POINTING_DEVICE_ACCEL_TAKEOFF_MIN
#    define POINTING_DEVICE_ACCEL_TAKEOFF_MIN 0.5f
#endif
#ifndef POINTING_DEVICE_ACCEL_TAKEOFF_MAX
#    define POINTING_DEVICE_ACCEL_TAKEOFF_MAX 10.0f
#endif
#ifndef POINTING_DEVICE_ACCEL_GROWTH_RATE_MAX
#    define POINTING_DEVICE_ACCEL_GROWTH_RATE_MAX 2.0f
#endif
#ifndef POINTING_DEVICE_ACCEL_OFFSET_MIN
#    define POINTING_DEVICE_ACCEL_OFFSET_MIN -10.0f
#endif
#ifndef POINTING_DEVICE_ACCEL_OFFSET_MAX
#    define POINTING_DEVICE_ACCEL_OFFSET_MAX 10.0f
#endif
#ifndef POINTING_DEVICE_ACCEL_LIMIT_MAX
#    define POINTING_DEVICE_ACCEL_LIMIT_MAX 1.0f
#endif
// milliseconds to wait between requesting the device's current DPI
#ifndef POINTING_DEVICE_ACCEL_CPI_THROTTLE_MS
#    define POINTING_DEVICE_ACCEL_CPI_THROTTLE_MS 200
#endif
// upper limit of accel curve, recommended to leave at 1; adjust DPI setting instead.
#ifndef POINTING_DEVICE_ACCEL_LIMIT_UPPER
#    define POINTING_DEVICE_ACCEL_LIMIT_UPPER 1
#endif
// milliseconds after which to reset quantization error correction (forget rounding remainder)
#ifndef POINTING_DEVICE_ACCEL_ROUNDING_CARRY_TIMEOUT_MS
#    define POINTING_DEVICE_ACCEL_ROUNDING_CARRY_TIMEOUT_MS 200
#endif

#ifndef POINTING_DEVICE_ACCEL_TAKEOFF_STEP
#    define POINTING_DEVICE_ACCEL_TAKEOFF_STEP 0.01f
#endif
#ifndef POINTING_DEVICE_ACCEL_GROWTH_RATE_STEP
#    define POINTING_DEVICE_ACCEL_GROWTH_RATE_STEP 0.01f
#endif
#ifndef POINTING_DEVICE_ACCEL_OFFSET_STEP
#    define POINTING_DEVICE_ACCEL_OFFSET_STEP 0.1f
#endif
#ifndef POINTING_DEVICE_ACCEL_LIMIT_STEP
#    define POINTING_DEVICE_ACCEL_LIMIT_STEP 0.01f
#endif

typedef struct pd_accel_config_t {
    bool  enabled;
    float growth_rate;
    float offset;
    float limit;
    float takeoff;
} pd_accel_config_t;

extern pd_accel_config_t g_pd_accel_config;

// Initialise the in-memory config to the compile-time defaults.
void pd_accel_init(void);

// Apply the acceleration curve to a mouse report. No-op when disabled or report is zero.
report_mouse_t pd_accel_apply(report_mouse_t mouse_report);

// Returns a modifier-adjusted step (Ctrl x10, Shift inverts).
float pd_accel_get_mod_step(float step);

void pd_accel_set_enabled(bool enable);
bool pd_accel_get_enabled(void);
void pd_accel_toggle_enabled(void);

float pd_accel_get_takeoff(void);
void  pd_accel_set_takeoff(float val);
void  pd_accel_takeoff_increment(void);

float pd_accel_get_growth_rate(void);
void  pd_accel_set_growth_rate(float val);
void  pd_accel_growth_rate_increment(void);

float pd_accel_get_offset(void);
void  pd_accel_set_offset(float val);
void  pd_accel_offset_increment(void);

float pd_accel_get_limit(void);
void  pd_accel_set_limit(float val);
void  pd_accel_limit_increment(void);
