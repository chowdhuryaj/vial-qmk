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

// RATCHET: default ball travel (sensor counts) per repeated key. Lower = fires
// more often. Runtime-adjustable via pd_gestures_set_ratchet_step().
#ifndef PD_GESTURES_RATCHET_STEP
#    define PD_GESTURES_RATCHET_STEP 400
#endif
// Clamp range for the runtime ratchet step.
#ifndef PD_GESTURES_RATCHET_STEP_MIN
#    define PD_GESTURES_RATCHET_STEP_MIN 50
#endif
#ifndef PD_GESTURES_RATCHET_STEP_MAX
#    define PD_GESTURES_RATCHET_STEP_MAX 1000
#endif

// 8 directions since 2026-07-02 (was 4): cardinals + diagonals. Index order is
// East then clockwise in 45° sectors: 0=E 1=SE 2=S 3=SW 4=W 5=NW 6=N 7=NE.
// An empty (KC_NO) DIAGONAL slot falls back to the nearest cardinal by
// dominant axis, so 4-direction sets keep their old 90°-sector behavior.
#define PD_GESTURES_NUM_DIRECTIONS 8

// Swallow movement while a gesture is active and fire one key per RATCHET_STEP
// of accumulated travel, in whichever of 8 directions dominates.
report_mouse_t pd_gestures_apply(report_mouse_t mouse_report);

// `table` is an 8-entry (PD_GESTURES_NUM_DIRECTIONS) direction->keycode array
// owned by the keymap and must outlive the gesture (use a static/const array).
// Direction index order: 0=E 1=SE 2=S 3=SW 4=W 5=NW 6=N 7=NE (see above).
void pd_gestures_begin(const uint16_t *table);  // hold style: call on press
void pd_gestures_end(void);                     // hold style: call on release (stops)
void pd_gestures_toggle(const uint16_t *table); // latch on/off (re-press same table = off)
void pd_gestures_cancel(void);                  // stop without firing
bool pd_gestures_is_active(void);
// True iff a gesture is active AND was started from this exact table pointer.
// Lets the keymap distinguish "toggle this set off" from "start this set".
bool pd_gestures_is_active_table(const uint16_t *table);

// Runtime ratchet step (travel per fired key). Setter clamps to
// [PD_GESTURES_RATCHET_STEP_MIN, PD_GESTURES_RATCHET_STEP_MAX].
uint16_t pd_gestures_get_ratchet_step(void);
void     pd_gestures_set_ratchet_step(uint16_t step);
