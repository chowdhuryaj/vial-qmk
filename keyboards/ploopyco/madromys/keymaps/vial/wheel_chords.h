// SPDX-License-Identifier: GPL-2.0-or-later
//
// Wheel chords: hold a mouse button (BTN1..BTN8) and move the ball — the
// motion is swallowed and fires gesture keycodes in one of 8 directions,
// one tap per WHEEL_CHORDS_STEP of accumulated travel (same ratchet + 45°
// binning as pd_gestures, but gated by a held button instead of a latch).
// The button's own click is NOT suppressed (user decision 2026-07-03): press
// clicks as normal, motion-while-held adds gestures on top. A button whose
// 8 slots are all KC_NO passes motion through untouched, so plain drag with
// an unconfigured button is unaffected.
//
// On the Svalboard this module is fed the LEFT (scroll) ball's counts; on
// the Adept, the single ball. The feeding pipeline decides — the module only
// sees deltas + button state.

#pragma once

#include "quantum.h"

#define WHEEL_CHORDS_BUTTONS 8
#define WHEEL_CHORDS_DIRECTIONS 8 // internal order E SE S SW W NW N NE

#ifndef WHEEL_CHORDS_STEP_DEFAULT
#    define WHEEL_CHORDS_STEP_DEFAULT 200 // sensor counts per gesture fire
#endif
#ifndef WHEEL_CHORDS_STEP_MIN
#    define WHEEL_CHORDS_STEP_MIN 50
#endif
#ifndef WHEEL_CHORDS_STEP_MAX
#    define WHEEL_CHORDS_STEP_MAX 2000
#endif
#ifndef WHEEL_CHORDS_ENABLED_DEFAULT
#    define WHEEL_CHORDS_ENABLED_DEFAULT true
#endif

// Hold delay (2026-07-03, user ask): capture only engages after the button
// has been held this long, so a quick click-drag stays a normal drag and a
// deliberate hold becomes a chord. 0 = capture immediately (old behavior).
#ifndef WHEEL_CHORDS_HOLD_MS_DEFAULT
#    define WHEEL_CHORDS_HOLD_MS_DEFAULT 200
#endif
#ifndef WHEEL_CHORDS_HOLD_MS_MAX
#    define WHEEL_CHORDS_HOLD_MS_MAX 1000
#endif

bool     wheel_chords_get_enabled(void);
void     wheel_chords_set_enabled(bool enabled);
uint16_t wheel_chords_get_step(void);
void     wheel_chords_set_step(uint16_t step);
uint16_t wheel_chords_get_hold_ms(void);
void     wheel_chords_set_hold_ms(uint16_t ms);

// The 8x8 slot table (buttons x directions), raw QMK keycodes. Returned as a
// pointer so the keymap can memcpy to/from its EEPROM snapshot.
uint16_t (*wheel_chords_table(void))[WHEEL_CHORDS_DIRECTIONS];

// Feed physical button state (button 0 = BTN1). Call from process_record on
// KC_BTN1..KC_BTN8 presses/releases (including layer-tap tap halves).
void wheel_chords_on_button(uint8_t button, bool pressed);

// True while a held button has at least one configured slot — the feeding
// pipeline should route ball counts into _feed and drop them from output.
bool wheel_chords_capturing(void);

// Accumulate swallowed motion and fire slots on threshold.
void wheel_chords_feed(int16_t dx, int16_t dy);

// Convenience for report-based pipelines (Adept): swallows x/y while
// capturing, otherwise passes the report through untouched.
report_mouse_t wheel_chords_apply(report_mouse_t mouse_report);
