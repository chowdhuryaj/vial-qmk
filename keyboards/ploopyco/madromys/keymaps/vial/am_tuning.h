// SPDX-License-Identifier: GPL-2.0-or-later
//
// Runtime tunability for QMK's built-in pointing_device_auto_mouse feature
// (quantum/pointing_device/pointing_device_auto_mouse.c). The core exposes
// runtime setters for enable/layer/timeout/debounce, but the activation
// *threshold* is only a compile-time macro (AUTO_MOUSE_THRESHOLD) baked into
// the weak default auto_mouse_activation(). This module overrides that weak
// function with an equivalent that reads a mutable threshold instead, so it
// can be stepped live from a keycode like the other tunables.

#pragma once

#include <stdint.h>

// Step applied per keypress when adjusting timeout/threshold. Shift inverts, Ctrl x10.
#ifndef AUTO_MOUSE_TIME_STEP
#    define AUTO_MOUSE_TIME_STEP 50
#endif
#ifndef AUTO_MOUSE_THRESHOLD_STEP
#    define AUTO_MOUSE_THRESHOLD_STEP 20
#endif

uint16_t am_tuning_get_threshold(void);
void     am_tuning_set_threshold(uint16_t threshold);
void     am_tuning_threshold_increment(void);

void am_tuning_timeout_increment(void);
void am_tuning_toggle_enabled(void);
