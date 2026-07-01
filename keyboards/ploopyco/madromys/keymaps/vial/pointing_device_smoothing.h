// Copyright 2025 Christopher Courtney, aka Drashna Jael're (@drashna)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from drashna's `pointing_device_smoothing` community module into a
// self-contained keymap module. Community-module plumbing and custom keycodes
// removed; the keymap drives this via pointing_device_smoothing_apply() and the
// setter/increment helpers.

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "report.h"

// 0.0 = maximum smoothing (slowest response), 1.0 = no smoothing (raw passthrough).
#ifndef POINTING_DEVICE_SMOOTHING_FACTOR
#    define POINTING_DEVICE_SMOOTHING_FACTOR 0.4f
#endif

// Milliseconds of inactivity after which the EMA state is reset.
#ifndef POINTING_DEVICE_SMOOTHING_RESET_TIMEOUT_MS
#    define POINTING_DEVICE_SMOOTHING_RESET_TIMEOUT_MS 200
#endif

// Step applied per keypress when adjusting the smooth factor. Shift inverts, Ctrl x10.
#ifndef POINTING_DEVICE_SMOOTHING_FACTOR_STEP
#    define POINTING_DEVICE_SMOOTHING_FACTOR_STEP 0.05f
#endif

// Step (ms) applied per keypress when adjusting the reset timeout. Shift inverts, Ctrl x10.
#ifndef POINTING_DEVICE_SMOOTHING_TIMEOUT_STEP
#    define POINTING_DEVICE_SMOOTHING_TIMEOUT_STEP 25
#endif

// Apply exponential-moving-average smoothing to a mouse report.
report_mouse_t pointing_device_smoothing_apply(report_mouse_t mouse_report);

float    pointing_device_smoothing_get_factor(void);
void     pointing_device_smoothing_set_factor(float factor);
void     pointing_device_smoothing_factor_increment(void);
bool     pointing_device_smoothing_get_enabled(void);
void     pointing_device_smoothing_set_enabled(bool enable);
void     pointing_device_smoothing_toggle_enabled(void);
uint32_t pointing_device_smoothing_get_reset_timeout(void);
void     pointing_device_smoothing_set_reset_timeout(uint32_t timeout_ms);
void     pointing_device_smoothing_timeout_increment(void);
