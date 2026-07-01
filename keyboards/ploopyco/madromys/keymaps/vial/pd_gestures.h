// Copyright 2024-2025 @tzarc
// Copyright 2025 Christopher Courtney, aka Drashna Jael're (@drashna)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from drashna's `pointing_device_gestures` community module into a
// self-contained keymap module. Renamed with a `pd_gestures_` prefix to avoid
// colliding with QMK's built-in `pointing_device_gestures` (cursor glide).
//
// Ratchet gestures: while a gesture is active, ball movement is swallowed (no
// cursor motion) and one keycode is tapped per RATCHET_STEP of accumulated
// travel, chosen from an 8-direction table the keymap supplies.

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "report.h"

// RATCHET: ball travel (sensor counts) per repeated key. Lower = fires more often.
#ifndef PD_GESTURES_RATCHET_STEP
#    define PD_GESTURES_RATCHET_STEP 400
#endif

#define PD_GESTURES_NUM_DIRECTIONS 4

// Swallow movement while a gesture is active and fire one key per RATCHET_STEP
// of accumulated travel, in whichever of 8 directions dominates.
report_mouse_t pd_gestures_apply(report_mouse_t mouse_report);

// `table` is a 4-entry (PD_GESTURES_NUM_DIRECTIONS) direction->keycode array
// owned by the keymap and must outlive the gesture (use a static/const array).
// Direction index order: 0=E 1=S 2=W 3=N (East, then clockwise).
void pd_gestures_begin(const uint16_t *table);  // hold style: call on press
void pd_gestures_end(void);                     // hold style: call on release (stops)
void pd_gestures_toggle(const uint16_t *table); // latch on/off (re-press same table = off)
void pd_gestures_cancel(void);                  // stop without firing
bool pd_gestures_is_active(void);
